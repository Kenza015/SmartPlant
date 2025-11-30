// ===============================
//  TAB5 — AP + /api/soil + UI horizontale (paysage)
// ===============================
#include <M5Unified.h>
#include <WiFi.h>
#include <WebServer.h>
#include "esp_wifi.h"

// ---------- Config Wi-Fi AP ----------
const char* AP_SSID = "ESP32-GW";
const char* AP_PASS = "12345678";
const int   AP_CH   = 6;    // canal Wi-Fi

WebServer server(80);

// ---------- Mesures reçues ----------
volatile int   lastRaw   = -1;
volatile float lastPct   = 0.0f;
String         lastState = "INCONNU";

// =====================================================
//                    UI GRAPHIQUE
// =====================================================
void drawFrame() {
  // Orientation paysage
  M5.Lcd.setRotation(1);
  M5.Lcd.fillScreen(TFT_BLACK);

  // Bandeau titre
  M5.Lcd.fillRect(0, 0, 320, 36, TFT_DARKGREEN);
  M5.Lcd.setTextColor(TFT_WHITE, TFT_DARKGREEN);
  M5.Lcd.setTextSize(2);
  M5.Lcd.setCursor(10, 10);
  M5.Lcd.print(" SMART PLANT (AP)");

  // Séparateur vertical
  M5.Lcd.drawFastVLine(180, 40, 160, TFT_DARKGREY);

  // Pied de page (infos Wi-Fi)
  M5.Lcd.fillRect(0, 200, 320, 40, TFT_DARKGREY);
  M5.Lcd.setTextColor(TFT_WHITE, TFT_DARKGREY);
  M5.Lcd.setTextSize(1);
  M5.Lcd.setCursor(10, 210);
  M5.Lcd.printf("SSID: %s", AP_SSID);
  M5.Lcd.setCursor(10, 222);
  M5.Lcd.printf("IP:   %s", WiFi.softAPIP().toString().c_str());
}

void drawValues() {
  const int leftX = 10;
  const int leftY = 50;
  const int leftW = 160;
  const int barH = 24;

  const int rightX = 190;
  const int rightY = 50;

  // Couleur selon l'humidite
  uint16_t color;
  if (lastPct < 30) {
    color = TFT_RED;
  } else if (lastPct < 60) {
    color = TFT_YELLOW;
  } else {
    color = TFT_GREEN;
  }

  // --------- Panneau gauche : % + jauge ---------
  M5.Lcd.fillRect(leftX, leftY, leftW, 130, TFT_BLACK);

  M5.Lcd.setTextColor(TFT_CYAN, TFT_BLACK);
  M5.Lcd.setTextSize(2);
  M5.Lcd.setCursor(leftX, leftY);
  M5.Lcd.print("Humidite");

  M5.Lcd.setTextSize(4);
  M5.Lcd.setCursor(leftX, leftY + 28);
  M5.Lcd.setTextColor(color, TFT_BLACK);
  M5.Lcd.printf("%.1f %% ", lastPct);

  // Jauge horizontale
  int filled = map((int)lastPct, 0, 100, 0, leftW);
  if (filled < 0) filled = 0;
  if (filled > leftW) filled = leftW;
  M5.Lcd.drawRect(leftX, leftY + 90, leftW, barH, TFT_WHITE);
  if (filled > 2) {
    M5.Lcd.fillRect(leftX + 1, leftY + 91, filled - 2, barH - 2, color);
  }

  // --------- Panneau droit : état + RAW ---------
  M5.Lcd.fillRect(rightX, rightY, 140, 130, TFT_BLACK); 

  M5.Lcd.setTextSize(2);
  M5.Lcd.setTextColor(TFT_WHITE, TFT_BLACK);
  M5.Lcd.setCursor(rightX, rightY);
  M5.Lcd.print("Etat");

  M5.Lcd.setTextSize(4);
  M5.Lcd.setCursor(rightX, rightY + 28);
  M5.Lcd.setTextColor(color, TFT_BLACK);
  M5.Lcd.print(lastState);

  M5.Lcd.setTextSize(2);
  M5.Lcd.setCursor(rightX, rightY + 90);
  M5.Lcd.setTextColor(TFT_LIGHTGREY, TFT_BLACK);
  M5.Lcd.printf("RAW: %d   ", lastRaw);
}

