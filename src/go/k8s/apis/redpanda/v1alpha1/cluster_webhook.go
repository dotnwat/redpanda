// Copyright 2021 Vectorized, Inc.
//
// Use of this software is governed by the Business Source License
// included in the file licenses/BSL.md
//
// As of the Change Date specified in that file, in accordance with
// the Business Source License, use of this software will be governed
// by the Apache License, Version 2.0

package v1alpha1

import (
	"fmt"
	"strconv"

	cmmeta "github.com/jetstack/cert-manager/pkg/apis/meta/v1"
	corev1 "k8s.io/api/core/v1"
	apierrors "k8s.io/apimachinery/pkg/api/errors"
	"k8s.io/apimachinery/pkg/api/resource"
	"k8s.io/apimachinery/pkg/runtime"
	"k8s.io/apimachinery/pkg/util/intstr"
	"k8s.io/apimachinery/pkg/util/validation/field"
	ctrl "sigs.k8s.io/controller-runtime"
	logf "sigs.k8s.io/controller-runtime/pkg/log"
	"sigs.k8s.io/controller-runtime/pkg/webhook"
)

const (
	kb = 1024
	mb = 1024 * kb
	gb = 1024 * mb

	defaultTopicReplicationNumber           = 3
	transactionCoordinatorReplicationNumber = 3
	idAllocatorReplicationNumber            = 3
	minimumReplicas                         = 3

	defaultTopicReplicationKey           = "redpanda.default_topic_replications"
	transactionCoordinatorReplicationKey = "redpanda.transaction_coordinator_replication"
	idAllocatorReplicationKey            = "redpanda.id_allocator_replication"

	defaultSchemaRegistryPort = 8081
)

type resourceField struct {
	resources *corev1.ResourceRequirements
	path      *field.Path
}

type redpandaResourceField struct {
	resources *RedpandaResourceRequirements
	path      *field.Path
}

// log is for logging in this package.
var log = logf.Log.WithName("cluster-resource")

// SetupWebhookWithManager autogenerated function by kubebuilder
func (r *Cluster) SetupWebhookWithManager(mgr ctrl.Manager) error {
	return ctrl.NewWebhookManagedBy(mgr).
		For(r).
		Complete()
}

//+kubebuilder:webhook:path=/mutate-redpanda-vectorized-io-v1alpha1-cluster,mutating=true,failurePolicy=fail,sideEffects=None,groups=redpanda.vectorized.io,resources=clusters,verbs=create;update,versions=v1alpha1,name=mcluster.kb.io,admissionReviewVersions={v1,v1beta1}

var _ webhook.Defaulter = &Cluster{}

func redpandaResourceFields(c *Cluster) redpandaResourceField {
	return redpandaResourceField{&c.Spec.Resources, field.NewPath("spec").Child("resources")}
}

func sidecarResourceFields(c *Cluster) []resourceField {
	var resources []resourceField

	if c.Spec.Sidecars.RpkStatus != nil && c.Spec.Sidecars.RpkStatus.Enabled {
		resources = append(resources, resourceField{c.Spec.Sidecars.RpkStatus.Resources, field.NewPath("spec").Child("resourcesRpkStatus")})
	}
	return resources
}

// Default implements defaulting webhook logic - all defaults that should be
// applied to cluster CRD after user submits it should be put in here
func (r *Cluster) Default() {
	log.Info("default", "name", r.Name)
	if r.Spec.Configuration.SchemaRegistry != nil && r.Spec.Configuration.SchemaRegistry.Port == 0 {
		r.Spec.Configuration.SchemaRegistry.Port = defaultSchemaRegistryPort
	}

	if r.Spec.CloudStorage.Enabled && r.Spec.CloudStorage.CacheStorage != nil && r.Spec.CloudStorage.CacheStorage.Capacity.Value() == 0 {
		r.Spec.CloudStorage.CacheStorage.Capacity = resource.MustParse("20G")
	}

	r.setDefaultAdditionalConfiguration()
	if r.Spec.PodDisruptionBudget == nil {
		defaultMaxUnavailable := intstr.FromInt(1)
		r.Spec.PodDisruptionBudget = &PDBConfig{
			Enabled:        true,
			MaxUnavailable: &defaultMaxUnavailable,
		}
	}
}

