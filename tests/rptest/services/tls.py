import tempfile
import typing
import collections
import pathlib
import subprocess
import os

_ca_config_tmpl = """
# OpenSSL CA configuration file
[ ca ]
default_ca = local_ca

[ local_ca ]
dir              = {dir}
database         = $dir/index.txt
serial           = $dir/serial.txt
default_days     = 730
default_md       = sha256
copy_extensions  = copy
unique_subject   = no

# Used to create the CA certificate.
[ req ]
prompt             = no
distinguished_name = distinguished_name
x509_extensions    = extensions

[ root_ca_distinguished_name ]
commonName              = Test TLS CA
stateOrProvinceName     = NY
countryName             = US
emailAddress            = hi@vectorized.io
organizationName        = Vectorized
organizationalUnitName  = Vectorized Test

[ distinguished_name ]
organizationName = Vectorized
commonName       = Vectorized Test CA

[ extensions ]
keyUsage         = critical,digitalSignature,nonRepudiation,keyEncipherment,keyCertSign
basicConstraints = critical,CA:true,pathlen:1

# Common policy for nodes and users.
[ signing_policy ]
organizationName = supplied
commonName       = optional

# Used to sign node certificates.
[ signing_node_req ]
keyUsage         = critical,digitalSignature,keyEncipherment
extendedKeyUsage = serverAuth,clientAuth

# Used to sign client certificates.
[ signing_client_req ]
keyUsage         = critical,digitalSignature,keyEncipherment
extendedKeyUsage = clientAuth
"""

_node_config_tmpl = """
# OpenSSL node configuration file
[ req ]
prompt=no
distinguished_name = distinguished_name
req_extensions = extensions

[ distinguished_name ]
organizationName = Vectorized
{common_name}

[ extensions ]
subjectAltName = critical,DNS:{host}
"""

_CA = collections.namedtuple("CA", ["cfg", "key", "crt"])
_Cert = collections.namedtuple("Cert", ["cfg", "key", "crt"])


class TLSCertManager:
    def __init__(self):
        self.dir = tempfile.TemporaryDirectory()
        self.ca = self._create_ca()
        self.certs = {}

    def _with_dir(self, name):
        return os.path.join(self.dir.name, name)

    def _exec(self, cmd):
        subprocess.check_output(cmd.split(), cwd=self.dir.name)

    def _create_ca(self):
        cfg = self._with_dir("ca.conf")
        key = self._with_dir("ca.key")
        crt = self._with_dir("ca.crt")
        idx = self._with_dir("index.txt")
        srl = self._with_dir("serial.txt")

        with open(f"{cfg}", "w") as f:
            f.write(_ca_config_tmpl.format(dir=self.dir.name))

        self._exec(f"openssl genrsa -out {key} 2048")

        self._exec(f"openssl req -new -x509 -config {cfg} "
                   f"-key {key} -out {crt} -days 365 -batch")

        if os.path.exists(idx): os.remove(idx)
        if os.path.exists(srl): os.remove(srl)
        pathlib.Path(idx).touch()
        with open(srl, "w") as f:
            f.writelines(["01"])

        return _CA(cfg, key, crt)

    def create_cert(self,
                    host: str,
                    *,
                    common_name: typing.Optional[str] = None,
                    name: typing.Optional[str] = None):
        name = name or host
        assert name not in self.certs, f"cert '{name}' already exists"

        cfg = self._with_dir(f"{name}.conf")
        key = self._with_dir(f"{name}.key")
        csr = self._with_dir(f"{name}.csr")
        crt = self._with_dir(f"{name}.crt")

        with open(cfg, "w") as f:
            if common_name is None:
                common_name = ""
            else:
                common_name = f"commonName = {common_name}"
            f.write(
                _node_config_tmpl.format(host=host, common_name=common_name))

        self._exec(f"openssl genrsa -out {key} 2048")

        self._exec(f"openssl req -new -config {cfg} "
                   f"-key {key} -out {csr} -batch")

        self._exec(f"openssl ca -config {self.ca.cfg} -keyfile "
                   f"{self.ca.key} -cert {self.ca.crt} -policy signing_policy "
                   f"-extensions signing_node_req -in {csr} -out {crt} "
                   f"-outdir {self.dir.name} -batch")

        cert = _Cert(cfg, key, crt)
        self.certs[name] = cert
        return cert
