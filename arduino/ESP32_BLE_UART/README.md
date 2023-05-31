Build and upload to ESP32 with Arduino IDe

Use mobile app to interact with devices.

IOS: https://apps.apple.com/at/app/adafruit-bluefruit-le-connect/id830125974

Android: https://play.google.com/store/search?q=bluefruit+connect&c=apps&hl=en_US&gl=US


```mermaid
sequenceDiagram
    participant device
    participant client

    client->>client: start scan
    device->>client: broadcasts GATT
    client->>client: stop scan
    client->>client: user selects device from UI
    client->>device: connection requested
    device->>client: connection accepted
    client->>client: write to TX charachteristic (<20bytes)
    client->>device: BLE notifies
    device->>device: read RX buffer
    device->>device: do stuff
    device->>device: write TX buffer
    device->>client: BLE notify
    client->>client: read RX buffer

```