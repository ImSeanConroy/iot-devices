# Device Certificates (Needed in Firmware)
output "device_certificates" {
  value       = { for key, cert in aws_iot_certificate.device_certificate : key => cert.certificate_pem }
  description = "Device certificates keyed by device role name"
  sensitive   = true
}

# Device Private Keys (Needed in Firmware)
output "device_private_keys" {
  value       = { for key, cert in aws_iot_certificate.device_certificate : key => cert.private_key }
  description = "Device private keys keyed by device role name"
  sensitive   = true
}

# Device Client IDs
output "device_client_ids" {
  value       = { for key, device in local.iot_devices : key => device.client_id }
  description = "Device client IDs keyed by device role name"
}

# AWS Root CA URL (Needed in Firmware)
output "root_ca_url" {
  value       = "https://www.amazontrust.com/repository/AmazonRootCA1.pem"
  description = "URL to get AWS Root CA (AWS Root CA needed in Firmware)"
}
