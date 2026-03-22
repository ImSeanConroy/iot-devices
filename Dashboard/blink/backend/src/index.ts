// @ts-ignore
import { device as AwsIotDevice } from 'aws-iot-device-sdk';
import express, { Request, Response } from 'express';
import http from 'http';
import path from 'path';
import { Server } from 'socket.io';

interface TelemetryData {
  [key: string]: any;
}

const app = express();
const server = http.createServer(app);
const io = new Server(server, { cors: { origin: "*" } });

const latestData: Record<string, TelemetryData> = {};

const device = AwsIotDevice({
  keyPath: path.resolve(__dirname, "certs/key.pem"),
  certPath: path.resolve(__dirname, "certs/cert.pem"),
  caPath: path.resolve(__dirname, "certs/rootCA.pem"),
  clientId: 'blink-dashboard',
  host: 'a1i7qjvv2mo489-ats.iot.eu-central-1.amazonaws.com'
});

device.on('connect', () => {
  console.log('Connected to IoT Core');
  device.subscribe('iot-devices/blink');
});

device.on('message', (topic: string, payload: Buffer) => {
  try {
    const data: TelemetryData = JSON.parse(payload.toString());
    const deviceId = topic.split('/')[1];
    
    latestData[deviceId] = data;
    console.log(data)

    io.emit('device-update', { deviceId, data });

  } catch (err) {
    console.error('Invalid JSON:', err);
  }
});

app.get('/devices', (_req: Request, res: Response) => {
  res.json(latestData);
});

server.listen(3000, () => console.log('Server running on port 3000'));