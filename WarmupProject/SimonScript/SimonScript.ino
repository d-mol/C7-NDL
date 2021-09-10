#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Wire.h>
#include "paj7620.h"
#include <LOLIN_I2C_MOTOR.h>
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>

/*

#define KEYA D3
#define KEYB D4
#define OLEDRESET 1 // GPIO1
Adafruit_SSD1306 display(OLEDRESET);
void setup() {
  display.begin(SSD1306_SWITCHCAPVCC, 0x3C); // i n i t i al i ze with the I2C address
  delay(1000);
  pinMode(KEYA, INPUT);
  pinMode(KEYB, INPUT);
}

void loop() {
  display.clearDisplay( );
  display.setCursor(0, 0);
  display.setTextColor(WHITE);
  display.setTextSize(2);
  display.println("NDL");
  display.setTextSize(1);
  display.print("Key A = ");
  display.println(digitalRead(KEYA) );
  display.print("Key B = ");
  display.println(digitalRead(KEYB) );
  display.display( );
  delay(200);
}*/

/*
#define GES_REACTION_TIME 500
#define GES_ENTRY_TIME 800
#define GES_QUIT_TIME 1000
#define I2C_ADDRESS 0x43
#define I2C_ADDRESS2 0x44
void setup() {
  Serial.begin(9600);
  uint8_t error = paj7620Init( ); // i n i t i al i ze Paj7620 reg isters
  if (error) {
    Serial.print("INIT ERROR,CODE: ");
  Serial.println(error);
  }
}

void loop() {
  uint8_t data = 0, data1 = 0, error;
  error = paj7620ReadReg(I2C_ADDRESS, 1, &data); // Read gesture resul t .
  if (!error) {
    switch (data) {
      case GES_RIGHT_FLAG:
        Serial.println("Right");
        break;
      case GES_LEFT_FLAG:
        Serial.println("Left");
        break;
      case GES_UP_FLAG:
        Serial.println("Up");
        break;
      case GES_DOWN_FLAG:
        Serial.println("Down");
        break;
      case GES_FORWARD_FLAG:
        Serial.println("Forward");
        delay(GES_QUIT_TIME);
        break;
      case GES_BACKWARD_FLAG:
        Serial.println("Backward");
        delay(GES_QUIT_TIME);
        break;
      case GES_CLOCKWISE_FLAG:
        Serial.println("Clockwise");
        break;
      case GES_COUNT_CLOCKWISE_FLAG:
        Serial.println("anti-clockwise");
        break;
      default:
        paj7620ReadReg(I2C_ADDRESS2, 1, &data);
        if (data == GES_WAVE_FLAG) {
          Serial.println("wave");
        } else {
          Serial.print(".");
        }
        break;
      }
  }
  delay(100);
}*/


#define LISTEN_PORT 80
#define PWMFREQUENCY 1000

LOLIN_I2C_MOTOR motor(DEFAULT_I2C_MOTOR_ADDRESS); //I2C address 0x30 SEE NOTEBELOW
/*const char* ssid = "ASUS_NDL"; // name of local WiFi network in the NDL
const char* password = "RT-AC66U"; // the password of the WiFi network*/
const char* ssid = "NDL_24G"; // name of local WiFi network in the NDL
const char* password = "RT-AC66U"; // the password of the WiFi network
MDNSResponder mdns;
ESP8266WebServer server(LISTEN_PORT);
String startPage = "<html><head><title>Simon commands</title></head><body><a href=\"begin\"><button style=\"background-color:blue;color:white;\">Begin</button></a></body></html>";
bool ledOn = false;

void setup() {
  Serial.begin(115200); // the serial speed
  pinMode(LED_BUILTIN, OUTPUT); // the LED
  digitalWrite(LED_BUILTIN, ledOn); // the actual status is inverted
  webPage += "<html><head><title>Simon commands</title></head><body><a id=\"startButton\" href=\"start\"><button style=\"background-color:blue;color:white;\">";
  webPage += "Begin</button></a><script>let instructions="+instructions+".split(\"-\"); let instDelay = 1500;showNextInstruction(0);";
  webPage += "function showNextInstruction(i){if (i==instructions.length)return;let instEl = document.createElement(\"h1\");instEl.innerText = instructions[i];";
  webPage += "document.body.appendChild(instEl);setTimeout(() => {document.body.removeChild(instEl);showNextInstruction(i+1);}, instDelay);}</script></body></html>";
  WiFi.begin(ssid, password); // make the WiFi connection
  Serial.println("Start connecting.");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.print("Connected to ");
  Serial.print(ssid);
  Serial.print(". IP address: ");
  Serial.println(WiFi.localIP( ) );
  if (mdns.begin("esp8266" , WiFi.localIP())) {
    Serial.println("MDNS responder started");
  }
  // make handlers for input from WiFi connection
  server.on("/" , [ ](){
    server.send(200, "text/html" , webPage);
  });

  server.on("/button" , [ ](){
    server.send(200, "text/html" , webPage);
    ledOn = !ledOn;
    Serial.print("led ");
    Serial.println(ledOn);
    digitalWrite(LED_BUILTIN, !ledOn);
    if (ledOn)
      moveMotor(75);
    else
      stopMotor();
    delay(100);
  });

  waitUntilMotorStart();
  server.begin( ); // star t the server for WiFi input
  Serial.println("HTTP server started");
}
void loop() {
  server.handleClient();
}

String generatePage() {
  String newPage = "<html><head><title>Simon commands</title></head><body></body><script>let instructions="+generateInstructions()+".split(\"-\");";
  newPage += "let instDelay = 1500;showNextInstruction(0);function showNextInstruction(i){if (i==instructions.length)return;let instEl = document.createElement(\"h1\");";
  newPage += "instEl.innerText = instructions[i];document.body.appendChild(instEl);setTimeout(() => {document.body.removeChild(instEl);showNextInstruction(i+1);}, instDelay);}</script></html>";
  return newPage;
}

String generateInstructions() {
  
}

void waitUntilMotorStart() {
  while (motor.PRODUCT_ID != PRODUCT_ID_I2C_MOTOR) { //wait motor shield ready .
    motor.getInfo();
  }
}

void moveMotor(int duty) {
  motor.changeFreq(MOTOR_CH_BOTH, PWMFREQUENCY);
  motor.changeStatus(MOTOR_CH_A, MOTOR_STATUS_CCW);
  motor.changeDuty(MOTOR_CH_A, duty);
}

void stopMotor() {
  motor.changeStatus(MOTOR_CH_A, MOTOR_STATUS_STANDBY);
}
