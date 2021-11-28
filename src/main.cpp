#include "SSD1306Wire.h"
#include "SdsDustSensor.h"
#include <ESP8266WiFi.h>
#include <MQTT.h>
#include "Wire.h"
#include "creds.h"
#include "font.h"


const int rxPin = D5;
const int txPin = D6;
const int btnLeft = D3;
const int btnRight = D7;
const int interval = 10 * 100 * 60; // 10 minutes

SSD1306Wire display(0x3c, SDA, SCL, GEOMETRY_128_32);
SdsDustSensor sds(rxPin, txPin);
WiFiClient wifi;
MQTTClient mqtt;

long i = 0;
float pm1 = 0.0;
float pm2 = 0.0;


void connect() {
  Serial.print("\nConnecting wifi...");
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(1000);
  }
  Serial.println();
  while (!mqtt.connect("drsSmogMiarka", mqtt_user, mqtt_pass, false)) {
    Serial.print(".");
    delay(1000);
  }
}

void drawText(int x, int y, String s) {
  display.drawStringMaxWidth(x, y, 128, s);
}

void splash() {
  display.clear();
  drawText(0, 0, "Smogomiarka");
  display.setFont(ArialMT_Plain_10);
  drawText(0, 20,"by SP5DRS");
  display.display();
}

void showIP() {
  display.clear();
  drawText(0, 0, "WIFI address:");
  display.setFont(ArialMT_Plain_16);
  drawText(0, 17, WiFi.localIP().toString());
  display.display();
}


void measInProgress() {
  display.clear();
  display.setFont(ArialMT_Plain_16);
  drawText(0, 10, "Pomiar...");
  display.display();
}

void mqttInProgress() {
  display.clear();
  display.setFont(ArialMT_Plain_16);
  drawText(0, 10, "Wysylanie...");
  display.display();
}


void displayResult() {
  display.clear();
  display.setFont(Roboto_Mono_10);
  display.setTextAlignment(TEXT_ALIGN_LEFT);
  drawText(0, 0, "PM 2.5"); drawText(55, 0, String(pm2));
  drawText(0, 20, "PM 10");  drawText(55,20, String(pm1));
  display.display();
}

void mqttPublish(){
    mqttInProgress();
    if (!mqtt.connected()) connect();
    mqtt.loop();
    Serial.println("Publishing MQTT message...");
    mqtt.publish(mqtt_topic, "{\"pm25\": " + String(pm2) + ",\"pm10\": " + String(pm1) + "}");
}

void makeMeas() {
  bool success = false;
  sds.wakeup();
  measInProgress();
  delay(30000); // warm up
  PmResult pm = sds.queryPm();
  pm1 = pm.pm10;
  pm2 = pm.pm25;
  success = pm.isOk();

  if (success){
    mqttPublish();
    display.clear();
    display.display();
  } else {
    Serial.println("No measurement");
  }

  WorkingStateResult state = sds.sleep();
  if (state.isWorking()) {
    Serial.println("Problem with sleeping the sensor.");
  } else {
    Serial.println("Sensor is sleeping");
  }
}


void setup() {
  i = 0;
  pinMode(btnLeft, INPUT_PULLUP);
  pinMode(btnRight, INPUT_PULLUP);
  Serial.begin(115200);
  Serial.println();

  display.init();
  display.flipScreenVertically();
  display.setFont(ArialMT_Plain_16);
  display.setTextAlignment(TEXT_ALIGN_LEFT);

  sds.begin();
  sds.setQueryReportingMode();
  Wire.begin();
  Wire.setClock(100000);

  splash();

  WiFi.begin(ssid, pass);
  mqtt.begin(mqtt_server, wifi);
  connect();

  showIP();
  delay(2000);
  display.clear();
  display.display();
  makeMeas();
}


void loop() {
  if (i>=interval) {
    makeMeas();
    i = 0;
  }

  if (!digitalRead(btnLeft)) makeMeas();
  if (!digitalRead(btnRight)) {
    displayResult();
    delay(3000);
  } else {
    display.clear();
    display.display();
  }
  delay(100);
  i++;
}
