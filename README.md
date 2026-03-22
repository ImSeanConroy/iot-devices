# IoT Devices

**A monorepo for AWS IoT experiments and hardware prototypes** built around ESP32 boards, MQTT messaging, custom PCB designs, and a local dashboard for visualizing device events.

This repository contains end-to-end building blocks for connected devices:

- ESP32 firmware publishers/subscribers
- AWS IoT Core infrastructure defined with Terraform
- KiCad PCB projects for multiple device types
- A Blink dashboard (Node.js backend + React frontend)

## Table of Contents

- [Monorepo Structure](#monorepo-structure)
- [Getting Started](#getting-started)
  - [1. Clone the Repository](#1-clone-the-repository)
  - [2. Provision AWS IoT Infrastructure](#2-provision-aws-iot-infrastructure) 
  - [3. Configure and Flash Firmware](#3-configure-and-flash-firmware)
  - [4. Run the Blink Dashboard](#4-run-the-blink-dashboard)
  - [5. Open PCB Projects (Optional)](#5-open-pcb-projects-optional)
- [Device Topics and Roles](#device-topics-and-roles)
- [Hardware Notes](#hardware-notes)
- [Contributing](#contributing)
- [License](#license)
- [Support](#support)
- [Acknowledgments](#acknowledgments)

## Monorepo Structure

- `Firmware/` - Arduino firmware for Blink, Generic, Door Sensor, GPS, and Temperature device variants.
- `Terraform/` - Infrastructure as code for AWS IoT Things, certificates, and policies.
- `Dashboard/` - Local dashboards built with node and express.
- `PCB/` - KiCad projects for Blink, Door Sensor, GPS, and Temperature hardware.
- `Case/` - Mechanical/case assets.

## Getting Started

Before starting, install:

| Tool | Purpose | Link |
| --- | --- | --- |
| Node.js (>=18) | Dashboard backend/frontend runtime | [Download](https://nodejs.org/) |
| npm | JavaScript package manager | [Download](https://www.npmjs.com/) |
| Terraform | AWS IoT infrastructure provisioning | [Install Guide](https://developer.hashicorp.com/terraform/install) |
| AWS CLI | AWS authentication and account setup | [AWS CLI](https://aws.amazon.com/cli/) |
| Arduino IDE | Build/upload firmware | [Arduino IDE](https://www.arduino.cc/en/software) |
| KiCad | Open PCB projects | [KiCad](https://www.kicad.org/) |

### 1. Clone the Repository

```bash
git clone git@github.com:ImSeanConroy/iot-devices.git
cd iot-devices
```

### 2. Provision AWS IoT Infrastructure

1. Configure AWS credentials (or run `aws configure`).

```bash
export AWS_ACCESS_KEY_ID=<your-access-key>
export AWS_SECRET_ACCESS_KEY=<your-secret-key>
export AWS_REGION=<your-region>
```

2. Deploy infrastructure.

```bash
cd Terraform
terraform init
terraform apply
```

3. Retrieve generated values.

```bash
terraform output root_ca_url
terraform output device_client_ids
terraform output device_certificates
terraform output device_private_keys
```

### 3. Configure and Flash Firmware

1. Choose a firmware target, for example:
   - `Firmware/BlinkPublisher/BlinkPublisher.ino`
   - `Firmware/BlinkSubscriber/BlinkSubscriber.ino`
   - `Firmware/GenericPublisher/GenericPublisher.ino`
   - `Firmware/GenericSubscriber/GenericSubscriber.ino`

2. In the selected folder, copy `secrets.h.template` to `secrets.h`.

3. Update `secrets.h` with:
   - `WIFI_SSID` and `WIFI_PASSWORD`
   - `AWS_IOT_ENDPOINT`
   - Root CA certificate
   - Device certificate and private key

4. Compile and upload from Arduino IDE.

### 4. Run the Blink Dashboard

The Blink dashboard lives in `Dashboard/blink/`.

1. Prepare backend cert files:

```bash
cd Dashboard/blink/backend/src/certs
cp cert.pem.example cert.pem
cp key.pem.example key.pem
cp rootCA.pem.example rootCA.pem
```

2. Start backend (default `http://localhost:3000`).

```bash
cd ../../
npm install
npm run dev
```

3. Start frontend (Vite, usually `http://localhost:5173`).

```bash
cd ../frontend
npm install
npm run dev
```

### 5. Open PCB Projects (Optional)

Open any KiCad project in `PCB/`, for example:

- `PCB/IoT_Devices_Blink/IoT_Devices_Blink.kicad_pro`
- `PCB/-IoT_Devices_Door_Sensor/`
- `PCB/-IoT_Devices_GPS/`
- `PCB/-IoT_Devices_Temperature_Sensor/`

## Device Topics and Roles

Terraform currently defines devices in `Terraform/main.tf`.

- Active by default:
  - `door_sensor_publisher` -> topic `iot-devices/door-sensor`
- Present but commented out:
  - Blink publisher/subscriber/dashboard
  - Generic publisher/subscriber
  - Temperature publisher
  - GPS publisher

To enable additional roles, uncomment the corresponding entries in `local.iot_devices` and re-run `terraform apply`.

## Hardware Notes

- Firmware targets Seeed Studio XIAO ESP32C3-class boards.
- Blink subscriber firmware expects a WS2812-compatible LED connected to the configured pin.
- PCB designs are maintained in KiCad and can be exported for fabrication.

## Contributing

Contributions are welcome. Please open an issue or fork the repository, create a new branch (`feature/your-feature-name`) and submit a pull request for any enhancements or bug fixes.

## License

This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for information.

## Support

If you are having problems, please let me know by [raising a new issue](https://github.com/ImSeanConroy/iot-devices/issues/new/choose).

## Acknowledgments

This project was made possible thanks to the following resources:

- [AWS re:Post](https://repost.aws/questions/QUxlg-arOrTcaxFAdNEj7hqA/lambda-publish-a-mqtt-message-to-iot-core-inside-a-lambda-function-with-node-and-sdk-3) - Publish a MQTT Message to IOT Core inside a Lambda function with Node and SDK 3
- [AWS Samples](https://github.com/aws-samples/aws-iot-esp32-arduino-examples) - Several Arduino examples for AWS IoT projects using ESP32.

