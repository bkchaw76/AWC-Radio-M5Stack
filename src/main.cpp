#include <Arduino.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <Preferences.h>
#include <WebServer.h>
#include <Update.h>
#include <SPIFFS.h>
#include <esp_ota_ops.h>
#include <esp_system.h>
#include <esp_task_wdt.h>
#include <ArduinoJson.h>
#include <M5Unified.h>
#include <BluetoothA2DPSource.h>
#include <esp_bt.h>
#include <esp_bt_main.h>
#include <esp_gap_bt_api.h>
#include <esp32-hal-bt.h>
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
static String wifiName, wifiPassword, btName, radioURL, radioUser, radioPass;
static String localURL, remoteURL;
static bool usingRemote=false;
static SemaphoreHandle_t serverMutex=nullptr;
static const char ROOT_CA[] = R"PEM(-----BEGIN CERTIFICATE-----
MIIFazCCA1OgAwIBAgIRAIIQz7DSQONZRGPgu2OCiwAwDQYJKoZIhvcNAQELBQAw
TzELMAkGA1UEBhMCVVMxKTAnBgNVBAoTIEludGVybmV0IFNlY3VyaXR5IFJlc2Vh
cmNoIEdyb3VwMRUwEwYDVQQDEwxJU1JHIFJvb3QgWDEwHhcNMTUwNjA0MTEwNDM4
WhcNMzUwNjA0MTEwNDM4WjBPMQswCQYDVQQGEwJVUzEpMCcGA1UEChMgSW50ZXJu
ZXQgU2VjdXJpdHkgUmVzZWFyY2ggR3JvdXAxFTATBgNVBAMTDElTUkcgUm9vdCBY
MTCCAiIwDQYJKoZIhvcNAQEBBQADggIPADCCAgoCggIBAK3oJHP0FDfzm54rVygc
h77ct984kIxuPOZXoHj3dcKi/vVqbvYATyjb3miGbESTtrFj/RQSa78f0uoxmyF+
0TM8ukj13Xnfs7j/EvEhmkvBioZxaUpmZmyPfjxwv60pIgbz5MDmgK7iS4+3mX6U
A5/TR5d8mUgjU+g4rk8Kb4Mu0UlXjIB0ttov0DiNewNwIRt18jA8+o+u3dpjq+sW
T8KOEUt+zwvo/7V3LvSye0rgTBIlDHCNAymg4VMk7BPZ7hm/ELNKjD+Jo2FR3qyH
B5T0Y3HsLuJvW5iB4YlcNHlsdu87kGJ55tukmi8mxdAQ4Q7e2RCOFvu396j3x+UC
B5iPNgiV5+I3lg02dZ77DnKxHZu8A/lJBdiB3QW0KtZB6awBdpUKD9jf1b0SHzUv
KBds0pjBqAlkd25HN7rOrFleaJ1/ctaJxQZBKT5ZPt0m9STJEadao0xAH0ahmbWn
OlFuhjuefXKnEgV4We0+UXgVCwOPjdAvBbI+e0ocS3MFEvzG6uBQE3xDk3SzynTn
jh8BCNAw1FtxNrQHusEwMFxIt4I7mKZ9YIqioymCzLq9gwQbooMDQaHWBfEbwrbw
qHyGO0aoSCqI3Haadr8faqU9GY/rOPNk3sgrDQoo//fb4hVC1CLQJ13hef4Y53CI
rU7m2Ys6xt0nUW7/vGT1M0NPAgMBAAGjQjBAMA4GA1UdDwEB/wQEAwIBBjAPBgNV
HRMBAf8EBTADAQH/MB0GA1UdDgQWBBR5tFnme7bl5AFzgAiIyBpY9umbbjANBgkq
hkiG9w0BAQsFAAOCAgEAVR9YqbyyqFDQDLHYGmkgJykIrGF1XIpu+ILlaS/V9lZL
ubhzEFnTIZd+50xx+7LSYK05qAvqFyFWhfFQDlnrzuBZ6brJFe+GnY+EgPbk6ZGQ
3BebYhtF8GaV0nxvwuo77x/Py9auJ/GpsMiu/X1+mvoiBOv/2X/qkSsisRcOj/KK
NFtY2PwByVS5uCbMiogziUwthDyC3+6WVwW6LLv3xLfHTjuCvjHIInNzktHCgKQ5
ORAzI4JMPJ+GslWYHb4phowim57iaztXOoJwTdwJx4nLCgdNbOhdjsnvzqvHu7Ur
TkXWStAmzOVyyghqpZXjFaH3pO3JLF+l+/+sKAIuvtd7u+Nxe5AW0wdeRlN8NwdC
jNPElpzVmbUq4JUagEiuTDkHzsxHpFKVK7q4+63SM1N95R1NbdWhscdCb+ZAJzVc
oyi3B43njTOQ5yOf+1CceWxG1bQVs5ZufpsMljq4Ui0/1lvh+wjChP4kqKOJ2qxq
4RgqsahDYVvTH9w7jXbyLeiNdd8XM2w9U/t7y0Ff/9yi0GE44Za4rF2LN9d11TPA
mRGunUHBcnWEvgJBQl9nJEiU0Zsnvgc/ubhPgXRR4Xq37Z0j4r7g1SgEEzwxA57d
emyPxgcYxn/eR44/KJ4EBs+lVDR3veyJm+kXQ99b21/+jh5Xos1AnX5iItreGCc=
-----END CERTIFICATE-----
)PEM";
static bool validURL(const String &url) { return (url.startsWith("http://") || url.startsWith("https://")) && url.indexOf('@')<0; }
static bool beginRadio(HTTPClient &http, WiFiClient &plain, WiFiClientSecure &tls, const String &url) {
  if(!validURL(url)) return false;
  if(url.startsWith("https://")) {
    if(time(nullptr)<1700000000) return false;
    tls.setCACert(ROOT_CA); tls.setHandshakeTimeout(8);
    if(!http.begin(tls,url)) return false;
    if(radioUser.length() && remoteURL.length() && url.startsWith(remoteURL+"/")) http.setAuthorization(radioUser.c_str(),radioPass.c_str());
  } else { if(!http.begin(plain,url)) return false; }
  http.setFollowRedirects(HTTPC_DISABLE_FOLLOW_REDIRECTS);
  return true;
}
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
  const char *rows[3][4]={{"1234567890","qwertyuiop","asdfghjkl","zxcvbnm.-/"},{"1234567890","QWERTYUIOP","ASDFGHJKL","ZXCVBNM.-/"},{"!@#$%^&*()","_+=:;?,~|","[]{}<>\\`","'\".-/"}};
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
static String savedSSID[6], savedPass[6];
static int wifiCount=0, wifiIndex=0;
static void loadWifi() {
  wifiCount=constrain(prefs.getInt("wcount",0),0,6);
  for(int i=0;i<wifiCount;i++) {
    savedSSID[i]=prefs.getString(("ws"+String(i)).c_str(),"");
    savedPass[i]=prefs.getString(("wp"+String(i)).c_str(),"");
  }
  if(!wifiCount && wifiName.length() && wifiName!="YOUR_IPHONE_HOTSPOT") {
    savedSSID[0]=wifiName; savedPass[0]=wifiPassword; wifiCount=1;
  }
  wifiIndex=constrain(prefs.getInt("windex",0),0,max(0,wifiCount-1));
}
static void saveWifi() {
  prefs.putInt("wcount",wifiCount); prefs.putInt("windex",wifiIndex);
  for(int i=0;i<6;i++) {
    String sk="ws"+String(i), pk="wp"+String(i);
    if(i<wifiCount) { prefs.putString(sk.c_str(),savedSSID[i]); prefs.putString(pk.c_str(),savedPass[i]); }
    else { prefs.remove(sk.c_str()); prefs.remove(pk.c_str()); }
  }
  if(wifiCount) { wifiName=savedSSID[wifiIndex]; wifiPassword=savedPass[wifiIndex]; }
  else { wifiName=""; wifiPassword=""; }
  prefs.putString("ssid",wifiName); prefs.putString("password",wifiPassword);
}
static void wifiManager() {
  int page=0;
  while(true) {
    heading("Wi-Fi networks");
    for(int i=0;i<4 && page+i<wifiCount;i++) {
      int n=page+i;
      keyBox(4,35+i*40,244,40,String(n==wifiIndex?"* ":"")+savedSSID[n]);
      keyBox(248,35+i*40,72,40,"FORGET");
    }
    keyBox(0,200,60,40,"PREV"); keyBox(60,200,60,40,"NEXT"); keyBox(120,200,90,40,"ADD/EDIT"); keyBox(210,200,110,40,"BACK");
    int x,y; tap(x,y);
    if(y>=200) {
      if(x<60) page=max(0,page-4);
      else if(x<120) { if(page+4<wifiCount) page+=4; }
      else if(x<210) {
        String ssid=chooseWifi(); if(!ssid.length()) continue;
        int n=0; while(n<wifiCount && savedSSID[n]!=ssid) n++;
        if(n>=6) { heading("Forget a network first"); delay(1200); continue; }
        String pw=keyboard("Wi-Fi password",n<wifiCount?savedPass[n]:"",true);
        if(n==wifiCount) wifiCount++;
        savedSSID[n]=ssid; savedPass[n]=pw; wifiIndex=n; page=(n/4)*4;
      } else return;
    } else if(y>=35 && y<195 && page+(y-35)/40<wifiCount) {
      int n=page+(y-35)/40;
      if(x>=248) {
        heading("Forget network?"); keyBox(0,100,160,60,"CANCEL"); keyBox(160,100,160,60,"FORGET");
        int cx,cy; tap(cx,cy); if(cx<160 || cy<100 || cy>=160) continue;
        for(int i=n;i+1<wifiCount;i++) { savedSSID[i]=savedSSID[i+1]; savedPass[i]=savedPass[i+1]; }
        wifiCount--; savedSSID[wifiCount]=""; savedPass[wifiCount]="";
        if(wifiIndex>n) wifiIndex--; wifiIndex=constrain(wifiIndex,0,max(0,wifiCount-1)); page=0;
      } else wifiIndex=n;
    }
    saveWifi();
  }
}
static unsigned long wifiAttemptAt=0;
static void connectSavedWifi(bool rotate) {
  wifiAttemptAt=millis();
  if(wifiCount) {
    if(rotate) wifiIndex=(wifiIndex+1)%wifiCount;
    wifiName=savedSSID[wifiIndex]; wifiPassword=savedPass[wifiIndex];
  }
  if(wifiName.length()) WiFi.begin(wifiName.c_str(),wifiPassword.c_str());
}
// Setup runs before A2DP starts. Use Classic GAP inquiry, then reboot
// into the normal A2DP player after saving the selected receiver.
struct FoundBT { char name[249]; uint8_t addr[6]; int rssi; };
static FoundBT foundBT[24];
static int foundCount=0;
static portMUX_TYPE scanMux=portMUX_INITIALIZER_UNLOCKED;
static volatile bool inquiryDone=false;
static void gapScan(esp_bt_gap_cb_event_t event, esp_bt_gap_cb_param_t *p) {
  if(event==ESP_BT_GAP_DISC_STATE_CHANGED_EVT) {
    if(p->disc_st_chg.state==ESP_BT_GAP_DISCOVERY_STOPPED) inquiryDone=true;
    return;
  }
  if(event!=ESP_BT_GAP_DISC_RES_EVT) return;
  char name[249]={};
  int rssi=-127;
  uint32_t cod=0;
  bool hasCod=false;
  for(int i=0;i<p->disc_res.num_prop;i++) {
    auto &prop=p->disc_res.prop[i];
    if(prop.type==ESP_BT_GAP_DEV_PROP_BDNAME) memcpy(name,prop.val,min((int)prop.len,248));
    else if(prop.type==ESP_BT_GAP_DEV_PROP_RSSI) rssi=*(int8_t*)prop.val;
    else if(prop.type==ESP_BT_GAP_DEV_PROP_COD && prop.len>=4) { memcpy(&cod,prop.val,4); hasCod=true; }
    else if(prop.type==ESP_BT_GAP_DEV_PROP_EIR && !name[0]) {
      uint8_t len=0;
      uint8_t *n=esp_bt_gap_resolve_eir_data((uint8_t*)prop.val,ESP_BT_EIR_TYPE_CMPL_LOCAL_NAME,&len);
      if(!n) n=esp_bt_gap_resolve_eir_data((uint8_t*)prop.val,ESP_BT_EIR_TYPE_SHORT_LOCAL_NAME,&len);
      if(n) memcpy(name,n,min((int)len,248));
    }
  }
  if(!name[0]) return;
  // Audio/video major class or rendering service; keep unknown class devices.
  if(hasCod && cod && ((cod>>8)&31)!=4 && !(cod & (1UL<<18))) return;
  portENTER_CRITICAL(&scanMux);
  int idx=0;
  while(idx<foundCount && memcmp(foundBT[idx].addr,p->disc_res.bda,6)) idx++;
  if(idx<24) {
    if(idx==foundCount) foundCount++;
    memcpy(foundBT[idx].addr,p->disc_res.bda,6);
    memcpy(foundBT[idx].name,name,sizeof(name));
    foundBT[idx].rssi=rssi;
  }
  portEXIT_CRITICAL(&scanMux);
}
struct SavedBT { char name[249]; uint8_t addr[6]; };
static SavedBT savedBT[8]={};
static int savedCount=0, selectedBT=-1;
static uint8_t chosenAddr[6]={};
static bool btEnabled=true;
static bool hasAddr(const uint8_t *a) { for(int i=0;i<6;i++) if(a[i]) return true; return false; }
static void loadBT() {
  if(prefs.getBytesLength("btlist")==sizeof(savedBT)) prefs.getBytes("btlist",savedBT,sizeof(savedBT));
  savedCount=constrain(prefs.getInt("btcount",0),0,8);
  selectedBT=prefs.getInt("btindex",-1);
  if(selectedBT<0 || selectedBT>=savedCount) selectedBT=-1;
  btEnabled=prefs.getBool("btenabled",true);
}
static void saveBT() {
  prefs.putBytes("btlist",savedBT,sizeof(savedBT));
  prefs.putInt("btcount",savedCount); prefs.putInt("btindex",selectedBT);
  prefs.putBool("btenabled",btEnabled);
}
static bool initClassic() {
  if(!btStarted() && !btStart()) return false;
  if(esp_bluedroid_get_status()==ESP_BLUEDROID_STATUS_UNINITIALIZED && esp_bluedroid_init()!=ESP_OK) return false;
  if(esp_bluedroid_get_status()==ESP_BLUEDROID_STATUS_INITIALIZED && esp_bluedroid_enable()!=ESP_OK) return false;
  return true;
}
static String chooseBluetooth() {
  memset(chosenAddr,0,6);
  if(!btStarted() && !btStart()) return keyboard("Bluetooth name",btName);
  if(esp_bluedroid_get_status()==ESP_BLUEDROID_STATUS_UNINITIALIZED && esp_bluedroid_init()!=ESP_OK) return keyboard("Bluetooth name",btName);
  if(esp_bluedroid_get_status()==ESP_BLUEDROID_STATUS_INITIALIZED && esp_bluedroid_enable()!=ESP_OK) return keyboard("Bluetooth name",btName);
  if(esp_bt_gap_register_callback(gapScan)!=ESP_OK) return keyboard("Bluetooth name",btName);
  while(true) {
    heading("Scanning Bluetooth");
    M5.Display.setTextSize(1); M5.Display.setCursor(8,42); M5.Display.print("Put receiver in pairing mode");
    portENTER_CRITICAL(&scanMux); foundCount=0; portEXIT_CRITICAL(&scanMux);
    inquiryDone=false;
    if(esp_bt_gap_start_discovery(ESP_BT_INQ_MODE_GENERAL_INQUIRY,8,0)==ESP_OK) {
      unsigned long start=millis();
      while(!inquiryDone && millis()-start<13000) { M5.update(); delay(50); }
      if(!inquiryDone) { esp_bt_gap_cancel_discovery(); delay(500); }
    }
    int page=0;
    while(true) {
      heading("Bluetooth devices");
      int n;
      portENTER_CRITICAL(&scanMux); n=foundCount; portEXIT_CRITICAL(&scanMux);
      if(!n) { M5.Display.setTextSize(1); M5.Display.setCursor(8,50); M5.Display.print("No named audio devices found"); }
      for(int i=0;i<4 && page+i<n;i++) {
        FoundBT item;
        portENTER_CRITICAL(&scanMux); item=foundBT[page+i]; portEXIT_CRITICAL(&scanMux);
        keyBox(4,35+i*40,312,40,String(item.name).substring(0,40)+" "+String(item.rssi));
      }
      keyBox(0,200,60,40,"PREV"); keyBox(60,200,60,40,"NEXT"); keyBox(120,200,70,40,"SCAN"); keyBox(190,200,70,40,"MANUAL"); keyBox(260,200,60,40,"BACK");
      int x,y; tap(x,y);
      if(y>=200) {
        if(x<60) page=max(0,page-4);
        else if(x<120) { if(page+4<n) page+=4; }
        else if(x<190) break;
        else if(x<260) return keyboard("Bluetooth name",btName=="YOUR_CAR_BLUETOOTH_NAME"?"":btName);
        else return "";
      } else if(y>=35 && y<195 && page+(y-35)/40<n) {
        FoundBT item;
        portENTER_CRITICAL(&scanMux); item=foundBT[page+(y-35)/40]; portEXIT_CRITICAL(&scanMux);
        memcpy(chosenAddr,item.addr,6);
        return String(item.name);
      }
    }
  }
}
static void manageBluetooth() {
  int page=0;
  while(true) {
    heading("Bluetooth manager");
    for(int i=0;i<4 && page+i<savedCount;i++) {
      int idx=page+i;
      keyBox(4,35+i*40,244,40,String(idx==selectedBT && btEnabled?"* ":"")+String(savedBT[idx].name).substring(0,32));
      keyBox(248,35+i*40,72,40,"FORGET");
    }
    keyBox(0,200,48,40,"PREV"); keyBox(48,200,48,40,"NEXT"); keyBox(96,200,64,40,"ADD"); keyBox(160,200,96,40,btEnabled?"DISCONNECT":"ENABLE"); keyBox(256,200,64,40,"BACK");
    int x,y; tap(x,y);
    if(y>=200) {
      if(x<48) page=max(0,page-4);
      else if(x<96) { if(page+4<savedCount) page+=4; }
      else if(x<160) {
        String name=chooseBluetooth();
        if(!name.length()) continue;
        int idx=0;
        while(idx<savedCount && !(hasAddr(chosenAddr)?memcmp(savedBT[idx].addr,chosenAddr,6)==0:!hasAddr(savedBT[idx].addr)&&name==savedBT[idx].name)) idx++;
        if(idx==8) { heading("List full: forget one"); delay(1500); continue; }
        if(idx==savedCount) savedCount++;
        memset(&savedBT[idx],0,sizeof(SavedBT));
        name.toCharArray(savedBT[idx].name,sizeof(savedBT[idx].name)); memcpy(savedBT[idx].addr,chosenAddr,6);
        selectedBT=idx; btName=name; btEnabled=true; page=(idx/4)*4;
      } else if(x<256) btEnabled=!btEnabled;
      else return;
    } else if(y>=35 && y<195 && page+(y-35)/40<savedCount) {
      int idx=page+(y-35)/40;
      if(x>=248) {
        heading("Forget device?"); keyBox(0,100,160,60,"CANCEL"); keyBox(160,100,160,60,"FORGET");
        int cx,cy; tap(cx,cy); if(cx<160 || cy<100 || cy>=160) continue;
        if(hasAddr(savedBT[idx].addr) && initClassic()) esp_bt_gap_remove_bond_device(savedBT[idx].addr);
        for(int k=idx;k+1<savedCount;k++) savedBT[k]=savedBT[k+1];
        memset(&savedBT[--savedCount],0,sizeof(SavedBT));
        if(selectedBT==idx) { selectedBT=-1; btEnabled=false; btName=""; }
        else if(selectedBT>idx) selectedBT--;
        page=min(page,max(0,((savedCount-1)/4)*4));
      } else { selectedBT=idx; btName=savedBT[idx].name; btEnabled=true; }
    }
    saveBT(); prefs.putString("bt",btName);
  }
}
static bool matchSaved(const char *name,esp_bd_addr_t addr,int rssi) {
  (void)rssi;
  if(selectedBT>=0 && hasAddr(savedBT[selectedBT].addr)) return memcmp(addr,savedBT[selectedBT].addr,6)==0;
  return btName==name;
}
static bool lcdSetup() {
  while(true) {
    heading("AWC Radio Settings");
    keyBox(4,35,312,38,"Wi-Fi: "+wifiName);
    keyBox(4,75,312,38,"Bluetooth: "+btName);
    keyBox(4,115,312,38,"Servers: "+localURL);
    keyBox(4,155,312,38,"Save and restart");
    keyBox(4,195,312,42,"Phone setup (optional)");
    int x,y; tap(x,y);
    if(y>=35 && y<75) wifiManager();
    else if(y>=75 && y<115) manageBluetooth();
    else if(y>=115 && y<155) { localURL=keyboard("Local server URL",localURL); remoteURL=keyboard("Remote HTTPS (optional)",remoteURL); if(remoteURL.length()) { radioUser=keyboard("HTTPS user (optional)",radioUser); radioPass=keyboard("HTTPS password",radioPass,true); } }
    else if(y>=155 && y<195) {
      if(!wifiName.length() || wifiName=="YOUR_IPHONE_HOTSPOT" || (btEnabled && (!btName.length() || btName=="YOUR_CAR_BLUETOOTH_NAME")) || !validURL(localURL) || (remoteURL.length() && (!validURL(remoteURL) || !remoteURL.startsWith("https://")))) { heading("Check settings"); delay(1200); continue; }
      prefs.putString("ssid",wifiName); prefs.putString("password",wifiPassword); prefs.putString("bt",btName); prefs.putString("url",localURL); prefs.putString("remote",remoteURL); prefs.putString("ruser",radioUser); prefs.putString("rpass",radioPass);
      prefs.end(); ESP.restart(); return true;
    } else if(y>=195) return false;
  }
}
static void configureDevice() {
  prefs.begin("awc-radio", false);
  wifiName = prefs.getString("ssid", WIFI_SSID);
  wifiPassword = prefs.getString("password", WIFI_PASSWORD);
  loadWifi();
  btName = prefs.getString("bt", BT_SPEAKER_NAME);
  loadBT();
  localURL = prefs.getString("url", RADIO_SERVER_URL);
  remoteURL=prefs.getString("remote","");
  if(localURL.startsWith("https://") && !remoteURL.length()) { remoteURL=localURL; localURL=RADIO_SERVER_URL; }
  while(localURL.endsWith("/")) localURL.remove(localURL.length()-1);
  while(remoteURL.endsWith("/")) remoteURL.remove(remoteURL.length()-1);
  radioURL=localURL;
  radioUser=prefs.getString("ruser",""); radioPass=prefs.getString("rpass","");
  bool forceSetup=prefs.getBool("setup",false); prefs.remove("setup");
  // Hold the touchscreen during startup to reopen setup.
  delay(600);
  M5.update();
  if (wifiName.length() && wifiName != "YOUR_IPHONE_HOTSPOT" && (!btEnabled || btName != "YOUR_CAR_BLUETOOTH_NAME") && !M5.Touch.getCount() && !forceSetup) {
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
    if (!setupServer.arg("ssid").length() || !setupServer.arg("bt").length() || !validURL(url)) {
      setupServer.send(400, "text/plain", "SSID, Bluetooth name and http:// server URL required");
      return;
    }
    int n=0; while(n<wifiCount && savedSSID[n]!=setupServer.arg("ssid")) n++;
    if(n>=6) { setupServer.send(400,"text/plain","Wi-Fi list full; forget a network on the LCD first"); return; }
    if(n==wifiCount) wifiCount++;
    wifiIndex=n; savedSSID[n]=setupServer.arg("ssid"); savedPass[n]=setupServer.arg("password"); saveWifi();
    prefs.putString("bt", setupServer.arg("bt"));
    selectedBT=-1; btEnabled=true; saveBT();
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
static bool btStartedForAudio=false;
static volatile bool receiverReady=false;
static unsigned long btRetryInterval=15000;
static unsigned long btConnectedAt=0;
static unsigned long lastBTAttempt=0;
static volatile bool liveSwitching=false;
static SemaphoreHandle_t audioMutex;
static Frame *pcmRing = nullptr;
static constexpr size_t RING_FRAMES = 32768; // 128 KB PSRAM, ~0.74 s PCM
static size_t ringRead = 0, ringWrite = 0, ringCount = 0;
static volatile uint32_t underruns=0, droppedFrames=0;
static volatile uint32_t decoderSession=0;
static_assert(sizeof(Frame)==4,"Stereo PCM frame must be four bytes");
static volatile bool playing = false;
static volatile int audioVolume=100;
static volatile bool audioMuted=false;
static volatile uint32_t generation = 1;
static volatile uint32_t lastAudioMillis = 0;
static volatile uint32_t streamHeartbeat=0;
static volatile uint32_t streamRecoveries=0;
static volatile bool streamWatchdogReady=false;
static unsigned long lastRecovery=0;
static String stationNames[40];
static int stationCount = 0, stationIndex = 0;
static String title = "", artist = "", nextTitle="", lastStation="";
static double trackStart=0, trackEnd=0, elapsedBase=0;
static unsigned long metadataAt=0;
static uint64_t favorites=0;
static int brightness=110, nightBrightness=40, screenTimeout=60;
static bool carMode=false, nightMode=false, standby=false, screenOff=false, resumePlaying=false, seenUSB=false;
static unsigned long lastTouch=0, usbLostAt=0, lastPowerPoll=0;
static volatile bool prebuffer=true;
static bool logoDirty=true, logoOK=false;
static unsigned long logoRetryAt=0;
static M5Canvas logoCanvas(&M5.Display);
static unsigned long lastMetadata = 0, lastScreen = 0;
static volatile bool streamOK = false;

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
  if(serverMutex) xSemaphoreTake(serverMutex,portMAX_DELAY);
  String base = radioURL;
  if(serverMutex) xSemaphoreGive(serverMutex);
  while (base.endsWith("/")) base.remove(base.length() - 1);
  return base + path;
}

static void clearAudio() {
  if (!audioMutex) return;
  if (xSemaphoreTake(audioMutex, pdMS_TO_TICKS(100))) {
    ringRead = ringWrite = ringCount = 0; prebuffer=true;
    xSemaphoreGive(audioMutex);
  }
}

// Called by the Bluetooth stack. Never wait on Wi-Fi or do allocations here.
static int32_t bluetoothFrames(Frame *dst, int32_t requested) {
  if (requested <= 0) return 0;
  memset(dst, 0, (size_t)requested * sizeof(Frame));
  if (!playing || !pcmRing || liveSwitching) return requested;
  if (xSemaphoreTake(audioMutex, 0) != pdTRUE) return requested;
  if(prebuffer && ringCount<11025) { xSemaphoreGive(audioMutex); return requested; }
  prebuffer=false;
  const int gain=audioMuted?0:audioVolume;
  size_t take=min((size_t)requested,ringCount);
  size_t first=min(take,RING_FRAMES-ringRead);
  memcpy(dst,pcmRing+ringRead,first*sizeof(Frame));
  if(take>first) memcpy(dst+first,pcmRing,(take-first)*sizeof(Frame));
  ringRead=(ringRead+take)%RING_FRAMES; ringCount-=take;
  if(take<(size_t)requested) { ++underruns; prebuffer=true; }
  xSemaphoreGive(audioMutex);
  if(gain==0) memset(dst,0,take*sizeof(Frame));
  else if(gain!=100) for(size_t i=0;i<take;i++) {
    dst[i].channel1=(int32_t)dst[i].channel1*gain/100;
    dst[i].channel2=(int32_t)dst[i].channel2*gain/100;
  }
  return requested;
}

static void decoded(MP3FrameInfo &info, short *samples, size_t count, void *) {
  if (!playing || !pcmRing || !samples || info.samprate != 44100 ||
      info.nChans != 2) return;
  // Helix callback length is count of 16-bit PCM samples, not stereo frames.
  size_t frames = count / 2;
  const uint32_t session=decoderSession;
  const unsigned long started=millis();
  size_t pos=0;
  for (; pos < frames && playing && generation==session && millis()-started<1000;) {
    if (xSemaphoreTake(audioMutex, pdMS_TO_TICKS(40)) != pdTRUE) continue;
    if(generation!=session || !playing) { xSemaphoreGive(audioMutex); break; }
    size_t space = RING_FRAMES - ringCount;
    size_t take = min(space, frames - pos);
    size_t first=min(take,RING_FRAMES-ringWrite);
    memcpy(pcmRing+ringWrite,samples+pos*2,first*sizeof(Frame));
    if(take>first) memcpy(pcmRing,samples+(pos+first)*2,(take-first)*sizeof(Frame));
    ringWrite=(ringWrite+take)%RING_FRAMES;
    ringCount += take;
    xSemaphoreGive(audioMutex);
    pos += take;
    if (!take) vTaskDelay(pdMS_TO_TICKS(5));
  }
  if(pos<frames && generation==session && playing) droppedFrames+=frames-pos;
}

static void networkTask(void *) {
  uint8_t chunk[2048];
  streamWatchdogReady=(esp_task_wdt_add(nullptr)==ESP_OK);
  while (true) {
    streamHeartbeat=millis(); if(streamWatchdogReady) esp_task_wdt_reset();
    if (liveSwitching || !receiverReady || !btEnabled || !playing || stationCount == 0 || WiFi.status() != WL_CONNECTED) {
      vTaskDelay(pdMS_TO_TICKS(300));
      continue;
    }
    const uint32_t session = generation;
    const String path = "/api/stations/" + encoded(stationNames[stationIndex]) + "/stream";
    const String address = endpoint(path);
    WiFiClient plain; WiFiClientSecure tls;
    HTTPClient http;
    http.useHTTP10(true); // close-delimited body, no HTTP/1.1 chunk framing
    http.setConnectTimeout(5000);
    http.setTimeout(8000);
    MP3DecoderHelix decoder(decoded);
    clearAudio();
    if (beginRadio(http,plain,tls,address) && http.GET() == HTTP_CODE_OK) {
      WiFiClient &client=http.getStream();
      streamOK = true;
      decoderSession=session;
      decoder.begin();
      lastAudioMillis = millis();
      while (playing && receiverReady && generation == session && WiFi.status() == WL_CONNECTED) {
        streamHeartbeat=millis(); if(streamWatchdogReady) esp_task_wdt_reset();
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
    streamHeartbeat=millis(); if(streamWatchdogReady) esp_task_wdt_reset();
    if (playing && generation == session) vTaskDelay(pdMS_TO_TICKS(1500));
  }
}

static bool readJson(const String &path, JsonDocument &doc) {
  WiFiClient plain; WiFiClientSecure tls;
  HTTPClient http;
  http.useHTTP10(true);
  http.setConnectTimeout(5000);
  http.setTimeout(5000);
  if (!beginRadio(http,plain,tls,endpoint(path))) return false;
  const int status = http.GET();
  if (status != HTTP_CODE_OK) { http.end(); return false; }
  DeserializationError err = deserializeJson(doc, http.getStream());
  http.end();
  return !err;
}

static bool probeServer(String base) {
  while(base.endsWith("/")) base.remove(base.length()-1);
  if(!validURL(base)) return false;
  WiFiClient plain; WiFiClientSecure tls; HTTPClient http;
  http.useHTTP10(true); http.setConnectTimeout(1500); http.setTimeout(2500);
  if(!beginRadio(http,plain,tls,base+"/api/stations")) return false;
  bool ok=false;
  if(http.GET()==200) { JsonDocument doc; ok=!deserializeJson(doc,http.getStream()) && doc.is<JsonArray>() && doc.size()>0; }
  http.end(); return ok;
}
static unsigned long lastServerCheck=0;
static String checkedSSID;
static IPAddress checkedIP;
static bool serverReachable=false;
static void checkServers(bool force=false) {
  if(WiFi.status()!=WL_CONNECTED) { serverReachable=false; checkedSSID=""; return; }
  String ssid=WiFi.SSID(); IPAddress ip=WiFi.localIP();
  bool changed=ssid!=checkedSSID || ip!=checkedIP;
  if(!force && !changed && millis()-lastServerCheck<(serverReachable?60000UL:10000UL)) return;
  checkedSSID=ssid; checkedIP=ip; lastServerCheck=millis();
  bool local=probeServer(localURL);
  bool remote=!local && remoteURL.length() && probeServer(remoteURL);
  serverReachable=local || remote;
  if(!serverReachable) return;
  String chosen=local?localURL:remoteURL;
  if(chosen!=radioURL) {
    xSemaphoreTake(serverMutex,portMAX_DELAY); radioURL=chosen; xSemaphoreGive(serverMutex);
    usingRemote=remote; ++generation; clearAudio(); logoDirty=true; logoOK=false; logoRetryAt=0; lastMetadata=0;
    Serial.println(remote?"Using remote HTTPS server":"Using local radio server");
  }
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
  for(int i=0;i<count;i++) if(stationNames[i]==lastStation) stationIndex=i;
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
    nextTitle=doc["next_song"]["title"] | "";
    trackStart=doc["start"] | 0.0; trackEnd=doc["end"] | 0.0;
    // Elapsed time uses the NTP-synchronized device clock.
    elapsedBase=0; metadataAt=millis();
    time_t now=time(nullptr); if(now>1700000000) elapsedBase=max(0.0,(double)now-trackStart);
  }
}

static void selectStation(int change) {
  if (stationCount == 0) return;
  stationIndex = (stationIndex + change + stationCount) % stationCount;
  ++generation;
  clearAudio();
  title = ""; artist = ""; nextTitle=""; trackStart=trackEnd=0; logoDirty=true; logoOK=false; logoRetryAt=0;
  lastStation=stationNames[stationIndex]; prefs.begin("awc-radio",false); prefs.putString("station",lastStation); prefs.end();
  lastMetadata = 0;
  Serial.printf("Station: %s\n", stationNames[stationIndex].c_str());
}

// Live switching uses the running A2DP stack; discovery/add/forget stays in setup.

static void liveBluetooth() {
  int page=0;
  while(true) {
    heading("Switch Bluetooth");
    for(int i=0;i<4 && page+i<savedCount;i++) {
      int idx=page+i;
      keyBox(4,35+i*40,312,40,String(idx==selectedBT && btEnabled?"* ":"")+String(savedBT[idx].name).substring(0,40));
    }
    keyBox(0,200,55,40,"PREV"); keyBox(55,200,55,40,"NEXT"); keyBox(110,200,105,40,"DISCONNECT"); keyBox(215,200,105,40,"BACK");
    int x,y; tap(x,y);
    int target=-2;
    if(y>=200) {
      if(x<55) page=max(0,page-4);
      else if(x<110) { if(page+4<savedCount) page+=4; }
      else if(x<215) target=-1;
      else return;
    } else if(y>=35 && y<195 && page+(y-35)/40<savedCount) target=page+(y-35)/40;
    if(target==-2) continue;
    if(target>=0 && !hasAddr(savedBT[target].addr)) {
      heading("Scan device in setup"); delay(1500); continue;
    }
    if(target==selectedBT && btEnabled && btStartedForAudio && bluetooth.is_connected()) return;
    liveSwitching=true; receiverReady=false;
    ++generation; clearAudio();
    heading("Switching receiver");
    if(btStartedForAudio) {
      bluetooth.set_auto_reconnect(false);
      bluetooth.disconnect();
      unsigned long start=millis();
      while(bluetooth.is_connected() && millis()-start<8000) delay(50);
      if(bluetooth.is_connected()) { liveSwitching=false; heading("Disconnect timed out"); delay(1500); return; }
      delay(500);
    }
    btEnabled=target>=0;
    if(target>=0) { selectedBT=target; btName=savedBT[target].name; }
    prefs.begin("awc-radio",false); saveBT(); prefs.putString("bt",btName); prefs.end();
    if(btEnabled) {
      if(!btStartedForAudio) {
        bluetooth.set_data_callback_in_frames(bluetoothFrames);
        bluetooth.set_ssid_callback(matchSaved);
        bluetooth.set_auto_reconnect(savedBT[selectedBT].addr);
        bluetooth.start(btName.c_str());
        btStartedForAudio=true;
      } else {
        bluetooth.set_auto_reconnect(savedBT[selectedBT].addr);
        bluetooth.connect_to(savedBT[selectedBT].addr);
      }
    }
    lastBTAttempt=millis(); btRetryInterval=15000; liveSwitching=false; return;
  }
}
static constexpr uint16_t UI_BG=0x0862, UI_GOLD=0xED6C, UI_GREY=0xAD55, UI_GREEN=0x56D0, UI_BLUE=0x653F;
static bool cacheReady=false;
static String logoPath() {
  uint64_t hash=14695981039346656037ULL;
  String key=localURL+"/"+stationNames[stationIndex];
  for(size_t i=0;i<key.length();i++) { hash^=(uint8_t)key[i]; hash*=1099511628211ULL; }
  char name[32]; snprintf(name,sizeof(name),"/l%016llx.raw",(unsigned long long)hash);
  return String(name);
}
static bool loadCachedLogo(const String &path) {
  if(!cacheReady) return false;
  File file=SPIFFS.open(path,"r");
  constexpr size_t bytes=94*94*2;
  if(!file || file.size()!=bytes) return false;
  if(!logoCanvas.width()) { logoCanvas.setColorDepth(16); logoCanvas.createSprite(94,94); }
  if(!logoCanvas.width()) return false;
  return file.read((uint8_t*)logoCanvas.getBuffer(),bytes)==bytes;
}
static void saveCachedLogo(const String &path) {
  if(!cacheReady || !logoCanvas.width()) return;
  constexpr size_t bytes=94*94*2;
  if(SPIFFS.totalBytes()-SPIFFS.usedBytes()<bytes+8192) return;
  SPIFFS.remove("/logo.tmp");
  File file=SPIFFS.open("/logo.tmp","w");
  if(!file) return;
  size_t written=file.write((const uint8_t*)logoCanvas.getBuffer(),bytes);
  file.close();
  if(written==bytes) { SPIFFS.remove(path); SPIFFS.rename("/logo.tmp",path); }
  else SPIFFS.remove("/logo.tmp");
}
static void fetchLogo() {
  if(!logoDirty || !stationCount) return;
  if(logoRetryAt && (int32_t)(millis()-logoRetryAt)<0) return;
  logoOK=false;
  String path=logoPath();
  if(loadCachedLogo(path)) { logoOK=true; logoDirty=false; return; }
  if(WiFi.status()!=WL_CONNECTED) { logoRetryAt=millis()+5000; return; }
  logoRetryAt=millis()+15000;
  WiFiClient plain; WiFiClientSecure tls;
  HTTPClient http; http.useHTTP10(true); http.setConnectTimeout(3000); http.setTimeout(2500);
  if(!beginRadio(http,plain,tls,endpoint("/api/logo/"+encoded(stationNames[stationIndex])))) return;
  if(http.GET()==200) {
    int len=http.getSize();
    if(len>0 && len<=512*1024) {
      uint8_t *buf=(uint8_t*)ps_malloc(len);
      if(buf) {
        size_t got=http.getStream().readBytes(buf,len);
        if(!logoCanvas.width()) { logoCanvas.setColorDepth(16); logoCanvas.createSprite(94,94); }
        if(got==(size_t)len && logoCanvas.width()) {
          logoCanvas.fillSprite(UI_BG);
          logoOK=logoCanvas.drawPng(buf,len,0,0,94,94,0,0,0.0f,0.0f);
          if(logoOK) { saveCachedLogo(path); logoDirty=false; }
        }
        free(buf);
      }
    }
  }
  http.end();
}
static void clearLogoCache() {
  if(!cacheReady) { heading("Cache unavailable"); delay(1200); return; }
  heading("Clear logo cache?");
  keyBox(0,100,160,60,"CANCEL"); keyBox(160,100,160,60,"CLEAR");
  int x,y; tap(x,y); if(x<160 || y<100 || y>=160) return;
  File root=SPIFFS.open("/");
  File file=root.openNextFile();
  while(file) {
    String name=file.name(); file.close();
    if(!name.startsWith("/")) name="/"+name;
    if((name.startsWith("/l") && name.endsWith(".raw") && name.length()==22) || name=="/logo.tmp") SPIFFS.remove(name);
    file=root.openNextFile();
  }
  root.close(); logoDirty=true; logoOK=false; logoRetryAt=0;
}
static void chooseStationMenu() {
  int page=0; bool onlyFav=false;
  while(true) {
    heading(onlyFav?"Favorites":"Radio stations");
    int ids[40], n=0;
    for(int i=0;i<stationCount;i++) if(!onlyFav || (favorites&(1ULL<<i))) ids[n++]=i;
    for(int i=0;i<4 && page+i<n;i++) keyBox(4,35+i*40,312,40,stationNames[ids[page+i]]);
    keyBox(0,200,65,40,"PREV"); keyBox(65,200,65,40,"NEXT"); keyBox(130,200,110,40,onlyFav?"ALL":"FAVORITES"); keyBox(240,200,80,40,"BACK");
    int x,y; tap(x,y);
    if(y>=200) { if(x<65) page=max(0,page-4); else if(x<130) {if(page+4<n) page+=4;} else if(x<240) {onlyFav=!onlyFav;page=0;} else return; }
    else if(y>=35 && y<195 && page+(y-35)/40<n) { selectStation(ids[page+(y-35)/40]-stationIndex); return; }
  }
}
static void applyBrightness() { M5.Display.setBrightness(screenOff || standby?0:(nightMode?nightBrightness:brightness)); }
static void savePower() {
  prefs.begin("awc-radio",false);
  prefs.putInt("bright",brightness); prefs.putInt("nightbright",nightBrightness);
  prefs.putInt("timeout",screenTimeout); prefs.putBool("car",carMode); prefs.putBool("night",nightMode);
  prefs.end();
}
static void otaMenu() {
  if(WiFi.status()!=WL_CONNECTED || M5.Power.getVBUSVoltage()<4400) {
    heading("OTA needs WiFi + USB"); delay(2000); return;
  }
  if(!esp_ota_get_next_update_partition(nullptr)) { heading("No OTA partition"); delay(2000); return; }
  bool wasPlaying=playing; playing=false; ++generation; clearAudio();
  char pass[17]; snprintf(pass,sizeof(pass),"%08lx%08lx",(unsigned long)esp_random(),(unsigned long)esp_random());
  WebServer ota(8080);
  bool authorized=false, started=false, good=false, restart=false;
  size_t received=0;
  ota.on("/",HTTP_GET,[&]() {
    if(!ota.authenticate("awc",pass)) { ota.requestAuthentication(); return; }
    ota.send(200,"text/html","<!doctype html><meta name='viewport' content='width=device-width'><h2>AWC Radio firmware update</h2><p>Keep USB connected. Select firmware.bin only, not the merged full image.</p><form method='POST' action='/update' enctype='multipart/form-data'><input type='file' name='firmware' accept='.bin' required><button>Update</button></form>");
  });
  ota.on("/update",HTTP_POST,[&]() {
    if(!ota.authenticate("awc",pass)) { ota.requestAuthentication(); return; }
    ota.send(good?200:400,"text/plain",good?"Update successful. Restarting.":"Update failed. Existing firmware retained.");
    restart=good;
  },[&]() {
    HTTPUpload &u=ota.upload();
    if(u.status==UPLOAD_FILE_START) {
      authorized=ota.authenticate("awc",pass);
      good=false; received=0;
      started=authorized && u.filename=="firmware.bin" && M5.Power.getVBUSVoltage()>=4400 && Update.begin(UPDATE_SIZE_UNKNOWN,U_FLASH);
    } else if(u.status==UPLOAD_FILE_WRITE && started) {
      if(M5.Power.getVBUSVoltage()<4000 || Update.write(u.buf,u.currentSize)!=u.currentSize) { Update.abort(); started=false; }
      else received+=u.currentSize;
    } else if(u.status==UPLOAD_FILE_END && started) {
      good=received>0 && Update.end(true); if(!good) Update.abort(); started=false;
    } else if(u.status==UPLOAD_FILE_ABORTED && started) { Update.abort(); started=false; }
  });
  ota.begin();
  heading("Firmware update");
  M5.Display.setTextSize(1); M5.Display.setCursor(8,45);
  M5.Display.println("Open in browser (same WiFi):");
  M5.Display.printf("http://%s:8080\n",WiFi.localIP().toString().c_str());
  M5.Display.printf("User: awc\nPassword: %s\n",pass);
  M5.Display.println("Upload firmware.bin only.");
  M5.Display.println("Keep USB power connected.");
  keyBox(4,190,312,45,"CANCEL");
  unsigned long began=millis();
  while(millis()-began<600000 || started) {
    ota.handleClient(); M5.update();
    if(restart) { delay(700); ESP.restart(); }
    if(!started && (WiFi.status()!=WL_CONNECTED || M5.Power.getVBUSVoltage()<4000)) break;
    if(!started && M5.Touch.getCount()) { auto t=M5.Touch.getDetail(0); if(t.wasPressed() && t.y>=190) break; }
    delay(10);
  }
  if(started) Update.abort();
  ota.stop(); playing=wasPlaying; ++generation; clearAudio(); lastTouch=millis();
}
static void diagnosticsMenu() {
  heading("Diagnostics v0.5");
  M5.Display.setTextSize(1); M5.Display.setCursor(8,38);
  M5.Display.printf("IP: %s\nRSSI: %d dBm\nHeap: %u bytes\nPSRAM: %u bytes\nUptime: %lu s\nUSB: %d mV\nBattery: %d%%\n",WiFi.localIP().toString().c_str(),WiFi.RSSI(),ESP.getFreeHeap(),ESP.getFreePsram(),millis()/1000,M5.Power.getVBUSVoltage(),M5.Power.getBatteryLevel());
  M5.Display.printf("Stream resets: %lu\nWatchdog: %s\n",(unsigned long)streamRecoveries,streamWatchdogReady?"ON":"OFF");
  M5.Display.printf("Underruns: %lu\nDropped frames: %lu\n",(unsigned long)underruns,(unsigned long)droppedFrames);
  keyBox(4,190,312,45,"BACK"); int x,y; do {tap(x,y);} while(y<190);
}
static void volumeMenu() {
  while(true) {
    heading("Audio volume");
    M5.Display.setTextSize(2); M5.Display.setCursor(12,45);
    M5.Display.printf("%d%% %s",(int)audioVolume,audioMuted?"MUTED":"");
    M5.Display.drawRect(12,80,296,16,TFT_WHITE);
    M5.Display.fillRect(14,82,292*audioVolume/100,12,TFT_ORANGE);
    keyBox(4,110,100,60,"VOL -"); keyBox(110,110,100,60,audioMuted?"UNMUTE":"MUTE"); keyBox(216,110,100,60,"VOL +");
    keyBox(4,190,312,45,"BACK");
    int x,y; tap(x,y);
    if(y>=190) { lastTouch=millis(); return; }
    if(y>=70 && y<105) audioVolume=constrain((x-12)*100/296,0,100);
    else if(y>=110 && y<170) {
      if(x<108) audioVolume=max(0,(int)audioVolume-5);
      else if(x<214) audioMuted=!audioMuted;
      else audioVolume=min(100,(int)audioVolume+5);
    }
    prefs.begin("awc-radio",false); prefs.putInt("volume",audioVolume); prefs.putBool("muted",audioMuted); prefs.end();
  }
}
static void systemMenu() {
  while(true) {
    heading("System settings");
    keyBox(4,45,312,42,"WiFi / Bluetooth / Server setup");
    keyBox(4,90,312,42,"OTA firmware update");
    keyBox(4,135,150,42,"Diagnostics"); keyBox(164,135,150,42,"Clear logo cache");
    keyBox(4,190,312,45,"BACK");
    int x,y; tap(x,y);
    if(y>=190) return;
    if(y>=45 && y<87) { prefs.begin("awc-radio",false); prefs.putBool("setup",true); prefs.end(); ESP.restart(); }
    if(y>=90 && y<132) otaMenu();
    if(y>=135 && y<177) { if(x<160) diagnosticsMenu(); else clearLogoCache(); }
  }
}
static void powerMenu() {
  while(true) {
    heading("Power settings");
    M5.Display.setTextSize(1); M5.Display.setCursor(8,32);
    M5.Display.printf("Battery %d%%  USB %d mV",M5.Power.getBatteryLevel(),M5.Power.getVBUSVoltage());
    keyBox(4,48,312,32,String("Car mode: ")+(carMode?"ON":"OFF"));
    keyBox(4,82,312,32,String("Display: ")+(nightMode?"NIGHT":"DAY"));
    keyBox(4,116,150,32,"DIM -"); keyBox(164,116,150,32,"BRIGHT +");
    keyBox(4,150,312,32,String("Screen timeout: ")+(screenTimeout?String(screenTimeout)+"s":"OFF"));
    keyBox(4,190,312,45,"BACK");
    int x,y; tap(x,y);
    if(y>=190) { lastTouch=millis(); return; }
    if(y>=48 && y<80) { carMode=!carMode; seenUSB=false; usbLostAt=0; }
    else if(y>=82 && y<114) nightMode=!nightMode;
    else if(y>=116 && y<148) { int &level=nightMode?nightBrightness:brightness; level=constrain(level+(x<160?-15:15),15,255); }
    else if(y>=150 && y<182) screenTimeout=screenTimeout==0?30:screenTimeout==30?60:screenTimeout==60?120:0;
    applyBrightness(); savePower();
  }
}
static void powerTick() {
  unsigned long now=millis();
  if(now-lastPowerPoll<1000) return;
  lastPowerPoll=now;
  int mv=M5.Power.getVBUSVoltage();
  if(carMode && mv>=0) {
    if(mv>=4400) {
      seenUSB=true; usbLostAt=0;
      if(standby) {
        standby=false; screenOff=false; lastTouch=now;
        WiFi.mode(WIFI_STA); connectSavedWifi(false);
        playing=resumePlaying; ++generation; clearAudio(); applyBrightness();
        if(btEnabled && btStartedForAudio && selectedBT>=0 && hasAddr(savedBT[selectedBT].addr)) {
          bluetooth.set_auto_reconnect(savedBT[selectedBT].addr); bluetooth.connect_to(savedBT[selectedBT].addr); lastBTAttempt=now;
        }
        btRetryInterval=15000;
      }
    } else if(mv<4000 && seenUSB && !standby) {
      if(!usbLostAt) usbLostAt=now;
      if(now-usbLostAt>=10000) {
        resumePlaying=playing; playing=false; ++generation; clearAudio();
        standby=true; screenOff=true; applyBrightness();
        WiFi.disconnect(false); WiFi.mode(WIFI_OFF);
        receiverReady=false;
        if(btStartedForAudio) { bluetooth.set_auto_reconnect(false); bluetooth.disconnect(); }
      }
    }
  }
  if(!standby && !screenOff && screenTimeout && now-lastTouch>=1000UL*screenTimeout) { screenOff=true; applyBrightness(); }
}
static String clockText(int seconds) { char b[24]; snprintf(b,sizeof(b),"%d:%02d",seconds/60,seconds%60); return String(b); }
static uint64_t lastScreenSignature=0;
static void drawScreen(bool force=true) {
  auto &d=M5.Display;
  static M5Canvas screen(&M5.Display);
  if(!screen.width()) { screen.setColorDepth(16); screen.createSprite(320,240); }
  if(!screen.width()) return;
  if(screenOff || standby) return;
  fetchLogo();
  uint64_t sig=14695981039346656037ULL;
  auto mix=[&](uint64_t v){sig^=v; sig*=1099511628211ULL;};
  auto mixText=[&](const String &v){for(size_t i=0;i<v.length();i++) mix((uint8_t)v[i]); mix(256);};
  mix(WiFi.status()); mix(btStartedForAudio && bluetooth.is_connected()); mix(streamOK); mix(playing); mix(M5.Power.getBatteryLevel()); mix(M5.Power.isCharging()); mix(stationCount); mix(stationIndex); mix(logoOK); mix(audioVolume); mix(audioMuted); mix(favorites);
  mixText(title); mixText(artist); mixText(nextTitle);
  if(stationCount) mixText(stationNames[stationIndex]);
  mix((uint64_t)trackStart); mix((uint64_t)trackEnd);
  if(trackEnd>trackStart) mix((millis()-metadataAt)/1000);
  if(title.length()>17) mix((millis()/350)%(title.length()+4));
  if(!force && sig==lastScreenSignature) return;
  lastScreenSignature=sig;
  auto &c=screen;
  c.fillSprite(UI_BG);
  c.setTextColor(WiFi.status()==WL_CONNECTED?UI_GREEN:UI_GREY,UI_BG); c.setTextSize(1); c.setCursor(8,9); c.print("WiFi");
  c.setTextColor(TFT_WHITE,UI_BG); c.setCursor(42,9); c.print("AWC RADIO");
  c.setTextColor(btStartedForAudio && bluetooth.is_connected()?UI_BLUE:UI_GREY,UI_BG); c.setCursor(151,9); c.print("BT");
  c.setTextColor(streamOK && playing?UI_GREEN:UI_GREY,UI_BG); c.setCursor(176,9); c.print(streamOK && playing?"LIVE":"IDLE");
  int bat=M5.Power.getBatteryLevel(); bool charging=M5.Power.isCharging();
  uint16_t bc=bat<15?TFT_RED:bat<30?UI_GOLD:UI_GREEN;
  c.drawRect(220,7,20,10,bc); c.fillRect(240,10,2,4,bc);
  c.fillRect(222,9,constrain(bat,0,100)*16/100,6,bc);
  c.setTextColor(bc,UI_BG); c.setCursor(245,9); if(charging)c.print("+"); if(bat>=0)c.printf("%d%%",bat); else c.print("--");
  c.setTextColor(UI_GREY,UI_BG); c.setCursor(296,9); c.print("SET");
  c.drawFastHLine(8,28,304,0x3186);
  if(logoOK) logoCanvas.pushSprite(&c,8,38);
  else { c.fillRoundRect(8,38,94,94,8,0x2104); c.setTextColor(UI_GOLD); c.setCursor(25,78); c.print("RADIO"); }
  c.setTextSize(1); c.setTextColor(UI_GREY,UI_BG); c.setCursor(110,43); c.print(stationCount?stationNames[stationIndex].substring(0,33):"Loading stations");
  c.setTextSize(2); c.setTextColor(TFT_WHITE,UI_BG); c.setCursor(110,64);
  const String &t=title;
  if(!t.length()) c.print("Waiting...");
  else if(t.length()<=17) c.print(t);
  else { size_t offset=(millis()/350)%(t.length()+4); for(size_t i=0;i<17;i++) { size_t j=(offset+i)%(t.length()+4); c.write(j<t.length()?(uint8_t)t[j]:(uint8_t)' '); } }
  c.setTextSize(1); c.setTextColor(UI_GREY,UI_BG); c.setCursor(110,98); c.print(artist.substring(0,33));
  c.setTextColor(UI_GOLD,UI_BG); c.setCursor(110,119); c.printf(audioMuted?"MUTED  (tap)":"VOL %d%%  (tap)",(int)audioVolume);
  int duration=max(0,(int)(trackEnd-trackStart));
  bool timed=duration>0 && time(nullptr)>1700000000;
  int elapsed=timed?constrain((int)(elapsedBase+(millis()-metadataAt)/1000),0,duration):0;
  c.fillRoundRect(8,140,304,4,2,0x3186);
  if(timed && elapsed) c.fillRoundRect(8,140,304*elapsed/duration,4,2,UI_GOLD);
  c.setCursor(8,151); c.print(timed?clockText(elapsed):"--:--");
  c.setCursor(276,151); c.print(duration?clockText(duration):"--:--");
  // Five large touch targets, matching the reference.
  c.fillTriangle(32,180,32,202,18,191,TFT_WHITE); c.fillRect(15,180,3,22,TFT_WHITE);
  bool fav=stationCount && (favorites&(1ULL<<stationIndex));
  for(int i=0;i<10;i++) { float a=-PI/2+i*PI/5,b=-PI/2+(i+1)*PI/5; int r=i%2?6:13,rr=(i+1)%2?6:13; c.drawLine(88+cos(a)*r,191+sin(a)*r,88+cos(b)*rr,191+sin(b)*rr,fav?UI_GOLD:UI_GREY); }
  c.fillCircle(160,191,21,UI_GOLD);
  if(playing) {c.fillRect(153,183,5,16,UI_BG);c.fillRect(162,183,5,16,UI_BG);} else c.fillTriangle(154,181,154,201,169,191,UI_BG);
  for(int i=0;i<3;i++) c.fillRoundRect(221,181+i*8,22-i*4,3,1,TFT_WHITE);
  c.fillTriangle(287,180,287,202,301,191,TFT_WHITE); c.fillRect(301,180,3,22,TFT_WHITE);
  c.setTextColor(UI_GREY,UI_BG); c.setCursor(8,224); c.print("Next up: "+nextTitle.substring(0,41));
  c.pushSprite(0,0);
}

void setup() {
  Serial.begin(115200);
  auto config = M5.config();
  M5.begin(config);
  M5.Display.setRotation(1);
  M5.Display.setBrightness(110);
  configureDevice();
  cacheReady=SPIFFS.begin(false);
  if(!cacheReady) Serial.println("Logo cache unavailable; using network artwork");
  prefs.begin("awc-radio",false); lastStation=prefs.getString("station",""); favorites=prefs.getULong64("favorites",0); brightness=prefs.getInt("bright",110); nightBrightness=prefs.getInt("nightbright",40); screenTimeout=prefs.getInt("timeout",60); carMode=prefs.getBool("car",false); nightMode=prefs.getBool("night",false); prefs.end();
  prefs.begin("awc-radio",false); audioVolume=constrain(prefs.getInt("volume",100),0,100); audioMuted=prefs.getBool("muted",false); prefs.end();
  applyBrightness(); lastTouch=millis();


  serverMutex=xSemaphoreCreateMutex();
  audioMutex = xSemaphoreCreateMutex();
  pcmRing = (Frame *)ps_malloc(RING_FRAMES * sizeof(Frame));
  if (!serverMutex || !audioMutex || !pcmRing) {
    Serial.println("FATAL: PSRAM/mutex allocation failed");
    M5.Display.println("PSRAM allocation failed");
    while (true) delay(1000);
  }
  WiFi.mode(WIFI_STA);
  WiFi.setSleep(false);
  connectSavedWifi(false);
  configTime(0,0,"pool.ntp.org","time.google.com");
  Serial.println("Connecting WiFi...");
  const unsigned long start = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - start < 15000) delay(250);
  if (WiFi.status() == WL_CONNECTED) { checkServers(true); refreshStations(); }

  bluetooth.set_data_callback_in_frames(bluetoothFrames);
  if(btEnabled) {
    bluetooth.set_ssid_callback(matchSaved);
    if(selectedBT>=0 && hasAddr(savedBT[selectedBT].addr)) bluetooth.set_auto_reconnect(savedBT[selectedBT].addr);
    else bluetooth.set_auto_reconnect(false);
    bluetooth.start(btName.c_str());
    btStartedForAudio=true;
  }
  playing = stationCount > 0;
  // Arduino ESP32 2.x (PlatformIO espressif32 6.12): task watchdog API.
  esp_task_wdt_init(30,true);
  xTaskCreatePinnedToCore(networkTask, "AWCStream", 12000, nullptr, 1, nullptr, 1);
  drawScreen();
}

void loop() {
  M5.update();
  powerTick();
  if(standby) { delay(100); return; }
  unsigned long now=millis();
  if(playing && btEnabled && !liveSwitching && WiFi.status()==WL_CONNECTED && stationCount && streamOK && now-lastAudioMillis>12000 && now-lastRecovery>15000) {
    lastRecovery=now; ++streamRecoveries; ++generation; clearAudio();
    Serial.println("Recovery: restarting stalled stream");
  }
  if(streamHeartbeat && now-streamHeartbeat>45000) { Serial.println("Recovery: stream task unresponsive"); ESP.restart(); }
  bool connected=btEnabled && btStartedForAudio && !liveSwitching && bluetooth.is_connected();
  if(connected!=receiverReady) {
    receiverReady=connected; ++generation; clearAudio();
    if(connected) { btConnectedAt=now; Serial.println("Bluetooth connected; buffering live audio"); }
    else { lastBTAttempt=now; Serial.println("Bluetooth disconnected; suspending stream"); }
  }
  if(connected && now-btConnectedAt>10000) btRetryInterval=15000;
  if(btEnabled && btStartedForAudio && selectedBT>=0 && hasAddr(savedBT[selectedBT].addr) && !connected && !bluetooth.is_discovery_active() && now-lastBTAttempt>=btRetryInterval) {
    lastBTAttempt=now; bluetooth.connect_to(savedBT[selectedBT].addr);
    btRetryInterval=min(60000UL,btRetryInterval*2);
  }
  checkServers();
  if (WiFi.status() != WL_CONNECTED) {
    static unsigned long lastWiFiRetry = 0;
    if (millis() - lastWiFiRetry > 15000 && millis()-wifiAttemptAt>=15000) {
      lastWiFiRetry = millis();
      WiFi.disconnect(false); connectSavedWifi(true);
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
      lastTouch=millis();
      if(screenOff) { screenOff=false; applyBrightness(); drawScreen(); return; }
      if(detail.y<30) {
        if(detail.x>=288 || detail.x<42) systemMenu();
        else if(detail.x>=215) powerMenu();
        else if(detail.x>=145 && detail.x<175) liveBluetooth();
      } else if(detail.y>=110 && detail.y<136 && detail.x>=105) volumeMenu();
      else if(detail.y>=170 && detail.y<216) {
        if(detail.x<56) selectStation(-1);
        else if(detail.x<120 && stationCount) { favorites^=1ULL<<stationIndex; prefs.begin("awc-radio",false); prefs.putULong64("favorites",favorites); prefs.end(); }
        else if(detail.x<200) { playing=!playing; ++generation; clearAudio(); }
        else if(detail.x<265) chooseStationMenu();
        else selectStation(1);
      }
      drawScreen();
    }
  }
  if (millis() - lastMetadata > 5000 && stationCount && WiFi.status()==WL_CONNECTED) {
    lastMetadata = millis();
    refreshMetadata();
  }
  if (!screenOff && millis() - lastScreen > 500) {
    lastScreen = millis();
    drawScreen(false);
  }
  delay(25);
}