var defaultAdditionalConfiguration = map[string]int{
	defaultTopicReplicationKey:           defaultTopicReplicationNumber,
	transactionCoordinatorReplicationKey: transactionCoordinatorReplicationNumber,
	idAllocatorReplicationKey:            idAllocatorReplicationNumber,
}

// setDefaultAdditionalConfiguration sets additional configuration fields based
// on the best practices
func (r *Cluster) setDefaultAdditionalConfiguration() {
	if *r.Spec.Replicas >= minimumReplicas {
		if r.Spec.AdditionalConfiguration == nil {
			r.Spec.AdditionalConfiguration = make(map[string]string)
		}

		for k, v := range defaultAdditionalConfiguration {
			_, ok := r.Spec.AdditionalConfiguration[k]
			if !ok {
				r.Spec.AdditionalConfiguration[k] = strconv.Itoa(v)
			}
		}
	}
}

// TODO(user): change verbs to "verbs=create;update;delete" if you want to enable deletion validation.
//+kubebuilder:webhook:path=/validate-redpanda-vectorized-io-v1alpha1-cluster,mutating=false,failurePolicy=fail,sideEffects=None,groups=redpanda.vectorized.io,resources=clusters,verbs=create;update,versions=v1alpha1,name=vcluster.kb.io,admissionReviewVersions={v1,v1beta1}

var _ webhook.Validator = &Cluster{}

// ValidateCreate implements webhook.Validator so a webhook will be registered for the type
func (r *Cluster) ValidateCreate() error {
	log.Info("validate create", "name", r.Name)

	var allErrs field.ErrorList

	allErrs = append(allErrs, r.validateKafkaListeners()...)

	allErrs = append(allErrs, r.validateAdminListeners()...)

	allErrs = append(allErrs, r.validatePandaproxyListeners()...)

	allErrs = append(allErrs, r.validateSchemaRegistryListener()...)

	allErrs = append(allErrs, r.checkCollidingPorts()...)

	allErrs = append(allErrs, r.validateRedpandaMemory()...)

	allErrs = append(allErrs, r.validateRedpandaResources(redpandaResourceFields(r))...)

	for _, rf := range sidecarResourceFields(r) {
		allErrs = append(allErrs, r.validateResources(rf)...)
	}

	allErrs = append(allErrs, r.validateArchivalStorage()...)

	allErrs = append(allErrs, r.validatePodDisruptionBudget()...)

	if len(allErrs) == 0 {
		return nil
	}

	return apierrors.NewInvalid(
		r.GroupVersionKind().GroupKind(),
		r.Name, allErrs)
}

// ValidateUpdate implements webhook.Validator so a webhook will be registered for the type
func (r *Cluster) ValidateUpdate(old runtime.Object) error {
	log.Info("validate update", "name", r.Name)
	oldCluster := old.(*Cluster)
	var allErrs field.ErrorList

	if r.Spec.Replicas != nil && oldCluster.Spec.Replicas != nil && *r.Spec.Replicas < *oldCluster.Spec.Replicas {
		allErrs = append(allErrs,
			field.Invalid(field.NewPath("spec").Child("replicas"),
				r.Spec.Replicas,
				"scaling down is not supported"))
	}
	allErrs = append(allErrs, r.validateKafkaListeners()...)

	allErrs = append(allErrs, r.validateAdminListeners()...)

	allErrs = append(allErrs, r.validatePandaproxyListeners()...)

	allErrs = append(allErrs, r.validateSchemaRegistryListener()...)

	allErrs = append(allErrs, r.checkCollidingPorts()...)

	allErrs = append(allErrs, r.validateRedpandaMemory()...)

	allErrs = append(allErrs, r.validateRedpandaCoreChanges(oldCluster)...)

	allErrs = append(allErrs, r.validateRedpandaResources(redpandaResourceFields(r))...)

	for _, rf := range sidecarResourceFields(r) {
		allErrs = append(allErrs, r.validateResources(rf)...)
	}

	allErrs = append(allErrs, r.validateArchivalStorage()...)

	allErrs = append(allErrs, r.validatePodDisruptionBudget()...)

	if len(allErrs) == 0 {
		return nil
	}

	return apierrors.NewInvalid(
		r.GroupVersionKind().GroupKind(),
		r.Name, allErrs)
}

