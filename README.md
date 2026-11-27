# AWC Radio — M5Stack Core2 v1.3

Experimental car radio firmware for the **original ESP32 Core2 / Core2 v1.3**, streaming from the [AWC Radio Docker web server](https://github.com/bkchaw76/AWC_from_RPF9_Extractor) over Wi-Fi and transmitting to a Bluetooth Classic A2DP head unit.

**Status: early v0.1 prototype. Hardware playback and Wi-Fi/Bluetooth coexistence are not yet verified.**

## What this branch implements

- Core2 display: basic station name, song title, artist and status.
- Touch controls on bottom of screen: previous station / pause-play / next station.
- GET `/api/stations`, GET `/api/stations/{name}/now`, and continuous MP3 GET `/api/stations/{name}/stream`.
- Helix MP3 software decoder (expects **44.1 kHz stereo** from the existing server), PSRAM PCM ring buffer and Bluetooth A2DP source.
- Bluetooth device name configuration, auto-reconnect, Wi-Fi reconnect, stream retry.
- PlatformIO project and GitHub Actions firmware build artifact.

The N100 Docker server continues to handle the original DJ voices, advertisements, news, mixing, and scheduling.

## Local configuration and flashing

1. Install [VS Code](https://code.visualstudio.com/) and [PlatformIO](https://platformio.org/).
2. Clone the `feature/core2-radio-v01` branch of this repository.
3. Copy `include/config.example.h` to `include/config.h`.
4. Edit the configuration, including your **2.4 GHz Wi-Fi** network, Bluetooth head unit's exact discoverable name, and AWC Radio server URL. For the home LAN, the default sample is `http://192.168.8.8:8088`.
5. Make the car head unit discoverable (prefer testing with a spare Bluetooth speaker first).
6. Connect the Core2 over USB-C and run `pio run -e m5stack-core2 -t upload`.
7. Check logs with `pio device monitor -b 115200`.

`include/config.h` is gitignored. **Never commit your hotspot password.**

## GitHub Actions

[Actions → Core2 firmware build](../../actions/workflows/firmware.yml) compiles the source and uploads `firmware.bin`, bootloader and partitions as artifacts. It uses **dummy example credentials** so the public download is a compilation artifact, **not a configured ready-to-run device firmware**. You still need a local firmware build with your own settings to connect to your hotspot and car.

## Phone hotspot and security

The device must independently reach the N100 server. **An iPhone hotspot does not automatically relay the iPhone's WireGuard VPN to hotspot clients.** The private `192.168.8.8` address normally works only on your home network, not in the car. Configure secure remote access before field testing. Do not expose the unauthenticated AWC web radio server directly to the public Internet.

## Limitations of v0.1

- Initial touchscreen UI only; original station logos, favourites, resume settings and Next up are planned.
- Audio latency, dropout handling and battery runtime are unmeasured.
- The PCM queue is currently 128 KiB (approximately 0.74 sec at 44.1 kHz stereo); longer compressed-stream buffering is a follow-up.
- Car head unit pairing and simultaneous Wi-Fi/A2DP performance require real hardware testing.
- Streaming HTTP connectivity, station-change recovery and ESP32 memory margins have not been field verified.
- Device firmware expects MP3 stereo 44.1 kHz output from your existing v0.4 Docker server.

## Roadmap

v0.2: Bluetooth and stream stability fixes, station artwork and a better touchscreen.
v0.3: favourites, Next up, metadata, recovery.
v0.4: ignition-power behaviour and reliable mobile use.

## Licensing

This repo is MIT, but its dependencies retain their respective licences. ESP32-A2DP and Arduino Audio Tools have GPL-related licensing obligations; Helix code has its own upstream terms. Review compliance before distributing binaries. No game audio is included.
