#!/bin/env bash
set -euo pipefail

server=localhost
domain=$(dnsdomainname)
name=$server

country=SE
state=Stockholm
locality=$state
org=$domain
unit=$domain
mail=mx
common=$server
subj=/C=$country/ST=$state/L=$locality/O=$domain/OU=$domain/CN=$common
client1=/C=$country/ST=$state/L=$locality/O=$domain/OU=$domain/CN=client1.org
client2=/C=$country/ST=$state/L=$locality/O=$domain/OU=$domain/CN=client2.org


openssl ecparam -name prime256v1 -genkey -noout -out mtls_server.key
openssl req -new -sha256 -key mtls_server.key -out mtls_server.csr -subj "$subj" -addext "subjectAltName = DNS:$common"
openssl x509 -req -in mtls_server.csr -CA mtls_ca.crt -CAkey mtls_ca.key -CAcreateserial -out mtls_server.crt -days 1000 -sha256 -extfile <(printf 'subjectAltName=DNS:%s' "$common") 

openssl ecparam -name prime256v1 -genkey -noout -out mtls_client1.key
openssl req -new -sha256 -key mtls_client1.key -out mtls_client1.csr  -subj "$client1" -addext "subjectAltName = DNS:$common"
openssl x509 -req -in mtls_client1.csr -CA mtls_ca.crt -CAkey mtls_ca.key -CAcreateserial -out mtls_client1.crt -days 1000 -sha256

openssl ecparam -name prime256v1 -genkey -noout -out mtls_client2.key
openssl req -new -sha256 -key mtls_client2.key -out mtls_client2.csr  -subj "$client2" -addext "subjectAltName = DNS:$common"
openssl x509 -req -in mtls_client2.csr -CA mtls_ca.crt -CAkey mtls_ca.key -CAcreateserial -out mtls_client2.crt -days 1000 -sha256
        self.ca_cnf = tempfile.NamedTemporaryFile()
