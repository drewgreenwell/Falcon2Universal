#include <ArduinoOTA.h>
class AppOta {

  #ifndef OTA_DEBUG
  #define OTA_DEBUG 0
  #endif

  AsyncWebServer *WebServer;

  public:
  // todo: this will be populated by wifi portal
  const char* ssid = "";
  const char* password = "";
  
  AppOta(AsyncWebServer *webServer) {
    this->WebServer = webServer;
  }
  
  void setup() {
    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid, password);

    while (WiFi.status() != WL_CONNECTED) {
      delay(500);
      Serial.print(".");
    }
  WebServer->on("/", HTTP_GET, [](AsyncWebServerRequest* request) {
    String body = String("Falcon 2 Universal Version: ");
    body.concat(prefs.getString("version"));
    request->send(200, "text/plain", body);
  });
  WebServer->begin();
  logln("HTTP server started");

  // Port defaults to 3232
  // ArduinoOTA.setPort(3232);

  // Hostname defaults to esp3232-[MAC]
  ArduinoOTA.setHostname("falcon-2-u");

  // No authentication by default
  // ArduinoOTA.setPassword("admin");

  // Password can be set with it's md5 value as well
  // MD5(admin) = 21232f297a57a5a743894a0e4a801fc3
  // ArduinoOTA.setPasswordHash("21232f297a57a5a743894a0e4a801fc3");

  ArduinoOTA
    .onStart([]() {
    String type;
    if (ArduinoOTA.getCommand() == U_FLASH)
      type = "sketch";
    else // U_SPIFFS
      type = "filesystem";

    // NOTE: if updating SPIFFS this would be the place to unmount SPIFFS using SPIFFS.end()
    logln("Start updating " + type);
    })
    .onEnd([]() {
      logln("\nEnd");
    })
    .onProgress([](unsigned int progress, unsigned int total) {
      logf("Progress: %u%%\r", (progress / (total / 100)));
    })
    .onError([](ota_error_t error) {
      logf("Error[%u]: ", error);
      if (error == OTA_AUTH_ERROR) logln("Auth Failed");
      else if (error == OTA_BEGIN_ERROR) logln("Begin Failed");
      else if (error == OTA_CONNECT_ERROR) logln("Connect Failed");
      else if (error == OTA_RECEIVE_ERROR) logln("Receive Failed");
      else if (error == OTA_END_ERROR) logln("End Failed");
    });
    ArduinoOTA.begin();
  }

  void loop() {
    ArduinoOTA.handle();
  }

  void end() {
    WebServer->end();
    ArduinoOTA.end();
    WiFi.disconnect(true);
    WiFi.mode(WIFI_OFF);
  }

  template <typename T>
  static void log(const T& data) {
    #if OTA_DEBUG
    Serial.print(data);
    #endif
  }

  template <typename T, typename... Args>
  static void logf(const char* format, T first, Args... rest) {
    #if OTA_DEBUG
    static char buffer[256];
    snprintf(buffer, sizeof(buffer), format, first, rest...);
    Serial.print(buffer);
    #endif
  }

  template <typename T>
  static void logln(const T& data) {
    #if OTA_DEBUG
    Serial.println(data);
    #endif
  }
};