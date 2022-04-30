import subprocess
import shutil
import tempfile

class Security:
    """
    TODO
    - use test prefix or dir for tmp files. things like serial file are
      generated relative to those files not in the current working dir.
    """

    def generate_ca(self):
		# run in some other dir
        server="localhost"
        domain="localhost"
        country="SE"
        state="Stockholm"
        locality=state
        self.common=server
        self.subject=f"/C={country}/ST={state}/L={locality}/O={domain}/OU={domain}/CN={self.common}"
        self.client1=f"/C={country}/ST={state}/L={locality}/O={domain}/OU={domain}/CN=client1.org"
        self.mtls_ca_key = tempfile.NamedTemporaryFile(delete=False)
        self.mtls_ca_crt = tempfile.NamedTemporaryFile(delete=False, prefix='ca-crt')
        self._run(f"openssl ecparam -name prime256v1 -genkey -noout -out {self.mtls_ca_key.name}")
        self._run(
            f"openssl req -new -x509 -sha256 -key {self.mtls_ca_key.name}".split() +
            f"-out {self.mtls_ca_crt.name} -subj".split() + [self.subject] +
            ["-addext", f"subjectAltName = DNS:{self.common}"])

    def generate_server(self):
        extfile = tempfile.NamedTemporaryFile(delete=False)
        extfile.write(f"subjectAltName=DNS:{self.common}".encode())
        extfile.flush()

        self.server_key = tempfile.NamedTemporaryFile(delete=False, prefix='server-key')
        server_csr = tempfile.NamedTemporaryFile(delete=False)
        self.server_crt = tempfile.NamedTemporaryFile(delete=False, prefix='server-crt')
        self._run(f"openssl ecparam -name prime256v1 -genkey -noout -out {self.server_key.name}")
        self._run(f"openssl req -new -sha256 -key {self.server_key.name} -out {server_csr.name}".split() +
             ["-subj", self.subject, "-addext", f"subjectAltName = DNS:{self.common}"])
        self._run(f"openssl x509 -req -in {server_csr.name} -CA {self.mtls_ca_crt.name} -CAkey {self.mtls_ca_key.name}".split() +
            f"-CAcreateserial -out {self.server_crt.name} -days 1000 -sha256 -extfile {extfile.name}".split())

    def generate_client(self):
        self.client_key = tempfile.NamedTemporaryFile(delete=False, prefix='client-key')
        csr = tempfile.NamedTemporaryFile(delete=False)
        self.client_crt = tempfile.NamedTemporaryFile(delete=False, prefix='client-crt')
        self._run(f"openssl ecparam -name prime256v1 -genkey -noout -out {self.client_key.name}")
        self._run(f"openssl req -new -sha256 -key {self.client_key.name} -out {csr.name}".split() +
             ["-subj", self.client1, "-addext", f"subjectAltName = DNS:{self.common}"])
        self._run(f"openssl x509 -req -in {csr.name} -CA {self.mtls_ca_crt.name} -CAkey {self.mtls_ca_key.name}".split() +
            f"-CAcreateserial -out {self.client_crt.name} -days 1000 -sha256".split())

    def generate_cert(self):
        self.generate_server()
        self.generate_client()
        pass

    def _run(self, cmd):
        if not isinstance(cmd, list):
            cmd = cmd.split()
        subprocess.check_output(cmd)

s = Security()
s.generate_ca()
s.generate_cert()

shutil.move(s.mtls_ca_crt.name, 'ca.crt')
shutil.move(s.server_key.name, 'server.key')
shutil.move(s.server_crt.name, 'server.crt')
shutil.move(s.client_key.name, 'client.key')
shutil.move(s.client_crt.name, 'client.crt')
