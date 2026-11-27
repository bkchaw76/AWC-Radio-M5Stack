#pragma once
// Copy this file to include/config.h and edit it. config.h is gitignored.
// This example is intentionally public: never commit real Wi-Fi credentials.
#define WIFI_SSID "YOUR_IPHONE_HOTSPOT"
#define WIFI_PASSWORD "YOUR_HOTSPOT_PASSWORD"
#define BT_SPEAKER_NAME "YOUR_CAR_BLUETOOTH_NAME"
// Must be reachable directly by the ESP32. Your iPhone's VPN is NOT shared
// with hotspot clients. For initial testing use the N100 LAN URL below.
#define RADIO_SERVER_URL "http://192.168.8.8:8088"
