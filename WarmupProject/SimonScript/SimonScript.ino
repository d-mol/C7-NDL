#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Wire.h>
#include "paj7620.h"
#include <LOLIN_I2C_MOTOR.h>
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>

#define OLEDRESET 1 // GPIO1
Adafruit_SSD1306 display(OLEDRESET);

#define GES_REACTION_TIME 500
#define GES_ENTRY_TIME 800
#define GES_QUIT_TIME 1000
#define I2C_ADDRESS 0x43
#define I2C_ADDRESS2 0x44
#define LISTEN_PORT 80
#define PWMFREQUENCY 1000

LOLIN_I2C_MOTOR motor(DEFAULT_I2C_MOTOR_ADDRESS); //I2C address 0x30 SEE NOTEBELOW
/*const char* ssid = "ASUS_NDL"; // name of local WiFi network in the NDL
const char* password = "RT-AC66U"; // the password of the WiFi network*/
const char* ssid = "NDL_24G"; // name of local WiFi network in the NDL
const char* password = "RT-AC66U"; // the password of the WiFi network
MDNSResponder mdns;
ESP8266WebServer server(LISTEN_PORT);
String startPage = "<html><head><title>Simon commands</title></head><body><h3>Simon is impatient and has given up on asking politely!</h3><p>He now only commands; Do what he says and be rewarded!</p><a href=\"begin\"><button style=\"background-color:blue;color:white;\">Begin</button></a></body></html>";
String readyPage = "<html><head><title>Simon commands</title></head><body><a href=\"check\"><button style=\"background-color:blue;color:white;\">Check your sequence</button></a></body></html>";
String allInstructions[] = {"Simon says move to the right",
                              "Simon says move to the left",
                              "Simon says move up",
                              "Simon says move down",
                              "Simon says move forward",
                              "Simon says move backward",
                              "Simon says wave"
                              /*
                              ,
                              "Simon says move clockwise",
                              "Simon says move counterclockwise",
      
                              //
                              "Move to the right",
                              "Move to the left",
                              "Move clockwise",
                              "Move counterclockwise",
                              "Move up",
                              "Move down",
                              "Move forward",
                              "Move backward",
                              "Wave*/
                              };

String screenTitle = "";
bool simonWatching = false;

const int maxInstructions = 4;
const int totalInstructions = 7;
int randomisedInts[maxInstructions];
int performedInstructions[maxInstructions];
int performedInstructionCount = 0;
int amountOfInstructions;

void writeToScreen(String text, int fontsize) {
  display.clearDisplay();
  display.setCursor(0, 0);
  display.setTextColor(WHITE);
  if (screenTitle!="") {
    display.setTextSize(2);
    display.println(screenTitle);
  }
  display.setTextSize(fontsize);
  display.println(text);
  display.display();
}

void setup() {
  display.begin(SSD1306_SWITCHCAPVCC, 0x3C); //initialize with the I2C address
  Serial.begin(115200); // the serial speed
  writeToScreen("Booting", 2);
  pinMode(LED_BUILTIN, OUTPUT); // the LED
  digitalWrite(LED_BUILTIN, HIGH); // the actual status is inverted
  WiFi.begin(ssid, password); // make the WiFi connection
  Serial.println("Start connecting.");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.print("Connected to ");
  Serial.print(ssid);
  Serial.print(". IP address: ");
  Serial.println(WiFi.localIP());
  if (mdns.begin("esp8266" , WiFi.localIP())) {
    Serial.println("MDNS responder started");
  }
  
  server.on("/" , [ ](){
    server.send(200, "text/html" , startPage);
  });

  server.on("/begin" , [ ](){
    server.send(200, "text/html" , generateInstructionPage());
  });

  server.on("/ready" , [ ](){
    simonWatching = true;
    server.send(200, "text/html" , readyPage);
    digitalWrite(LED_BUILTIN, LOW);
  });

  server.on("/check" , [ ](){
    performedInstructionCount = 0;
    simonWatching = false;
    server.send(200, "text/html" , generateCheckPage());
    digitalWrite(LED_BUILTIN, LOW);
  });
  uint8_t error = paj7620Init( ); // initialize Paj7620 registers
  if (error) {
    Serial.print("INIT ERROR,CODE: ");
    Serial.println(error);
  }
  waitUntilMotorStart();
  server.begin(); // start the server for WiFi input
  Serial.println("HTTP server started");
  screenTitle = "State";
  writeToScreen("\nCommand: 0\nPoints: 0", 1);
}
void loop() {
  server.handleClient();
  if (simonWatching && performedInstructionCount < maxInstructions)
    handleGestures();
}

