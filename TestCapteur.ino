// ===== ESP32 capteur : STA + POST /api/soil =====
#include <WiFi.h>
#include <HTTPClient.h>

const char* SSID = "ESP32-GW";
const char* PASS = "12345678";

const int SENSOR_PIN = 34;    // ADC1_CH6
int DRY_RAW = 3200;           
int WET_RAW = 1400;           

float rawToPct(int raw){
  float pct = 100.0f * (float)(DRY_RAW - raw) / (float)(DRY_RAW - WET_RAW);
  if (pct < 0) pct = 0; if (pct > 100) pct = 100;
  return pct;
}

void wifiConnect(){
  WiFi.mode(WIFI_STA);
  WiFi.setSleep(false);
  WiFi.begin(SSID, PASS);
  Serial.printf("Connexion a %s\n", SSID);
  int tries=0; while (WiFi.status()!=WL_CONNECTED && tries<60){ delay(250); Serial.print("."); tries++; }
  Serial.println(WiFi.status()==WL_CONNECTED ? "\nWiFi OK" : "\nWiFi FAIL");
  if (WiFi.status()==WL_CONNECTED) { Serial.print("IP STA: "); Serial.println(WiFi.localIP()); }
}

void setup(){
  Serial.begin(115200);
  analogSetPinAttenuation(SENSOR_PIN, ADC_11db);
  analogReadResolution(12);
  wifiConnect();
}

void loop(){
  if (WiFi.status() == WL_CONNECTED) {
    int raw = analogRead(SENSOR_PIN);
    float pct = rawToPct(raw);
    const char* state = (pct < 35.0f) ? "SEC" : "HUMIDE";

    HTTPClient http;
    http.begin("http://192.168.4.1/api/soil");
    http.addHeader("Content-Type", "application/json");

    String body = String("{\"raw\":") + raw +
                  ",\"pct\":" + String(pct,1) +
                  ",\"state\":\"" + state + "\"" +
                  ",\"ts\":" + String(millis()) + "}";
    int code = http.POST(body);
    Serial.printf("POST -> %d  %s\n", code, body.c_str());
    http.end();
  } else {
    Serial.println("Reconnexion WiFi..."); WiFi.reconnect();
  }
  delay(2000);
}