func (r *Cluster) validateAdminListeners() field.ErrorList {
	var allErrs field.ErrorList
	externalAdmin := r.AdminAPIExternal()
	targetAdminCount := 1
	if externalAdmin != nil {
		targetAdminCount = 2
	}
	if len(r.Spec.Configuration.AdminAPI) != targetAdminCount {
		allErrs = append(allErrs,
			field.Invalid(field.NewPath("spec").Child("configuration").Child("adminApi"),
				r.Spec.Configuration.AdminAPI,
				"need exactly one internal API listener and up to one external"))
	}

	if externalAdmin != nil && externalAdmin.Port != 0 {
		allErrs = append(allErrs,
			field.Invalid(field.NewPath("spec").Child("configuration").Child("adminApi"),
				r.Spec.Configuration.AdminAPI,
				"external admin listener cannot have port specified"))
	}

	// for now only one listener can have TLS to be backward compatible with v1alpha1 API
	foundListenerWithTLS := false
	for i, p := range r.Spec.Configuration.AdminAPI {
		if p.TLS.Enabled {
			if foundListenerWithTLS {
				allErrs = append(allErrs,
					field.Invalid(field.NewPath("spec").Child("configuration").Child("adminApi").Index(i).Child("tls"),
						r.Spec.Configuration.AdminAPI[i].TLS,
						"only one listener can have TLS enabled"))
			}
			foundListenerWithTLS = true
		}
		// we need to run the validation on all listeners to also catch errors like !Enabled && RequireClientAuth
		allErrs = append(allErrs, validateAdminTLS(p.TLS, field.NewPath("spec").Child("configuration").Child("adminApi").Index(i).Child("tls"))...)
	}
	return allErrs
}

func (r *Cluster) validateKafkaListeners() field.ErrorList {
	var allErrs field.ErrorList
	if len(r.Spec.Configuration.KafkaAPI) == 0 {
		allErrs = append(allErrs,
			field.Invalid(field.NewPath("spec").Child("configuration").Child("kafkaApi"),
				r.Spec.Configuration.KafkaAPI,
				"need at least one kafka api listener"))
	}

	var external *KafkaAPI
	for i, p := range r.Spec.Configuration.KafkaAPI {
		if p.External.Enabled {
			if external != nil {
				allErrs = append(allErrs,
					field.Invalid(field.NewPath("spec").Child("configuration").Child("kafkaApi"),
						r.Spec.Configuration.KafkaAPI,
						"only one kafka api listener can be marked as external"))
			}
			external = &r.Spec.Configuration.KafkaAPI[i]
		}
	}

	// for now only one listener can have TLS to be backward compatible with v1alpha1 API
	foundListenerWithTLS := false
	for i, p := range r.Spec.Configuration.KafkaAPI {
		if p.TLS.Enabled {
			if foundListenerWithTLS {
				allErrs = append(allErrs,
					field.Invalid(field.NewPath("spec").Child("configuration").Child("kafkaApi").Index(i).Child("tls"),
						r.Spec.Configuration.KafkaAPI[i].TLS,
						"only one listener can have TLS enabled"))
			}
			foundListenerWithTLS = true
		}
		// we need to run the validation on all listeners to also catch errors like !Enabled && RequireClientAuth
		tlsErrs := validateTLS(
			p.TLS.Enabled,
			p.TLS.RequireClientAuth,
			p.TLS.IssuerRef,
			p.TLS.NodeSecretRef,
			field.NewPath("spec").Child("configuration").Child("kafkaApi").Index(i).Child("tls"))
		allErrs = append(allErrs, tlsErrs...)
	}

	if !((len(r.Spec.Configuration.KafkaAPI) == 2 && external != nil) || (external == nil && len(r.Spec.Configuration.KafkaAPI) == 1)) {
		allErrs = append(allErrs,
			field.Invalid(field.NewPath("spec").Child("configuration").Child("kafkaApi"),
				r.Spec.Configuration.KafkaAPI,
				"one internal listener and up to to one external kafka api listener is required"))
	}
	if external != nil && external.Port != 0 && (external.Port < 30000 || external.Port > 32768) {
		allErrs = append(allErrs,
			field.Invalid(field.NewPath("spec").Child("configuration").Child("kafkaApi"),
				r.Spec.Configuration.KafkaAPI,
				"external port must be in the following range: 30000-32768"))
	}

	return allErrs
}

