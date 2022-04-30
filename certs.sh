#!/usr/bin/env bash
set -e
set -x

function gen_ca {
mkdir certs my-safe-directory
cat > ca.cnf <<EOF
# OpenSSL CA configuration file
[ ca ]
default_ca = CA_default
[ CA_default ]
default_days = 365
database = index.txt
serial = serial.txt
default_md = sha256
copy_extensions = copy
unique_subject = no
# Used to create the CA certificate.
[ req ]
prompt=no
distinguished_name = distinguished_name
x509_extensions = extensions
[ distinguished_name ]
organizationName = Vectorized
commonName = Vectorized CA
[ extensions ]
keyUsage = critical,digitalSignature,nonRepudiation,keyEncipherment,keyCertSign
basicConstraints = critical,CA:true,pathlen:1
# Common policy for nodes and users.
[ signing_policy ]
organizationName = supplied
commonName = optional
# Used to sign node certificates.
[ signing_node_req ]
keyUsage = critical,digitalSignature,keyEncipherment
extendedKeyUsage = serverAuth,clientAuth
# Used to sign client certificates.
[ signing_client_req ]
keyUsage = critical,digitalSignature,keyEncipherment
extendedKeyUsage = clientAuth
EOF
openssl genrsa -out my-safe-directory/ca.key 2048
chmod 400 my-safe-directory/ca.key
#openssl req -new -x509 -config ca.cnf -key my-safe-directory/ca.key -out certs/ca.key -days 365 -batch
openssl req -new -x509 -config ca.cnf -key my-safe-directory/ca.key -out certs/ca.crt -days 365 -batch
rm -f index.txt serial.txt
touch index.txt
echo '01' > serial.txt
}

function gen_node {
    name=${1}
cat > ${name}.cnf <<EOF
[ req ]
prompt=no
distinguished_name = distinguished_name
req_extensions = extensions
[ distinguished_name ]
organizationName = ${1}
[ extensions ]
subjectAltName = critical,DNS:localhost,IP:127.0.0.1
EOF
openssl genrsa -out certs/${name}.key 2048
chmod 400 certs/${name}.key
openssl req -new -config ${name}.cnf -key certs/${name}.key -out ${name}.csr -batch
openssl ca -config ca.cnf -keyfile my-safe-directory/ca.key -cert certs/ca.crt \
	-policy signing_policy -extensions signing_node_req -out certs/${name}.crt \
    -outdir certs/ -in ${name}.csr -batch
}

gen_ca
gen_node server iamserver
gen_node client iamclient
