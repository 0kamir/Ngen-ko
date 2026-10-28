#include "net.h"
#include "zones.h"
#include "web_page.h"
#include <WiFi.h>
#include <WebServer.h>
#include <ESPmDNS.h>
#include <ArduinoJson.h>

static WebServer server(80);
static bool apActive = false;
static bool wasConnected = false;
static unsigned long staStarted = 0;
static char netLine[17] = "WiFi apagado";

static void startAccessPoint() {
  WiFi.mode(settings.wifiSsid[0] ? WIFI_AP_STA : WIFI_AP);
  WiFi.softAP(AP_SSID, AP_PASSWORD);
  apActive = true;
  Serial.printf("Red propia \"%s\" activa en %s\n", AP_SSID, WiFi.softAPIP().toString().c_str());
}

static void stopAccessPoint() {
  WiFi.softAPdisconnect(true);
  WiFi.mode(WIFI_STA);
  apActive = false;
}

// -----------------------------------
// API
// -----------------------------------

static void sendJson(JsonDocument &doc, int code = 200) {
  String out;
  serializeJson(doc, out);
  server.send(code, "application/json", out);
}

static void sendError(int code, const char *msg) {
  JsonDocument doc;
  doc["error"] = msg;
  sendJson(doc, code);
}

static bool parseBody(JsonDocument &doc) {
  if (deserializeJson(doc, server.arg("plain"))) {
    sendError(400, "JSON invalido");
    return false;
  }
  return true;
}

static void handleState() {
  JsonDocument doc;
  doc["version"] = NGENKO_VERSION;
  doc["mode"] = settings.mode;
  doc["uptimeSec"] = millis() / 1000;

  if (timeIsValid()) {
    time_t now = time(nullptr);
    struct tm t;
    localtime_r(&now, &t);
    char buf[20];
    strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M", &t);
    doc["time"] = buf;
  }

  JsonObject wifi = doc["wifi"].to<JsonObject>();
  wifi["ssid"] = settings.wifiSsid;
  wifi["connected"] = WiFi.status() == WL_CONNECTED;
  wifi["ip"] = WiFi.localIP().toString();
  wifi["rssi"] = WiFi.RSSI();
  wifi["ap"] = apActive;

  JsonArray zs = doc["zones"].to<JsonArray>();
  for (int i = 0; i < NUM_ZONES; i++) {
    JsonObject z = zs.add<JsonObject>();
    z["on"] = zones[i].on;
    z["moisture"] = zones[i].moisture;
    z["mv"] = zones[i].rawMv;
    z["status"] = statusName(zones[i].status);
    z["onForSec"] = zones[i].on ? (millis() - zones[i].onSince) / 1000 : 0;
    z["threshold"] = settings.zones[i].threshold;
    z["enabled"] = settings.zones[i].enabled;
    z["airMv"] = settings.zones[i].airMv;
    z["waterMv"] = settings.zones[i].waterMv;
  }

  JsonObject cfg = doc["config"].to<JsonObject>();
  cfg["hysteresis"] = settings.hysteresis;
  cfg["maxRunMin"] = settings.maxRunMin;
  cfg["cooldownMin"] = settings.cooldownMin;
  cfg["oneZoneAtATime"] = settings.oneZoneAtATime;
  cfg["windowEnabled"] = settings.windowEnabled;
  cfg["windowStartHour"] = settings.windowStartHour;
  cfg["windowEndHour"] = settings.windowEndHour;
  cfg["tz"] = settings.tz;

  sendJson(doc);
}

static void handleMode() {
  JsonDocument doc;
  if (!parseBody(doc)) return;
  int m = doc["mode"] | -1;
  if (m < MODE_OFF || m > MODE_AUTO) return sendError(400, "modo invalido");
  setMode((Mode)m);
  handleState();
}

static void handleRelay() {
  JsonDocument doc;
  if (!parseBody(doc)) return;
  if (settings.mode != MODE_MANUAL) return sendError(409, "solo en modo manual");
  int zone = doc["zone"] | -1;
  if (zone == -1) {
    setAllRelays(doc["on"] | false);  // zone = -1: todas
  } else if (zone >= 0 && zone < NUM_ZONES) {
    setRelay(zone, doc["on"] | false);
  } else {
    return sendError(400, "zona invalida");
  }
  handleState();
}