func (r *Cluster) validatePandaproxyListeners() field.ErrorList {
	var allErrs field.ErrorList
	var proxyExternal *PandaproxyAPI
	kafkaExternal := r.ExternalListener()
	for i, p := range r.Spec.Configuration.PandaproxyAPI {
		if !p.External.Enabled {
			continue
		}
		if proxyExternal != nil {
			allErrs = append(allErrs,
				field.Invalid(field.NewPath("spec").Child("configuration").Child("pandaproxyApi").Index(i),
					r.Spec.Configuration.PandaproxyAPI[i],
					"only one pandaproxy api listener can be marked as external"))
		}
		proxyExternal = &r.Spec.Configuration.PandaproxyAPI[i]
		if proxyExternal.Port != 0 {
			allErrs = append(allErrs,
				field.Invalid(field.NewPath("spec").Child("configuration").Child("pandaproxyApi").Index(i),
					r.Spec.Configuration.PandaproxyAPI[i],
					"external pandaproxy api listener cannot have port specified, it's autogenerated"))
		}
		if (kafkaExternal == nil || !kafkaExternal.External.Enabled) && (proxyExternal != nil && proxyExternal.External.Enabled) {
			allErrs = append(allErrs,
				field.Invalid(field.NewPath("spec").Child("configuration").Child("pandaproxyApi").Index(i),
					r.Spec.Configuration.PandaproxyAPI[i],
					"cannot have a pandaproxy external listener without a kafka external listener"))
		}
		if kafkaExternal == nil && proxyExternal.External.Subdomain != "" {
			allErrs = append(allErrs,
				field.Invalid(field.NewPath("spec").Child("configuration").Child("pandaproxyApi").Index(i),
					r.Spec.Configuration.PandaproxyAPI[i],
					"kafka external listener is empty but must specify the same sudomain as that of the external pandaproxy"))
		}
		if kafkaExternal != nil && kafkaExternal.External.Subdomain != proxyExternal.External.Subdomain {
			allErrs = append(allErrs,
				field.Invalid(field.NewPath("spec").Child("configuration").Child("pandaproxyApi").Index(i),
					r.Spec.Configuration.PandaproxyAPI[i],
					"sudomain of external pandaproxy must be the same as kafka's"))
		}
	}

	// for now only one listener can have TLS to be backward compatible with v1alpha1 API
	foundListenerWithTLS := false
	for i, p := range r.Spec.Configuration.PandaproxyAPI {
		if p.TLS.Enabled {
			if foundListenerWithTLS {
				allErrs = append(allErrs,
					field.Invalid(field.NewPath("spec").Child("configuration").Child("pandaproxyApi").Index(i).Child("tls"),
						r.Spec.Configuration.PandaproxyAPI[i].TLS,
						"only one pandaproxy listener can have TLS enabled"))
			}
			foundListenerWithTLS = true
		}
		allErrs = append(allErrs, validatePandaproxyTLS(p.TLS, field.NewPath("spec").Child("configuration").Child("pandaproxyApi").Index(i).Child("tls"))...)
	}

	// If we have an external proxy listener and no other listeners, we're missing an internal one
	if proxyExternal != nil && len(r.Spec.Configuration.PandaproxyAPI) == 1 {
		allErrs = append(allErrs,
			field.Invalid(field.NewPath("spec").Child("configuration").Child("pandaproxyApi"),
				r.Spec.Configuration.PandaproxyAPI,
				"an internal pandaproxy listener is required when an external one is provided"))
	}

	if !((len(r.Spec.Configuration.PandaproxyAPI) == 2 && proxyExternal != nil) || (proxyExternal == nil && len(r.Spec.Configuration.PandaproxyAPI) <= 1)) {
		allErrs = append(allErrs,
			field.Invalid(field.NewPath("spec").Child("configuration").Child("pandaproxyApi"),
				r.Spec.Configuration.PandaproxyAPI,
				"up to one internal listener and no external listener, or one external and one internal listener for pandaproxy is allowed"))
	}

	return allErrs
}

