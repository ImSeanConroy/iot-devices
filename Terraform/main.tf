/*
Name: IaC Buildout for IoT Devices
Description: AWS Infrastruture Buildout
Project: IoT Devices
*/

# Configure the AWS Provider
provider "aws" {
  default_tags {
    tags = {
      Project     = "IoT_Devices"
      Provisioned = "Terraform"
      Environment = "Development"
    }
  }
}

# AWS Account ID
data "aws_caller_identity" "current" {}

# AWS Region
data "aws_region" "current" {}

# === IoT Cloud Configuration ===

locals {
  iot_devices = {
    generic_subscriber = {
      thing_name = "generic-subscriber"
      client_id  = "generic-subscriber"
      role       = "subscriber"
      topic      = "iot-devices/generic"
    }
    generic_publisher = {
      thing_name = "generic-publisher"
      client_id  = "generic-publisher"
      role       = "publisher"
      topic      = "iot-devices/generic"
    }
  }

  publishers  = { for key, device in local.iot_devices : key => device if device.role == "publisher" }
  subscribers = { for key, device in local.iot_devices : key => device if device.role == "subscriber" }
}

# IoT Things
resource "aws_iot_thing" "device" {
  for_each = local.iot_devices

  name = each.value.thing_name
  attributes = {
    Project = "IoT_Devices"
    Model   = "Development-ESP32"
    Role    = each.value.role
  }
}

# IoT Certificates
resource "aws_iot_certificate" "device_certificate" {
  for_each = local.iot_devices
  active   = true
}

# Publisher Policies (one per publisher)
resource "aws_iot_policy" "publisher_policy" {
  for_each = local.publishers

  name = "IoT-Devices-${replace(each.key, "_", "-")}-policy"
  policy = jsonencode({
    Version = "2012-10-17",
    Statement = [
      {
        Effect   = "Allow",
        Action   = "iot:Connect",
        Resource = "arn:aws:iot:${data.aws_region.current.name}:${data.aws_caller_identity.current.account_id}:client/${each.value.client_id}"
      },
      {
        Effect   = "Allow",
        Action   = "iot:Publish",
        Resource = "arn:aws:iot:${data.aws_region.current.name}:${data.aws_caller_identity.current.account_id}:topic/${each.value.topic}"
      }
    ]
  })
}

# Subscriber Policies (one per subscriber)
resource "aws_iot_policy" "subscriber_policy" {
  for_each = local.subscribers

  name = "IoT-Devices-${replace(each.key, "_", "-")}-policy"
  policy = jsonencode({
    Version = "2012-10-17",
    Statement = [
      {
        Effect   = "Allow",
        Action   = "iot:Connect",
        Resource = "arn:aws:iot:${data.aws_region.current.name}:${data.aws_caller_identity.current.account_id}:client/${each.value.client_id}"
      },
      {
        Effect = "Allow",
        Action = "iot:Subscribe",
        Resource = [
          for publisher in local.publishers :
          "arn:aws:iot:${data.aws_region.current.name}:${data.aws_caller_identity.current.account_id}:topicfilter/${publisher.topic}"
        ]
      },
      {
        Effect = "Allow",
        Action = "iot:Receive",
        Resource = [
          for publisher in local.publishers :
          "arn:aws:iot:${data.aws_region.current.name}:${data.aws_caller_identity.current.account_id}:topic/${publisher.topic}"
        ]
      }
    ]
  })
}

# Attach Publisher Policies to Publisher Certificates
resource "aws_iot_policy_attachment" "publisher_policy_certificate_attachment" {
  for_each = local.publishers

  policy = aws_iot_policy.publisher_policy[each.key].name
  target = aws_iot_certificate.device_certificate[each.key].arn
}

# Attach Subscriber Policy to Subscriber Certificates
resource "aws_iot_policy_attachment" "subscriber_policy_certificate_attachment" {
  for_each = local.subscribers

  policy = aws_iot_policy.subscriber_policy[each.key].name
  target = aws_iot_certificate.device_certificate[each.key].arn
}

# Attach IoT Things and IoT Certificates
resource "aws_iot_thing_principal_attachment" "device_certificate_attachment" {
  for_each = local.iot_devices

  principal = aws_iot_certificate.device_certificate[each.key].arn
  thing     = aws_iot_thing.device[each.key].name
}
