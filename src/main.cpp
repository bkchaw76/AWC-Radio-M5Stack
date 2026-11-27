#include <Arduino.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <Preferences.h>
#include <WebServer.h>
#include <ArduinoJson.h>
#include <M5Unified.h>
#include <BluetoothA2DPSource.h>
#include "MP3DecoderHelix.h"
// Local credentials override the public example (which never contains secrets).
#if __has_include("../include/config.h")
#include "../include/config.h"
#endif

// Safe defaults for public CI, independent of header search behavior.
#ifndef WIFI_SSID
#define WIFI_SSID "YOUR_IPHONE_HOTSPOT"
#endif
#ifndef WIFI_PASSWORD
#define WIFI_PASSWORD "YOUR_HOTSPOT_PASSWORD"
#endif
#ifndef BT_SPEAKER_NAME
#define BT_SPEAKER_NAME "YOUR_CAR_BLUETOOTH_NAME"
#endif
#ifndef RADIO_SERVER_URL
#define RADIO_SERVER_URL "http://192.168.8.8:8088"
#endif

using libhelix::MP3DecoderHelix;
static String wifiName, wifiPassword, btName, radioURL;
static Preferences prefs;
static WebServer setupServer(80);

static void keyBox(int x,int y,int w,int h,const String &label) {
  auto &d=M5.Display;
  d.fillRoundRect(x,y,w-2,h-2,4,TFT_DARKGREY);
  d.setTextColor(TFT_WHITE,TFT_DARKGREY);
  d.setTextSize(1);
  d.setCursor(x+4,y+h/2-4);
  d.print(label);
}
static void heading(const String &label) {
  M5.Display.fillScreen(TFT_BLACK);
  M5.Display.setTextColor(TFT_CYAN,TFT_BLACK);
  M5.Display.setTextSize(2);
  M5.Display.setCursor(8,8);
  M5.Display.print(label);
}
static void tap(int &x,int &y) {
  while(true) {
    M5.update();
    if(M5.Touch.getCount()) {
      auto t=M5.Touch.getDetail(0);
      if(t.wasPressed()) { x=t.x; y=t.y; return; }
    }
    delay(15);
  }
}
static String keyboard(const String &label,String value,bool secret=false) {
  int mode=0;
  const char *rows[3][4]={{"1234567890","qwertyuiop","asdfghjkl","zxcvbnm.-/"},{"1234567890","QWERTYUIOP","ASDFGHJKL","ZXCVBNM.-/"},{"!@#$%^&*()","_+=:;?,~|","[]{}<>\\`","'\\" .-/"}};
  while(true) {
    heading(label);
    String shown=value;
    if(secret) { shown=""; for(size_t i=0;i<value.length();++i) shown+="*"; }
    if(shown.length()>48) shown=shown.substring(shown.length()-48);
    M5.Display.setTextSize(1); M5.Display.setCursor(8,38); M5.Display.print(shown);
    for(int r=0;r<4;r++) for(int c=0;c<(int)strlen(rows[mode][r]);c++) keyBox(c*32,60+r*34,32,34,String(rows[mode][r][c]));
    keyBox(0,200,64,40,"ABC/#"); keyBox(64,200,80,40,"SPACE"); keyBox(144,200,64,40,"DEL"); keyBox(208,200,112,40,"OK");
    int x,y; tap(x,y);
    if(y>=200) {
      if(x<64) mode=(mode+1)%3;
      else if(x<144) { if(value.length()<128) value+=' '; }
      else if(x<208) { if(value.length()) value.remove(value.length()-1); }
      else return value;
    } else if(y>=60 && y<196) {
      int r=(y-60)/34,c=x/32;
      if(c>=0 && c<(int)strlen(rows[mode][r]) && value.length()<128) value+=rows[mode][r][c];
    }
  }
}
static String chooseWifi() {
  heading("Scanning Wi-Fi...");
  WiFi.mode(WIFI_STA);
  int n=WiFi.scanNetworks();
  int page=0;
  while(true) {
    heading("Select Wi-Fi");
    for(int i=0;i<4 && page+i<n;i++) keyBox(4,35+i*40,312,40,WiFi.SSID(page+i));
    keyBox(0,200,70,40,"PREV"); keyBox(70,200,70,40,"NEXT"); keyBox(140,200,180,40,"MANUAL SSID");
    int x,y; tap(x,y);
    if(y>=200) {
      if(x<70) page=max(0,page-4);
      else if(x<140) { if(page+4<n) page+=4; }
      else { WiFi.scanDelete(); return keyboard("Wi-Fi SSID",""); }
    } else if(y>=35 && y<195 && page+(y-35)/40<n) {
      String ssid=WiFi.SSID(page+(y-35)/40); WiFi.scanDelete(); return ssid;
    }
  }
}
static bool lcdSetup() {
  while(true) {
    heading("AWC Radio Settings");
    keyBox(4,35,312,38,"Wi-Fi: "+wifiName);
    keyBox(4,75,312,38,"Bluetooth: "+btName);
    keyBox(4,115,312,38,"Server: "+radioURL);
    keyBox(4,155,312,38,"Save and restart");
    keyBox(4,195,312,42,"Phone setup (optional)");
    int x,y; tap(x,y);
    if(y>=35 && y<75) { wifiName=chooseWifi(); wifiPassword=keyboard("Wi-Fi password","",true); }
    else if(y>=75 && y<115) btName=keyboard("Bluetooth name",btName=="YOUR_CAR_BLUETOOTH_NAME"?"":btName);
    else if(y>=115 && y<155) radioURL=keyboard("Radio server URL",radioURL);
    else if(y>=155 && y<195) {
      if(!wifiName.length() || wifiName=="YOUR_IPHONE_HOTSPOT" || !btName.length() || btName=="YOUR_CAR_BLUETOOTH_NAME" || !radioURL.startsWith("http://")) { heading("Check settings"); delay(1200); continue; }
      prefs.putString("ssid",wifiName); prefs.putString("password",wifiPassword); prefs.putString("bt",btName); prefs.putString("url",radioURL);
      prefs.end(); ESP.restart(); return true;
    } else if(y>=195) return false;
  }
}
static void configureDevice() {
  prefs.begin("awc-radio", false);
  wifiName = prefs.getString("ssid", WIFI_SSID);
  wifiPassword = prefs.getString("password", WIFI_PASSWORD);
  btName = prefs.getString("bt", BT_SPEAKER_NAME);
  radioURL = prefs.getString("url", RADIO_SERVER_URL);
  bool forceSetup=prefs.getBool("setup",false); prefs.remove("setup");
  // Hold the touchscreen during startup to reopen setup.
  delay(600);
  M5.update();
  if (wifiName != "YOUR_IPHONE_HOTSPOT" && btName != "YOUR_CAR_BLUETOOTH_NAME" && !M5.Touch.getCount() && !forceSetup) {
    prefs.end();
    return;
  }
  if(lcdSetup()) return;
  WiFi.mode(WIFI_AP);
  String apPassword = String((uint32_t)esp_random(), HEX);
  while (apPassword.length() < 8) apPassword = "0" + apPassword;
  WiFi.softAP("AWC-Radio-Setup", apPassword.c_str());
  M5.Display.fillScreen(TFT_BLACK);
  M5.Display.setTextSize(2);
  M5.Display.setCursor(8, 10);
  M5.Display.println("AWC Radio Setup");
  M5.Display.println("AWC-Radio-Setup");
  M5.Display.println("Password:");
  M5.Display.println(apPassword);
  M5.Display.println("192.168.4.1");
  setupServer.on("/", HTTP_GET, []() {
    setupServer.send(200, "text/html", R"HTML(<!doctype html><meta name="viewport" content="width=device-width,initial-scale=1"><h2>AWC Radio setup</h2><form method="post" action="/save"><p>Wi-Fi SSID<br><input name="ssid" required></p><p>Wi-Fi password<br><input name="password" type="password"></p><p>Bluetooth head unit name<br><input name="bt" required></p><p>Radio server URL<br><input name="url" type="url" value="http://192.168.8.8:8088" required></p><button>Save and restart</button></form>)HTML");
  });
  setupServer.on("/save", HTTP_POST, []() {
    String url = setupServer.arg("url");
    if (!setupServer.arg("ssid").length() || !setupServer.arg("bt").length() || !url.startsWith("http://")) {
      setupServer.send(400, "text/plain", "SSID, Bluetooth name and http:// server URL required");
      return;
    }
    prefs.putString("ssid", setupServer.arg("ssid"));
    prefs.putString("password", setupServer.arg("password"));
    prefs.putString("bt", setupServer.arg("bt"));
    prefs.putString("url", url);
    setupServer.send(200, "text/plain", "Saved. Restarting...");
    delay(500);
    prefs.end();
    ESP.restart();
  });
  setupServer.begin();
  while (true) { setupServer.handleClient(); delay(10); }
}


static BluetoothA2DPSource bluetooth;
static SemaphoreHandle_t audioMutex;
static Frame *pcmRing = nullptr;
static constexpr size_t RING_FRAMES = 32768; // 128 KB PSRAM, ~0.74 s PCM
static size_t ringRead = 0, ringWrite = 0, ringCount = 0;
static volatile bool playing = false;
static volatile uint32_t generation = 1;
static volatile uint32_t lastAudioMillis = 0;
static String stationNames[40];
static int stationCount = 0, stationIndex = 0;
static String title = "", artist = "";
static unsigned long lastMetadata = 0, lastScreen = 0;
static bool streamOK = false;

static String encoded(const String &s) {
  const char hex[] = "0123456789ABCDEF";
  String out;
  for (size_t i = 0; i < s.length(); ++i) {
    unsigned char c = (unsigned char)s[i];
    if (isalnum(c) || c == '-' || c == '_' || c == '.' || c == '~') out += (char)c;
    else { out += '%'; out += hex[c >> 4]; out += hex[c & 15]; }
  }
  return out;
}

static String endpoint(const String &path) {
  String base = radioURL;
  while (base.endsWith("/")) base.remove(base.length() - 1);
  return base + path;
}

static void clearAudio() {
  if (!audioMutex) return;
  if (xSemaphoreTake(audioMutex, pdMS_TO_TICKS(100))) {
    ringRead = ringWrite = ringCount = 0;
    xSemaphoreGive(audioMutex);
  }
}

// Called by the Bluetooth stack. Never wait on Wi-Fi or do allocations here.
static int32_t bluetoothFrames(Frame *dst, int32_t requested) {
  if (requested <= 0) return 0;
  memset(dst, 0, (size_t)requested * sizeof(Frame));
  if (!playing || !pcmRing) return requested;
  if (xSemaphoreTake(audioMutex, 0) != pdTRUE) return requested;
  for (int32_t i = 0; i < requested && ringCount; ++i) {
    dst[i] = pcmRing[ringRead];
    ringRead = (ringRead + 1) % RING_FRAMES;
    --ringCount;
  }
  xSemaphoreGive(audioMutex);
  return requested;
}

static void decoded(MP3FrameInfo &info, short *samples, size_t count, void *) {
  if (!playing || !pcmRing || !samples || info.samprate != 44100 ||
      info.nChans != 2) return;
  // Helix callback length is count of 16-bit PCM samples, not stereo frames.
  size_t frames = count / 2;
  for (size_t pos = 0; pos < frames && playing;) {
    if (xSemaphoreTake(audioMutex, pdMS_TO_TICKS(40)) != pdTRUE) continue;
    size_t space = RING_FRAMES - ringCount;
    size_t take = min(space, frames - pos);
    for (size_t j = 0; j < take; ++j) {
      pcmRing[ringWrite].channel1 = samples[(pos + j) * 2];
      pcmRing[ringWrite].channel2 = samples[(pos + j) * 2 + 1];
      ringWrite = (ringWrite + 1) % RING_FRAMES;
    }
    ringCount += take;
    xSemaphoreGive(audioMutex);
    pos += take;
    if (!take) vTaskDelay(pdMS_TO_TICKS(5));
  }
}

static void networkTask(void *) {
  uint8_t chunk[2048];
  while (true) {
    if (!playing || stationCount == 0 || WiFi.status() != WL_CONNECTED) {
      vTaskDelay(pdMS_TO_TICKS(300));
      continue;
    }
    const uint32_t session = generation;
    const String path = "/api/stations/" + encoded(stationNames[stationIndex]) + "/stream";
    const String address = endpoint(path);
    WiFiClient client;
    HTTPClient http;
    http.useHTTP10(true); // close-delimited body, no HTTP/1.1 chunk framing
    http.setTimeout(8000);
    MP3DecoderHelix decoder(decoded);
    clearAudio();
    if (http.begin(client, address) && http.GET() == HTTP_CODE_OK) {
      streamOK = true;
      decoder.begin();
      lastAudioMillis = millis();
      while (playing && generation == session && WiFi.status() == WL_CONNECTED) {
        int available = client.available();
        if (available > 0) {
          int got = client.read(chunk, min((int)sizeof(chunk), available));
          if (got > 0) { decoder.write(chunk, got); lastAudioMillis = millis(); }
        } else if (!client.connected() || millis() - lastAudioMillis > 8000) {
          break;
        } else {
          vTaskDelay(pdMS_TO_TICKS(15));
        }
      }
      decoder.end();
    }
    http.end();
    streamOK = false;
    if (playing && generation == session) vTaskDelay(pdMS_TO_TICKS(1500));
  }
}

static bool readJson(const String &path, JsonDocument &doc) {
  HTTPClient http;
  http.useHTTP10(true);
  http.setTimeout(5000);
  if (!http.begin(endpoint(path))) return false;
  const int status = http.GET();
  if (status != HTTP_CODE_OK) { http.end(); return false; }
  DeserializationError err = deserializeJson(doc, http.getStream());
  http.end();
  return !err;
}

static void refreshStations() {
  JsonDocument doc;
  if (!readJson("/api/stations", doc) || !doc.is<JsonArray>()) return;
  int count = 0;
  for (JsonObject entry : doc.as<JsonArray>()) {
    if (count >= 40) break;
    const char *name = entry["name"] | "";
    if (strlen(name)) stationNames[count++] = name;
  }
  stationCount = count;
  if (stationIndex >= stationCount) stationIndex = 0;
  Serial.printf("Loaded %d stations\n", stationCount);
}

static void refreshMetadata() {
  if (stationCount == 0 || WiFi.status() != WL_CONNECTED) return;
  JsonDocument doc;
  const String path = "/api/stations/" + encoded(stationNames[stationIndex]) + "/now";
  if (readJson(path, doc)) {
    title = doc["title"].as<String>();
    artist = doc["artist"].as<String>();
  }
}

static void selectStation(int change) {
  if (stationCount == 0) return;
  stationIndex = (stationIndex + change + stationCount) % stationCount;
  ++generation;
  clearAudio();
  title = ""; artist = "";
  lastMetadata = 0;
  Serial.printf("Station: %s\n", stationNames[stationIndex].c_str());
}

static void drawScreen() {
  auto &d = M5.Display;
  d.fillScreen(TFT_BLACK);
  d.setTextColor(TFT_GREEN, TFT_BLACK);
  d.setTextSize(2);
  d.setCursor(12, 12);
  d.print("AWC RADIO");
  keyBox(244,4,76,32,"SETTINGS");
  d.setTextColor(TFT_WHITE, TFT_BLACK);
  d.setCursor(12, 48);
  d.print(stationCount ? stationNames[stationIndex] : "Loading stations...");
  d.setTextSize(1);
  d.setCursor(12, 92);
  d.print(title.substring(0, 45));
  d.setCursor(12, 110);
  d.print(artist.substring(0, 45));
  d.setCursor(12, 145);
  d.printf("WiFi: %s  BT: %s", WiFi.status() == WL_CONNECTED ? "OK" : "OFF",
           btName.c_str());
  d.setCursor(12, 164);
  d.printf("Audio: %s", streamOK ? "streaming" : "waiting");
  d.drawLine(0, 189, 320, 189, TFT_DARKGREY);
  d.setTextColor(TFT_CYAN, TFT_BLACK);
  d.setTextSize(2);
  d.setCursor(19, 205); d.print("< PREV");
  d.setCursor(130, 205); d.print(playing ? "PAUSE" : "PLAY");
  d.setCursor(243, 205); d.print("NEXT >");
}

void setup() {
  Serial.begin(115200);
  auto config = M5.config();
  M5.begin(config);
  M5.Display.setRotation(1);
  M5.Display.setBrightness(110);
  configureDevice();

  audioMutex = xSemaphoreCreateMutex();
  pcmRing = (Frame *)ps_malloc(RING_FRAMES * sizeof(Frame));
  if (!audioMutex || !pcmRing) {
    Serial.println("FATAL: PSRAM/mutex allocation failed");
    M5.Display.println("PSRAM allocation failed");
    while (true) delay(1000);
  }
  WiFi.mode(WIFI_STA);
  WiFi.setSleep(false);
  WiFi.begin(wifiName.c_str(), wifiPassword.c_str());
  Serial.println("Connecting WiFi...");
  const unsigned long start = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - start < 15000) delay(250);
  if (WiFi.status() == WL_CONNECTED) refreshStations();

  bluetooth.set_data_callback_in_frames(bluetoothFrames);
  bluetooth.set_auto_reconnect(true);
  bluetooth.start(btName.c_str());
  playing = stationCount > 0;
  xTaskCreatePinnedToCore(networkTask, "AWCStream", 12000, nullptr, 1, nullptr, 1);
  drawScreen();
}

void loop() {
  M5.update();
  if (WiFi.status() != WL_CONNECTED) {
    static unsigned long lastWiFiRetry = 0;
    if (millis() - lastWiFiRetry > 10000) {
      lastWiFiRetry = millis();
      WiFi.reconnect();
    }
  } else if (!stationCount) {
    static unsigned long lastLookup = 0;
    if (millis() - lastLookup > 5000) {
      lastLookup = millis();
      refreshStations();
      if (stationCount && !playing) { playing = true; ++generation; }
    }
  }
  if (M5.Touch.getCount()) {
    auto detail = M5.Touch.getDetail(0);
    if (detail.wasPressed()) {
      if (detail.y < 35 && detail.x >= 244) {
        prefs.begin("awc-radio",false); prefs.putBool("setup",true); prefs.end(); ESP.restart();
      }
      if (detail.y > 185) {
        if (detail.x < 110) selectStation(-1);
        else if (detail.x > 220) selectStation(1);
        else { playing = !playing; ++generation; clearAudio(); }
      }
      drawScreen();
    }
  }
  if (millis() - lastMetadata > 5000 && stationCount) {
    lastMetadata = millis();
    refreshMetadata();
  }
  if (millis() - lastScreen > 2000) {
    lastScreen = millis();
    drawScreen();
  }
  delay(25);
}