func (r *Cluster) validateSchemaRegistryListener() field.ErrorList {
	var allErrs field.ErrorList
	schemaRegistry := r.Spec.Configuration.SchemaRegistry
	if schemaRegistry == nil {
		return allErrs
	}
	if r.IsSchemaRegistryExternallyAvailable() {
		kafkaExternal := r.ExternalListener()

		if kafkaExternal == nil || !kafkaExternal.External.Enabled {
			allErrs = append(allErrs,
				field.Invalid(field.NewPath("spec").Child("configuration").Child("schemaRegistry"),
					r.Spec.Configuration.SchemaRegistry,
					"cannot have a schema registry external listener without a kafka external listener"))
		}
		if kafkaExternal == nil && schemaRegistry.External.Subdomain != "" {
			allErrs = append(allErrs,
				field.Invalid(field.NewPath("spec").Child("configuration").Child("schemaRegistry").Child("external").Child("subdomain"),
					r.Spec.Configuration.SchemaRegistry.External.Subdomain,
					"the external kafka listener can't be empty if the registry subdomain is set"))
		}
		if kafkaExternal != nil && kafkaExternal.External.Subdomain != schemaRegistry.External.Subdomain {
			allErrs = append(allErrs,
				field.Invalid(field.NewPath("spec").Child("configuration").Child("schemaRegistry").Child("external").Child("subdomain"),
					r.Spec.Configuration.SchemaRegistry.External.Subdomain,
					"sudomain of external schema registry must be the same as kafka's"))
		}
	}

	if schemaRegistry.TLS != nil {
		tlsErrs := validateTLS(
			schemaRegistry.TLS.Enabled,
			schemaRegistry.TLS.RequireClientAuth,
			schemaRegistry.TLS.IssuerRef,
			schemaRegistry.TLS.NodeSecretRef,
			field.NewPath("spec").Child("configuration").Child("schemaRegistry").Child("tls"))
		allErrs = append(allErrs, tlsErrs...)
	}
	return allErrs
}

func (r *Cluster) validateResources(rf resourceField) field.ErrorList {
	if rf.resources == nil {
		return nil
	}
	var allErrs field.ErrorList

	// Memory limit (if set) cannot be lower than the requested
	if !rf.resources.Limits.Memory().IsZero() && rf.resources.Limits.Memory().Cmp(*rf.resources.Requests.Memory()) == -1 {
		allErrs = append(allErrs,
			field.Invalid(
				rf.path.Child("requests").Child("memory"),
				rf.resources.Requests.Memory(),
				"limits.memory < requests.memory; either increase the limit or remove it"))
	}

	// CPU limit (if set) cannot be lower than the requested
	if !rf.resources.Requests.Cpu().IsZero() && !rf.resources.Limits.Cpu().IsZero() &&
		rf.resources.Limits.Cpu().Cmp(*rf.resources.Requests.Cpu()) == -1 {
		allErrs = append(allErrs,
			field.Invalid(
				rf.path.Child("requests").Child("cpu"),
				rf.resources.Requests.Cpu(),
				"limits.cpu < requests.cpu; either increase the limit or remove it"))
	}

	return allErrs
}

