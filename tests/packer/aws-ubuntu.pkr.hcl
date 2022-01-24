packer {
  required_plugins {
    amazon = {
      version = ">= 0.0.2"
      source  = "github.com/hashicorp/amazon"
    }
  }
}

source "amazon-ebs" "ubuntu" {
  ami_name      = "redpanda-testing"
  instance_type = "i3en.large"
  region        = "us-west-2"
  source_ami_filter {
    filters = {
      name                = "ubuntu/images/*ubuntu-impish-21.10-amd64-server-*"
      root-device-type    = "ebs"
      virtualization-type = "hvm"
    }
    most_recent = true
    owners      = ["099720109477"]
  }
  ssh_username = "ubuntu"
}

build {
  name = "redpanda-testing"
  sources = [
    "source.amazon-ebs.ubuntu"
  ]
  provisioner "shell" {
    inline = ["cloud-init status --wait"]
  }
  #provisioner "shell-local" {
  #  inline = ["echo '${build.SSHPrivateKey}'", "echo '${build.SSHPrivateKey}' > /tmp/packer-aws.pem"]
  #}
  #provisioner "breakpoint" {}
  provisioner "shell" {
    execute_command = "chmod +x {{ .Path }}; sudo {{ .Vars }} {{ .Path }}"
    scripts = [
      "install-deps.sh",
    ]
  }
}