void handleGestures()
{
  uint8_t data = 0, data1 = 0, error;
  error = paj7620ReadReg(I2C_ADDRESS, 1, &data); // Read gesture result.
  if (!error) {
    switch (data) {
      case GES_RIGHT_FLAG:
        Serial.println("Right");
        performedInstructions[performedInstructionCount] = 0;
        performedInstructionCount++;
        break;
      case GES_LEFT_FLAG:
        Serial.println("Left");
        performedInstructions[performedInstructionCount] = 1;
        performedInstructionCount++;
        break;
      case GES_UP_FLAG:
        Serial.println("Up");
        performedInstructions[performedInstructionCount] = 2;
        performedInstructionCount++;
        break;
      case GES_DOWN_FLAG:
        Serial.println("Down");
        performedInstructions[performedInstructionCount] = 3;
        performedInstructionCount++;
        break;
      case GES_FORWARD_FLAG:
        Serial.println("Forward");
        performedInstructions[performedInstructionCount] = 4;
        performedInstructionCount++;
        delay(GES_QUIT_TIME);
        break;
      case GES_BACKWARD_FLAG:
        Serial.println("Backward");
        performedInstructions[performedInstructionCount] = 5;
        performedInstructionCount++;
        delay(GES_QUIT_TIME);
        break;
      case GES_CLOCKWISE_FLAG:
        Serial.println("Clockwise");
        performedInstructions[performedInstructionCount] = 7;
        performedInstructionCount++;
        break;
      case GES_COUNT_CLOCKWISE_FLAG:
        Serial.println("Counter Clockwise");
        performedInstructions[performedInstructionCount] = 8;
        performedInstructionCount++;
        break;
      default:
        paj7620ReadReg(I2C_ADDRESS2, 1, &data);
        if (data == GES_WAVE_FLAG) {
          Serial.println("Wave");
          performedInstructions[performedInstructionCount] = 6;
          performedInstructionCount++;
        } else {
          Serial.print(".");
        }
        break;
      }
  }
  delay(100);
}

String generateCheckPage() {
  String performed = "";
  String expected = "";
  for(int i=0; i<amountOfInstructions; i++){
    //expected += allInstructions[randomisedInts[i]];
    expected += (char) (randomisedInts[i]+48);
    if(i != amountOfInstructions)
      expected += "-";
  }

  for(int i=0; i<performedInstructionCount; i++){
    //performed += allInstructions[performedInstructions[i]];
    performed += (char) (performedInstructions[i]+48);
    if(i != performedInstructionCount)
      performed += "-";
  }
  return "<html><head><title>Simon commands</title></head><body><h1>"+expected+"</h1><h1>"+performed+"</h1><h1>"+expected.equals(performed)+"</h1></body></html>";
}

String generateInstructionPage() {
  return "<html><head><title>Simon commands</title></head><body><a href=\"ready\"><button style=\"background-color:blue;color:white;\">Ready</button></a></body><script>let instructions=\""+generateInstructions()+"\".split(\"-\"); let instDelay = 1500;showNextInstruction(0);function showNextInstruction(i){if (i==instructions.length)return;let instEl = document.createElement(\"h1\");instEl.innerText = instructions[i];document.body.appendChild(instEl);setTimeout(() => {document.body.removeChild(instEl);setTimeout(() => {showNextInstruction(i+1);}, 250)}, instDelay);}</script></html>";
}

String generateInstructions() {
  String instructionsString = "";
  amountOfInstructions = random(2, maxInstructions + 1);
  for(int i=0; i<amountOfInstructions; i++){
    int randInt = random(totalInstructions);
    randomisedInts[i] = randInt;
    instructionsString += allInstructions[randInt];
    if(i != amountOfInstructions)  
      instructionsString += "-";
  }
  return instructionsString; 
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