func (r *Cluster) validateRedpandaResources(
	rf redpandaResourceField,
) field.ErrorList {
	allErrs := r.validateResources(resourceField{&rf.resources.ResourceRequirements, rf.path})

	// Memory redpanda (if set) must be less than or equal to request (if set)
	if !rf.resources.Requests.Memory().IsZero() && !rf.resources.RedpandaMemory().IsZero() && rf.resources.Requests.Memory().Cmp(*rf.resources.RedpandaMemory()) == -1 {
		allErrs = append(allErrs,
			field.Invalid(
				rf.path.Child("redpanda").Child("memory"),
				rf.resources.Requests.Memory(),
				"requests.memory < redpanda.memory; decrease or remove redpanda.memory"))
	}

	return allErrs
}

// validateRedpandaMemory verifies that memory limits are aligned with the minimal requirement of redpanda
// which is defined in `MinimumMemoryPerCore` constant
func (r *Cluster) validateRedpandaMemory() field.ErrorList {
	if r.Spec.Configuration.DeveloperMode {
		// for developer mode we don't enforce any memory limits
		return nil
	}
	var allErrs field.ErrorList

	// Ensure a requested 2GB of memory per core
	requests := r.Spec.Resources.Requests.DeepCopy()
	requests.Cpu().RoundUp(0)
	requestedCores := requests.Cpu().Value()
	if r.Spec.Resources.Requests.Memory().Value() < requestedCores*MinimumMemoryPerCore {
		allErrs = append(allErrs,
			field.Invalid(
				field.NewPath("spec").Child("resources").Child("requests").Child("memory"),
				r.Spec.Resources.Requests.Memory(),
				"requests.memory < 2Gi per core; either decrease requests.cpu or increase requests.memory"))
	}

	redpandaCores := r.Spec.Resources.RedpandaCPU().Value()
	if !r.Spec.Resources.RedpandaMemory().IsZero() && r.Spec.Resources.RedpandaMemory().Value() < redpandaCores*MinimumMemoryPerCore {
		allErrs = append(allErrs,
			field.Invalid(
				field.NewPath("spec").Child("resources").Child("redpanda").Child("memory"),
				r.Spec.Resources.Requests.Memory(),
				"redpanda.memory < 2Gi per core; either decrease redpanda.cpu or increase redpanda.memory"))
	}

	return allErrs
}

// validateRedpandaCoreChanges verifies that the number of cores assigned to each Redpanda node
// are not reduced during cluster updates
func (r *Cluster) validateRedpandaCoreChanges(old *Cluster) field.ErrorList {
	if r.Spec.Configuration.DeveloperMode {
		// for developer mode we don't enforce this rule
		return nil
	}
	var allErrs field.ErrorList

	oldCPURequest := old.Spec.Resources.RedpandaCPU()
	newCPURequest := r.Spec.Resources.RedpandaCPU()
	if oldCPURequest != nil && newCPURequest != nil {
		oldCores := oldCPURequest.Value()
		newCores := newCPURequest.Value()

		if newCores < oldCores {
			minAllowedCPU := (oldCores-1)*1000 + 1
			allErrs = append(allErrs,
				field.Invalid(
					field.NewPath("spec").Child("resources").Child("requests").Child("cpu"),
					r.Spec.Resources.Requests.Cpu(),
					fmt.Sprintf("CPU request must not be decreased; increase requests.cpu or redpanda.cpu to at least %dm", minAllowedCPU)))
		}
	}

	return allErrs
}

func validateAdminTLS(tlsConfig AdminAPITLS, path *field.Path) field.ErrorList {
	var allErrs field.ErrorList
	if tlsConfig.RequireClientAuth && !tlsConfig.Enabled {
		allErrs = append(allErrs,
			field.Invalid(
				path.Child("requireclientauth"),
				tlsConfig.RequireClientAuth,
				"Enabled has to be set to true for RequireClientAuth to be allowed to be true"))
	}
	return allErrs
}

