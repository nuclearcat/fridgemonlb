#include <OneWire.h>
#include <DallasTemperature.h>
#include <WiFi.h>
#include <WiFiClient.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include "HttpsOTAUpdate.h"
#include <esp_pm.h>
#include <esp_wifi.h>
#include <esp_wifi_types.h>
#include "fridgemonlb_example.h"

#define FW_VERSION 9
#define UPDATE_INTERVAL 30000

// GPIO where the DS18B20 is connected to
const int oneWireBus = 12;

// Setup a oneWire instance to communicate with any OneWire devices
OneWire oneWire(oneWireBus);

// Pass our oneWire reference to Dallas Temperature sensor
DallasTemperature sensors(&oneWire);

volatile long last_disconnect = 0;
volatile bool sta_ok = false;

int updating = 0;
int badtemp = 0;
static HttpsOTAStatus_t otastatus;

// Specify your certificate root, if your website doesnt use letsencrypt
static const char *server_certificate = "-----BEGIN CERTIFICATE-----\n" \
                                        "MIIFYDCCBEigAwIBAgIQQAF3ITfU6UK47naqPGQKtzANBgkqhkiG9w0BAQsFADA/\n" \
                                        "MSQwIgYDVQQKExtEaWdpdGFsIFNpZ25hdHVyZSBUcnVzdCBDby4xFzAVBgNVBAMT\n" \
                                        "DkRTVCBSb290IENBIFgzMB4XDTIxMDEyMDE5MTQwM1oXDTI0MDkzMDE4MTQwM1ow\n" \
                                        "TzELMAkGA1UEBhMCVVMxKTAnBgNVBAoTIEludGVybmV0IFNlY3VyaXR5IFJlc2Vh\n" \
                                        "cmNoIEdyb3VwMRUwEwYDVQQDEwxJU1JHIFJvb3QgWDEwggIiMA0GCSqGSIb3DQEB\n" \
                                        "AQUAA4ICDwAwggIKAoICAQCt6CRz9BQ385ueK1coHIe+3LffOJCMbjzmV6B493XC\n" \
                                        "ov71am72AE8o295ohmxEk7axY/0UEmu/H9LqMZshftEzPLpI9d1537O4/xLxIZpL\n" \
                                        "wYqGcWlKZmZsj348cL+tKSIG8+TA5oCu4kuPt5l+lAOf00eXfJlII1PoOK5PCm+D\n" \
                                        "LtFJV4yAdLbaL9A4jXsDcCEbdfIwPPqPrt3aY6vrFk/CjhFLfs8L6P+1dy70sntK\n" \
                                        "4EwSJQxwjQMpoOFTJOwT2e4ZvxCzSow/iaNhUd6shweU9GNx7C7ib1uYgeGJXDR5\n" \
                                        "bHbvO5BieebbpJovJsXQEOEO3tkQjhb7t/eo98flAgeYjzYIlefiN5YNNnWe+w5y\n" \
                                        "sR2bvAP5SQXYgd0FtCrWQemsAXaVCg/Y39W9Eh81LygXbNKYwagJZHduRze6zqxZ\n" \
                                        "Xmidf3LWicUGQSk+WT7dJvUkyRGnWqNMQB9GoZm1pzpRboY7nn1ypxIFeFntPlF4\n" \
                                        "FQsDj43QLwWyPntKHEtzBRL8xurgUBN8Q5N0s8p0544fAQjQMNRbcTa0B7rBMDBc\n" \
                                        "SLeCO5imfWCKoqMpgsy6vYMEG6KDA0Gh1gXxG8K28Kh8hjtGqEgqiNx2mna/H2ql\n" \
                                        "PRmP6zjzZN7IKw0KKP/32+IVQtQi0Cdd4Xn+GOdwiK1O5tmLOsbdJ1Fu/7xk9TND\n" \
                                        "TwIDAQABo4IBRjCCAUIwDwYDVR0TAQH/BAUwAwEB/zAOBgNVHQ8BAf8EBAMCAQYw\n" \
                                        "SwYIKwYBBQUHAQEEPzA9MDsGCCsGAQUFBzAChi9odHRwOi8vYXBwcy5pZGVudHJ1\n" \
                                        "c3QuY29tL3Jvb3RzL2RzdHJvb3RjYXgzLnA3YzAfBgNVHSMEGDAWgBTEp7Gkeyxx\n" \
                                        "+tvhS5B1/8QVYIWJEDBUBgNVHSAETTBLMAgGBmeBDAECATA/BgsrBgEEAYLfEwEB\n" \
                                        "ATAwMC4GCCsGAQUFBwIBFiJodHRwOi8vY3BzLnJvb3QteDEubGV0c2VuY3J5cHQu\n" \
                                        "b3JnMDwGA1UdHwQ1MDMwMaAvoC2GK2h0dHA6Ly9jcmwuaWRlbnRydXN0LmNvbS9E\n" \
                                        "U1RST09UQ0FYM0NSTC5jcmwwHQYDVR0OBBYEFHm0WeZ7tuXkAXOACIjIGlj26Ztu\n" \
                                        "MA0GCSqGSIb3DQEBCwUAA4IBAQAKcwBslm7/DlLQrt2M51oGrS+o44+/yQoDFVDC\n" \
                                        "5WxCu2+b9LRPwkSICHXM6webFGJueN7sJ7o5XPWioW5WlHAQU7G75K/QosMrAdSW\n" \
                                        "9MUgNTP52GE24HGNtLi1qoJFlcDyqSMo59ahy2cI2qBDLKobkx/J3vWraV0T9VuG\n" \
                                        "WCLKTVXkcGdtwlfFRjlBz4pYg1htmf5X6DYO8A4jqv2Il9DjXA6USbW1FzXSLr9O\n" \
                                        "he8Y4IWS6wY7bCkjCWDcRQJMEhg76fsO3txE+FiYruq9RUWhiF1myv4Q6W+CyBFC\n" \
                                        "Dfvp7OOGAN6dEOM4+qR9sdjoSYKEBpsr6GtPAQw4dy753ec5\n" \
                                        "-----END CERTIFICATE-----\n";


