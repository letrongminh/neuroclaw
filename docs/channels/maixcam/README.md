> Back to [README](../../../README.md)

# MaixCam

MaixCam is a dedicated channel for connecting to Sipeed MaixCAM and MaixCAM2 AI camera devices. It uses TCP sockets for bidirectional communication and supports edge AI deployment scenarios.

## Configuration

```json
{
  "channel_list": {
    "maixcam": {
      "enabled": true,
      "type": "maixcam",
      "host": "0.0.0.0",
      "port": 18790,
      "allow_from": []
    }
  }
}
```

| Field      | Type   | Required | Description                                                      |
| ---------- | ------ | -------- | ---------------------------------------------------------------- |
| enabled    | bool   | Yes      | Whether to enable the MaixCam channel                            |
| host       | string | Yes      | TCP server listening address                                     |
| port       | int    | Yes      | TCP server listening port (default `18790`, the same as the gateway's HTTP port; set a different one if both run on the same host) |
| allow_from | array  | No       | Allowlist of device IDs; empty lets anyone in (the gateway logs a warning); set it, or use `["*"]` to allow everyone on purpose     |

## Use Cases

The MaixCam channel enables NeuroClaw to act as an AI backend for edge devices:

- **Smart Surveillance**: MaixCAM sends image frames; NeuroClaw analyzes them using vision models
- **IoT Control**: Devices send sensor data; NeuroClaw coordinates responses
- **Offline AI**: Deploy NeuroClaw on a local network for low-latency inference