func validateTLS(
	tlsEnabled, requireClientAuth bool,
	issuerRef *cmmeta.ObjectReference,
	nodeSecretRef *corev1.ObjectReference,
	path *field.Path,
) field.ErrorList {
	var allErrs field.ErrorList
	if requireClientAuth && !tlsEnabled {
		allErrs = append(allErrs,
			field.Invalid(
				path.Child("requireClientAuth"),
				requireClientAuth,
				"Enabled has to be set to true for RequireClientAuth to be allowed to be true"))
	}
	if issuerRef != nil && nodeSecretRef != nil {
		allErrs = append(allErrs,
			field.Invalid(
				path.Child("nodeSecretRef"),
				nodeSecretRef,
				"Cannot provide both IssuerRef and NodeSecretRef"))
	}
	return allErrs
}

func validatePandaproxyTLS(
	tlsConfig PandaproxyAPITLS, path *field.Path,
) field.ErrorList {
	var allErrs field.ErrorList
	if tlsConfig.RequireClientAuth && !tlsConfig.Enabled {
		allErrs = append(allErrs,
			field.Invalid(
				path.Child("requireclientauth"),
				tlsConfig.RequireClientAuth,
				"Enabled has to be set to true for RequireClientAuth to be allowed to be true"))
	}
	return allErrs
}

func (r *Cluster) validateArchivalStorage() field.ErrorList {
	var allErrs field.ErrorList
	if !r.Spec.CloudStorage.Enabled {
		return allErrs
	}
	if r.Spec.CloudStorage.AccessKey == "" {
		allErrs = append(allErrs,
			field.Invalid(
				field.NewPath("spec").Child("configuration").Child("cloudStorage").Child("accessKey"),
				r.Spec.CloudStorage.AccessKey,
				"AccessKey has to be provided for cloud storage to be enabled"))
	}
	if r.Spec.CloudStorage.Bucket == "" {
		allErrs = append(allErrs,
			field.Invalid(
				field.NewPath("spec").Child("configuration").Child("cloudStorage").Child("bucket"),
				r.Spec.CloudStorage.Bucket,
				"Bucket has to be provided for cloud storage to be enabled"))
	}
	if r.Spec.CloudStorage.Region == "" {
		allErrs = append(allErrs,
			field.Invalid(
				field.NewPath("spec").Child("configuration").Child("cloudStorage").Child("region"),
				r.Spec.CloudStorage.Region,
				"Region has to be provided for cloud storage to be enabled"))
	}
	if r.Spec.CloudStorage.SecretKeyRef.Name == "" {
		allErrs = append(allErrs,
			field.Invalid(
				field.NewPath("spec").Child("configuration").Child("cloudStorage").Child("secretKeyRef").Child("name"),
				r.Spec.CloudStorage.SecretKeyRef.Name,
				"SecretKeyRef name has to be provided for cloud storage to be enabled"))
	}
	if r.Spec.CloudStorage.SecretKeyRef.Namespace == "" {
		allErrs = append(allErrs,
			field.Invalid(
				field.NewPath("spec").Child("configuration").Child("cloudStorage").Child("secretKeyRef").Child("namespace"),
				r.Spec.CloudStorage.SecretKeyRef.Namespace,
				"SecretKeyRef namespace has to be provided for cloud storage to be enabled"))
	}
	return allErrs
}

func (r *Cluster) validatePodDisruptionBudget() field.ErrorList {
	var allErrs field.ErrorList
	if r.Spec.PodDisruptionBudget == nil {
		return allErrs
	}
	if (r.Spec.PodDisruptionBudget.MaxUnavailable != nil || r.Spec.PodDisruptionBudget.MinAvailable != nil) &&
		!r.Spec.PodDisruptionBudget.Enabled {
		allErrs = append(allErrs,
			field.Invalid(
				field.NewPath("spec").Child("podDisruptionBudget"),
				r.Spec.PodDisruptionBudget,
				"MaxUnavailable or MinAvailable is set but the podDisruptionBudget is not enabled"))
	}
	if r.Spec.PodDisruptionBudget.Enabled && r.Spec.PodDisruptionBudget.MaxUnavailable != nil && r.Spec.PodDisruptionBudget.MinAvailable != nil {
		allErrs = append(allErrs,
			field.Invalid(
				field.NewPath("spec").Child("podDisruptionBudget"),
				r.Spec.PodDisruptionBudget,
				"Cannot specify both MaxUnavailable and MinAvailable in PodDisruptionBudget"))
	}
	return allErrs
}