// =====================================================
//                    HTTP HANDLERS
// =====================================================
void handleRoot() {
  String s =
    "<!doctype html><html><head><meta charset='utf-8'>"
    "<meta name='viewport' content='width=device-width,initial-scale=1'>"
    "<style>"
    "body{font-family:Arial;padding:16px;background:#0b0e0c;color:#fff}"
    ".card{background:#121614;border:1px solid #1f2621;"
    "border-radius:12px;padding:16px;max-width:420px}"
    "a{color:#7fd1ff;text-decoration:none}"
    "</style></head><body>";

  s += "<div class='card'><h2>Smart Plant (AP)</h2>";
  s += "<p><b>IP:</b> " + WiFi.softAPIP().toString() + "<br>";
  s += "<b>Test:</b> <a href='/ping'>/ping</a></p>";
  s += "<p><b>RAW:</b> " + String(lastRaw) +
       "<br><b>Hum:</b> " + String(lastPct, 1) + " %"
       "<br><b>Etat:</b> " + lastState + "</p>";
  s += "<p><small>POST /api/soil JSON {raw,pct}</small></p>";
  s += "</div></body></html>";

  server.send(200, "text/html", s);
}

void handlePing() {
  server.send(200, "text/plain", "pong");
}

// Réception des mesures depuis l'ESP32 capteur
void handleSoil() {
  if (server.method() != HTTP_POST) {
    server.send(405, "text/plain", "Use POST");
    return;
  }

  String b = server.arg("plain"); // JSON brut envoyé par l'ESP32
  Serial.print("BODY: ");
  Serial.println(b);

  int rPos = b.indexOf("\"raw\":");
  int pPos = b.indexOf("\"pct\":");
  if (rPos < 0 || pPos < 0) {
    server.send(400, "text/plain", "Bad JSON");
    return;
  }

  int comma = b.indexOf(',', rPos);
  if (comma < 0) comma = b.length();
  lastRaw = b.substring(rPos + 6, comma).toInt();

  comma = b.indexOf(',', pPos);
  if (comma < 0) comma = b.length();
  lastPct = b.substring(pPos + 6, comma).toFloat();

  if (lastPct < 30) {
    lastState = "SEC";
  } else if (lastPct < 60) {
    lastState = "NORMAL";
  } else {
    lastState = "HUMIDE";
  }

  drawValues();
  server.send(200, "application/json", "{\"ok\":true}");
}

// =====================================================
//                    SETUP / LOOP
// =====================================================
void setup() {
  auto cfg = M5.config();
  M5.begin(cfg);

  M5.Speaker.end();          // couper le son
  M5.Lcd.setBrightness(50);  // luminosité modérée

  Serial.begin(115200);

  // Config Wi-Fi en mode AP
  WiFi.mode(WIFI_MODE_NULL);
  delay(100);
  WiFi.mode(WIFI_AP);
  WiFi.setSleep(true);
  esp_wifi_set_ps(WIFI_PS_MIN_MODEM);
  WiFi.setTxPower(WIFI_POWER_11dBm);

  bool ok = WiFi.softAP(AP_SSID, AP_PASS, AP_CH, 0, 4);
  Serial.print("AP start: ");
  Serial.println(ok ? "OK" : "FAIL");
  Serial.print("AP IP: ");
  Serial.println(WiFi.softAPIP());

  // Routes HTTP
  server.on("/", handleRoot);
  server.on("/ping", handlePing);
  server.on("/api/soil", HTTP_POST, handleSoil);
  server.begin();

  // UI initiale
  drawFrame();
  drawValues();
}

void loop() {
  server.handleClient();
  M5.update();
  delay(5);
}
