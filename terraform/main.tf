# Configure the AWS Provider
provider "aws" {
  version = "~> 2.0"
  region  = "us-east-2"
}

data "aws_ami" "ubuntu" {
  most_recent = true

  filter {
    name   = "name"
    values = ["ubuntu/images/hvm-ssd/ubuntu-bionic-18.04-amd64-server-20180426.2"]
  }

  filter {
    name   = "virtualization-type"
    values = ["hvm"]
  }

  owners = ["099720109477"] # Canonical
}

resource "aws_instance" "web" {
  ami           = "${data.aws_ami.ubuntu.id}"
  instance_type = "t2.micro"
  key_name      = "MyKeyPair"
  vpc_security_group_ids = [aws_security_group.allow_server.id]
}

resource "null_resource" "scp" {
  provisioner "local-exec" {
    command = "sleep 15 && scp -i ~/.ssh/MyKeyPair.pem ../udp_server.cpp ubuntu@${aws_instance.web.public_ip}:~/ && ssh -i ~/.ssh/MyKeyPair.pem ubuntu@${aws_instance.web.public_ip} 'bash -s' < build_server.sh"
  }
}

output "public_ip" {
    value = "${aws_instance.web.public_ip}"
}

resource "aws_security_group" "allow_server" {
  name        = "allow_server"
  vpc_id      = "vpc-b86590d3"

  ingress {
    # TLS (change to whatever ports you need)
    from_port   = 22
    to_port     = 22
    protocol    = "tcp"
    # Please restrict your ingress to only necessary IPs and ports.
    # Opening to 0.0.0.0/0 can lead to security vulnerabilities.
    cidr_blocks = ["0.0.0.0/0"]
  }

  ingress {
      from_port = 32000
      to_port = 32000
      protocol = "udp"
      cidr_blocks = ["0.0.0.0/0"]
  }

  egress {
    from_port       = 0
    to_port         = 0
    protocol        = "-1"
    cidr_blocks     = ["0.0.0.0/0"]
  }
}