// ValidateDelete implements webhook.Validator so a webhook will be registered for the type
func (r *Cluster) ValidateDelete() error {
	log.Info("validate delete", "name", r.Name)

	// TODO(user): fill in your validation logic upon object deletion.
	return nil
}

type listenersPorts struct {
	name                 string
	port                 int
	externalConnectivity bool
	externalPort         *int
}

// TODO move this to networking package
func (r *Cluster) checkCollidingPorts() field.ErrorList {
	var allErrs field.ErrorList
	ports := r.getAllPorts()

	for i := range ports {
		for j := len(ports) - 1; j > i; j-- {
			if ports[i].port == ports[j].port {
				allErrs = append(allErrs, field.Invalid(field.NewPath("spec").Child("configuration", ports[i].name, "port"),
					ports[i].port,
					fmt.Sprintf("%s port collide with Spec.Configuration.%s Port", ports[i].name, ports[j].name)))
			}
			externalPortJ := ports[j].port + 1
			if ports[j].externalPort != nil {
				externalPortJ = *ports[j].externalPort
			}
			externalPortI := ports[i].port + 1
			if ports[i].externalPort != nil {
				externalPortI = *ports[i].externalPort
			}
			if ports[j].externalConnectivity && ports[i].port == externalPortJ {
				allErrs = append(allErrs, field.Invalid(field.NewPath("spec").Child("configuration", ports[i].name, "port"),
					ports[i].port,
					fmt.Sprintf("%s port collide with external %s port", ports[i].name, ports[j].name)))
			}

			if ports[i].externalConnectivity && externalPortI == ports[j].port {
				allErrs = append(allErrs, field.Invalid(field.NewPath("spec").Child("configuration", ports[i].name, "port"),
					ports[i].port,
					fmt.Sprintf("external %s port collide with Spec.Configuration.%s port", ports[i].name, ports[j].name)))
			}

			if ports[i].externalConnectivity && ports[j].externalConnectivity && externalPortI == externalPortJ {
				allErrs = append(allErrs, field.Invalid(field.NewPath("spec").Child("configuration", ports[i].name, "port"),
					ports[i].port,
					fmt.Sprintf("external %s port collide with external %s Port that is not defined in CR", ports[i].name, ports[j].name)))
			}
		}
	}

	return allErrs
}

func (r *Cluster) getAllPorts() []listenersPorts {
	ports := []listenersPorts{
		{
			name:                 "RPCApi",
			port:                 r.Spec.Configuration.RPCServer.Port,
			externalConnectivity: false,
		},
	}

	if internal := r.InternalListener(); internal != nil {
		externalListener := r.ExternalListener()
		var externalPort *int
		if externalListener != nil && externalListener.Port != 0 {
			externalPort = &externalListener.Port
		}
		ports = append(ports, listenersPorts{
			name:                 "kafkaApi",
			port:                 internal.Port,
			externalConnectivity: externalListener != nil,
			externalPort:         externalPort,
		})
	}

	if internal := r.AdminAPIInternal(); internal != nil {
		ports = append(ports, listenersPorts{
			name:                 "adminApi",
			port:                 internal.Port,
			externalConnectivity: r.AdminAPIExternal() != nil,
		})
	}

	if internal := r.PandaproxyAPIInternal(); internal != nil {
		ports = append(ports, listenersPorts{
			name:                 "pandaproxyApi",
			port:                 internal.Port,
			externalConnectivity: r.PandaproxyAPIExternal() != nil,
		})
	}

	if r.Spec.Configuration.SchemaRegistry != nil {
		ports = append(ports, listenersPorts{
			name: "schemaRegistryApi",
			port: r.Spec.Configuration.SchemaRegistry.Port,
			// Schema registry does not have problem with external port being hidden next port of the
			// internal one.
			externalConnectivity: false,
		})
	}
	return ports
}
