#include <ArduinoOTA.h>
class AppOta {

  #ifndef OTA_DEBUG
  #define OTA_DEBUG 0
  #endif

  AsyncWebServer *WebServer;
  Preferences *AppPreferences;

  public:
  
  bool polling = false;
  bool hosting = false;
  bool resetFlag = false;
  IPAddress IP;
  String ssid = "";
  String password = "";
  String hostName = "falcon-2-u";
  String apPass = "falcon2u";
  
  AppOta(AsyncWebServer *webServer, Preferences *preferences) {
    this->WebServer = webServer;
    this->AppPreferences = preferences;
  }
  
  void setup() {
    ssid = AppPreferences->getString("ssid", "");
    password = AppPreferences->getString("password", "");
    logf("ssid %s, pass %s", ssid, password);
  }

  bool begin() {
    if(polling || hosting){
      return true;
    }
    
    if(ssid == "") {
      if(!hosting){
        logln("Start Hosting..");
        hosting = true;
        WiFi.mode(WIFI_AP);
        if(WiFi.softAP(hostName, apPass)){
          IP = WiFi.softAPIP();
          log("AP IP address: ");
          logln(IP);
          startWebServer();
        } else {
          logln("failed to start wifi AP!");
          hosting = false;
          return false;
        }
      }
      return true;
    } else {
      logf("ssid %s", ssid);
    }
    
    polling = true;

    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid, password);

    while (WiFi.status() != WL_CONNECTED) {
      delay(500);
      log(".");
    }
    IP = WiFi.localIP();
    log("IP address: ");
    logln(IP);
    hosting = true;
    startWebServer();
    logln("HTTP server started");

  // Port defaults to 3232
  // ArduinoOTA.setPort(3232);

  // Hostname defaults to esp3232-[MAC]
  ArduinoOTA.setHostname(hostName.c_str());

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
    return true;
  }

  void loop() {
    if(polling) {
      ArduinoOTA.handle();
    }
  }

  void end() {
    stopWebServer();
    ArduinoOTA.end();
    WiFi.disconnect(true);
    WiFi.mode(WIFI_OFF);
    polling = false;
    hosting = false;
  }

  void stopWebServer() {
    WebServer->end();
  }
  void startWebServer() {
    WebServer->on("/", HTTP_GET, [&](AsyncWebServerRequest* request) {
      String body = String("<html><head></head><body><h1>Falcon 2 Universal Version: ");
      body.concat(AppPreferences->getString("version"));
      body.concat(String("</h1><p><a href='/wifi'>Wifi Config</a></p>"));

      request->send(200, "text/html", body);
    });
    WebServer->on("/wifi", HTTP_GET, [&](AsyncWebServerRequest* request) {
      String body = String("<html><head></head><body><h1>WiFi: Falcon 2 Universal Version: ");
      body.concat(AppPreferences->getString("version").concat("</h1>"));
      body.concat(String("<form method='post'><label for='ssid'>SSID:</label><input type='text' name='ssid' value='' /><label for='password'>Password:</label><input type='password' name='password' /><button type='submit'>Save</></form>"));
      request->send(200, "text/html", body);
    });
    WebServer->on("/wifi", HTTP_POST, [&](AsyncWebServerRequest* request) {
      if (request->hasParam("ssid", true) && request->hasParam("password", true)){
        String posted_ssid = request->getParam("ssid", true)->value();
        String posted_pass = request->getParam("password", true)->value();
        AppPreferences->putString("ssid", posted_ssid);
        AppPreferences->putString("password", posted_pass);
        log("pssid: ");
        log(posted_ssid);
        log(" ppass: ");
        logln(posted_pass);
        String body = String("<html><head></head><body><h1>WiFi: Falcon 2 Universal Version: ");
        body.concat(AppPreferences->getString("version").concat("</h1>"));
        body.concat(String("<form method='post'><input type='hidden' name='reset' value='true' /><h2>Config Saved!</h2><button type='submit'>Reboot</></form>"));

        request->send(200, "text/html", body);
      } else if (request->hasParam("reset", true)) {
        logln("restarting esp because of post request.");
        ESP.restart();
      } else {
        request->send(400, "text/plain", "Bad Request. must pass ssid and password");
      }
    });
    WebServer->begin();
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