static void handleConfig() {
  JsonDocument doc;
  if (!parseBody(doc)) return;

  settings.hysteresis = constrain(doc["hysteresis"] | settings.hysteresis, 0, 50);
  settings.maxRunMin = constrain(doc["maxRunMin"] | settings.maxRunMin, 0, 600);
  settings.cooldownMin = constrain(doc["cooldownMin"] | settings.cooldownMin, 0, 1440);
  settings.oneZoneAtATime = doc["oneZoneAtATime"] | settings.oneZoneAtATime;
  settings.windowEnabled = doc["windowEnabled"] | settings.windowEnabled;
  settings.windowStartHour = constrain(doc["windowStartHour"] | settings.windowStartHour, 0, 23);
  settings.windowEndHour = constrain(doc["windowEndHour"] | settings.windowEndHour, 0, 23);

  const char *tz = doc["tz"];
  if (tz && strlen(tz) < sizeof(settings.tz) && strcmp(tz, settings.tz) != 0) {
    strlcpy(settings.tz, tz, sizeof(settings.tz));
    setenv("TZ", settings.tz, 1);
    tzset();
  }

  JsonArray zs = doc["zones"];
  for (int i = 0; i < NUM_ZONES && i < (int)zs.size(); i++) {
    ZoneSettings &z = settings.zones[i];
    z.threshold = constrain(zs[i]["threshold"] | z.threshold, 0, 100);
    z.enabled = zs[i]["enabled"] | z.enabled;
  }

  settingsChanged();
  handleState();
}

// Guarda la lectura actual del sensor como punto "seco" (aire) o "mojado" (agua).
static void handleCalibrate() {
  JsonDocument doc;
  if (!parseBody(doc)) return;
  int zone = doc["zone"] | -1;
  const char *point = doc["point"] | "";
  if (zone < 0 || zone >= NUM_ZONES) return sendError(400, "zona invalida");

  uint16_t mv = zones[zone].rawMv;
  if (mv < SENSOR_MIN_VALID_MV || mv > SENSOR_MAX_VALID_MV) return sendError(409, "sensor sin lectura valida");

  if (strcmp(point, "air") == 0) settings.zones[zone].airMv = mv;
  else if (strcmp(point, "water") == 0) settings.zones[zone].waterMv = mv;
  else return sendError(400, "point debe ser air o water");

  settingsChanged();
  handleState();
}

static void handleWifi() {
  JsonDocument doc;
  if (!parseBody(doc)) return;
  const char *ssid = doc["ssid"] | "";
  const char *pass = doc["pass"] | "";
  if (strlen(ssid) == 0 || strlen(ssid) >= sizeof(settings.wifiSsid) ||
      strlen(pass) >= sizeof(settings.wifiPass)) {
    return sendError(400, "SSID o clave invalidos");
  }
  strlcpy(settings.wifiSsid, ssid, sizeof(settings.wifiSsid));
  strlcpy(settings.wifiPass, pass, sizeof(settings.wifiPass));
  settingsSaveNow();

  server.send(200, "application/json", "{\"ok\":true}");
  delay(500);
  ESP.restart();
}

// -----------------------------------

void netBegin() {
  WiFi.persistent(false);  // Las credenciales las guardamos nosotros
  WiFi.setHostname(MDNS_NAME);
  configTzTime(settings.tz, "pool.ntp.org", "time.google.com");

  if (settings.wifiSsid[0]) {
    WiFi.mode(WIFI_STA);
    WiFi.begin(settings.wifiSsid, settings.wifiPass);
    staStarted = millis();
    Serial.printf("Conectando a \"%s\"...\n", settings.wifiSsid);
  } else {
    startAccessPoint();
  }

  server.on("/", HTTP_GET, [] { server.send_P(200, "text/html", WEB_PAGE); });
  server.on("/api/state", HTTP_GET, handleState);
  server.on("/api/mode", HTTP_POST, handleMode);
  server.on("/api/relay", HTTP_POST, handleRelay);
  server.on("/api/config", HTTP_POST, handleConfig);
  server.on("/api/calibrate", HTTP_POST, handleCalibrate);
  server.on("/api/wifi", HTTP_POST, handleWifi);
  server.onNotFound([] { server.send(404, "text/plain", "No encontrado"); });
  server.begin();

  pinMode(STATUS_LED_PIN, OUTPUT);
}

void netLoop() {
  server.handleClient();

  bool connected = WiFi.status() == WL_CONNECTED;

  if (connected && !wasConnected) {
    Serial.printf("WiFi conectado: %s\n", WiFi.localIP().toString().c_str());
    MDNS.begin(MDNS_NAME);
    MDNS.addService("http", "tcp", 80);
    if (apActive) stopAccessPoint();
  }
  wasConnected = connected;

  // Si la red guardada no responde, abrimos la red propia para poder corregirla.
  // El ESP32 sigue reintentando la red guardada en segundo plano.
  if (!connected && !apActive && settings.wifiSsid[0] &&
      millis() - staStarted > WIFI_CONNECT_TIMEOUT_MS) {
    startAccessPoint();
  }

  if (connected) {
    snprintf(netLine, sizeof(netLine), "%s", WiFi.localIP().toString().c_str());
    digitalWrite(STATUS_LED_PIN, HIGH);
  } else if (apActive) {
    snprintf(netLine, sizeof(netLine), "WiFi: %s", AP_SSID);
    digitalWrite(STATUS_LED_PIN, (millis() / 500) % 2);  // Parpadeo = red propia
  } else {
    snprintf(netLine, sizeof(netLine), "WiFi conectando");
    digitalWrite(STATUS_LED_PIN, LOW);
  }
}

const char *networkLine() {
  return netLine;
}