void wifiOnConnect() {
  Serial.println("STA Connected");
  Serial.print("STA IPv4: ");
  Serial.println(WiFi.localIP());
  sta_ok = true;
}

void wifiOnDisconnect() {
  sta_ok = false;
  last_disconnect = millis();
  Serial.println("STA Disconnected");
  delay(1000);
  WiFi.begin(WIFI_SSID, WIFI_PASS);
}

void WiFiEvent(WiFiEvent_t event) {
  switch (event) {
    case SYSTEM_EVENT_AP_START:
      //can set ap hostname here
      //WiFi.softAPsetHostname(AP_SSID);
      //enable ap ipv6 here
      WiFi.softAPenableIpV6();
      break;

    case SYSTEM_EVENT_STA_START:
      //set sta hostname here
      //WiFi.setHostname(AP_SSID);
      break;
    case SYSTEM_EVENT_STA_CONNECTED:
      //enable sta ipv6 here
      WiFi.enableIpV6();
      break;
    case SYSTEM_EVENT_STA_GOT_IP:
      wifiOnConnect();
      break;
    case SYSTEM_EVENT_STA_DISCONNECTED:
      wifiOnDisconnect();
      break;
    default:
      break;
  }
}

void HttpEvent(HttpEvent_t *event)
{
  switch (event->event_id) {
    case HTTP_EVENT_ERROR:
      Serial.println("Http Event Error");
      break;
    case HTTP_EVENT_ON_CONNECTED:
      Serial.println("Http Event On Connected");
      break;
    case HTTP_EVENT_HEADER_SENT:
      Serial.println("Http Event Header Sent");
      break;
    case HTTP_EVENT_ON_HEADER:
      Serial.printf("Http Event On Header, key=%s, value=%s\n", event->header_key, event->header_value);
      break;
    case HTTP_EVENT_ON_DATA:
      break;
    case HTTP_EVENT_ON_FINISH:
      Serial.println("Http Event On Finish");
      break;
    case HTTP_EVENT_DISCONNECTED:
      Serial.println("Http Event Disconnected");
      break;
  }
}


int sendtemp(float temp) {
  WiFiClientSecure *client = new WiFiClientSecure;
  HTTPClient http;
  int return_code = 1;
  
  client->setInsecure();// FIXME
  if (http.begin(*client, IOT_URL)) {
    String post_payload;
    StaticJsonDocument<1000> doc;
    long myrssi = WiFi.RSSI();
    doc["temp"] = temp;
    doc["fw"] = FW_VERSION;
    doc["millis"] = millis();
    doc["uptime"] = time(NULL);
    doc["dis_ts"] = last_disconnect;
    doc["rssi"] = myrssi;
    serializeJson(doc, post_payload);
    int httpResponseCode = http.POST(post_payload);
    
    if (httpResponseCode == 200) {
      return_code = 0;
      //Serial.print(http.getString());
      DynamicJsonDocument doc(2048);
      deserializeJson(doc, http.getStream());
      Serial.println("DEBUG");
      if (doc["upgrade"].as<unsigned int>() == 1) {
          updating = 1;
          HttpsOTA.onHttpEvent(HttpEvent);
          Serial.println("Starting OTA");
          HttpsOTA.begin(UPGRADE_URL, server_certificate, false);
      }
    }
  }
   http.end();
    delete client;
   return return_code;
}

void setup() {
  // Start the Serial Monitor for debug
  Serial.begin(115200);
  // Give power for sensor
  pinMode(14, OUTPUT);
  digitalWrite(14, HIGH);
  // Start the DS18B20 sensor
  sensors.begin();
  WiFi.onEvent(WiFiEvent);

  
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  Serial.println("Connecting");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.print("Connected to WiFi network with IP Address: ");
  Serial.println(WiFi.localIP());

}

long last_post = 0;

void loop() {
  if (updating) {
    otastatus = HttpsOTA.status();
    if (otastatus == HTTPS_OTA_SUCCESS) {
      Serial.println("Firmware written successfully. Rebooting...");
      ESP.restart();
    } else if (otastatus == HTTPS_OTA_FAIL) {
      Serial.println("Firmware Upgrade Fail");
      updating = 0;
    }
    delay(500);
    return;
  }

  if (millis() > last_post && sta_ok) {
    sensors.requestTemperatures();
    float temperatureC = sensors.getTempCByIndex(0);
    // Ignore invalid temperature
    if (temperatureC > -100 || badtemp > 4) {
      sendtemp(temperatureC);
      last_post = millis() + UPDATE_INTERVAL;
      badtemp = 0;
    } else {
      badtemp++;
    }
  } else if (last_disconnect) {
    if (millis() - last_disconnect > 60000 && millis() > 60000) {
      ESP.restart();
    }
  }
}
