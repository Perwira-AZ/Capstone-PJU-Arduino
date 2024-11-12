#include <LoRa.h>
#include <SPI.h>

#define ss 5
#define rst 14
#define dio0 2

const int ldr1 = 13;
const int led1 = 27;
const String superBlock = "A";  // Define the superblock (first character of the block)
const String block = "AB";  // Define the block
const int nodeCount = 12;  // Define how many node in that block (including anchor and backup anchor)
bool nodesStatus[nodeCount] = {0};
int ldrValues[nodeCount] = {0};
int responseCode[nodeCount] = {0};
String command = "";
bool lastDarkStat = 0;
bool isAnchor = 0;

const int threshold = 500;  // Threshold for LDR to set the light on/off
const int ledThreshold = 3200;  // threshold for the light to be considered on        
const unsigned long timeout = 30000;  // 30 second timeout for response from node (it vary depends on the number of the lights)

unsigned long lastSendTime = 0;
bool waitingForResponse = false;

int ledIsOn = 0;

void setup() {
  Serial.begin(115200);
  delay(1000);
  
  pinMode(ldr1, INPUT);
  pinMode(led1, OUTPUT);

  LoRa.setPins(ss, rst, dio0);
  if (!LoRa.begin(915E6)) {  // LoRa frequency 915 MHz
    Serial.println("Starting LoRa failed!");
    while (1);
  } else {
    Serial.println("LoRa initialization done.");
  }

  // LoRa settings
  LoRa.setSpreadingFactor(12);
  LoRa.setSignalBandwidth(125E3);
  LoRa.setCodingRate4(8);
  LoRa.setPreambleLength(12);
  LoRa.setTxPower(20);
}

void loop() {
  // LoRa packet handling
  int packetSize = LoRa.parsePacket();
  if (packetSize) {
    String received = "";
    while (LoRa.available()) {
      received += (char)LoRa.read();
    }

    if(received.startsWith("PJU-3-"+block)){
      delay(500);
      // Handle the message based on the last character
      if (received.endsWith("-I")) {  
        delay(500);
        analogRead(ldr1) >= threshold ? ledIsOn = 1 : ledIsOn = 0;
        sendLoRa("PJU-1-" + block + "-" + String(nodeId) + "-" + String(ledIsOn) + "-" + String(analogRead(ldr1)) + "-S", "responding to anchor");
      } else if (received.endsWith("-H")) {
        // Turn on LED
        digitalWrite(led1, HIGH);
        analogRead(ldr1) >= threshold ? ledIsOn = 1 : ledIsOn = 0;
        sendLoRa("PJU-1-" + block + "-" + String(nodeId) + "-" + String(ledIsOn) + "-" + String(analogRead(ldr1)) + "-S", "responding to anchor");
      } else if (received.endsWith("-L")) {
        // Turn off LED
        digitalWrite(led1, LOW);
        analogRead(ldr1) >= threshold ? ledIsOn = 1 : ledIsOn = 0;
        sendLoRa("PJU-1-" + block + "-" + String(nodeId) + "-" + String(ledIsOn) + "-" + String(analogRead(ldr1)) + "-S", "responding to anchor");
      }
    }else if(received.startsWith("PJU-2-" + superBlock)){
      if (received.startsWith("PJU-2-" + block)) {
        isAnchor = 1;  // Set the status as an anchor

        if (received.endsWith("-I")) {
          resetArray();
          delay(500);
          sendLoRa("PJU-3-" + block + "-I", "Forward to node");  // Forwarding message to all node in the block
          command = "I";
          ldrValues[1] = analogRead(ldr1);
          nodesStatus[1] = (digitalRead(led1) == HIGH) ? 1 : 0;
          delay(500);
          setResponseCode(1, (analogRead(ldr1) >= ledThreshold) ? 1 : 0, analogRead(ldr1));
          lastSendTime = millis();
          waitingForResponse = true;
        } else if (received.endsWith("-H")) {
          resetArray();
          delay(500);
          sendLoRa("PJU-3-" + block + "-H", "Forward to node");  // Forwarding message to all node in the block
          command = "H";
          digitalWrite(led1, HIGH);
          lastDarkStat = 1;
          ldrValues[1] = analogRead(ldr1);
          nodesStatus[1] = (digitalRead(led1) == HIGH) ? 1 : 0;
          delay(500);
          setResponseCode(1, (analogRead(ldr1) >= ledThreshold) ? 1 : 0, analogRead(ldr1));
          lastSendTime = millis();
          waitingForResponse = true;
        } else if (received.endsWith("-L")) {
          resetArray();
          delay(500);
          sendLoRa("PJU-3-" + block + "-L", "Forward to node");  // Forwarding message to all node in the block
          command = "L";
          digitalWrite(led1, LOW);
          lastDarkStat = 0;
          ldrValues[1] = analogRead(ldr1);
          nodesStatus[1] = (digitalRead(led1) == HIGH) ? 1 : 0;
          delay(500);
          setResponseCode(1, (analogRead(ldr1) >= ledThreshold) ? 1 : 0, analogRead(ldr1));
          lastSendTime = millis();
          waitingForResponse = true;
        }
      }else if(isAnchor && received.startsWith("PJU-1-" + block) && received.endsWith("-S")){  // Receiving reply from the node
        parseNodeInfo(received);
      }else{
        sendLoRa(received, "Forward to the next block");  // Receiving message from controller that is not intended to it's block, then forward it
      }
    }
  }

  // Timeout handling
  if (waitingForResponse && (millis() - lastSendTime >= timeout) && (millis() - lastSendTime < timeout + 5000)) {
    String result = "";
    for (int i = 0; i < nodeCount; i++) {
      result += String(responseCode[i]);
    }
    sendLoRa("PJU-0-" + block + "-" + result, "Return info to control");  // Send response of the block to the controller
  }

  delay(50);
}

// Function to send a message via LoRa
void sendLoRa(String message, String info) {
  for(int i=0; i<3; i++){ 
    LoRa.beginPacket();
    LoRa.print(message);
    LoRa.endPacket();
  }
}

// Parse node information from received message
void parseNodeInfo(String received) {
  int firstDash = received.indexOf('-');
  int secondDash = received.indexOf('-', firstDash + 1);
  int thirdDash = received.indexOf('-', secondDash + 1);
  int fourthDash = received.indexOf('-', thirdDash + 1);
  int fifthDash = received.indexOf('-', fourthDash + 1);

  String idStr = received.substring(thirdDash + 1, fourthDash);
  String statusStr = received.substring(fourthDash + 1, fifthDash);
  String ldrValStr = received.substring(fifthDash + 1, received.length() - 2);

  int id = idStr.toInt();
  int status = statusStr.toInt();
  int ldrVal = ldrValStr.toInt();
  ldrValues[id] = ldrVal;
  nodesStatus[id] = status;

  setResponseCode(id, status, ldrVal);
}

void setResponseCode(int id, int status, int ldrVal){
  // 0 = no response | 1 = the light is on | 2 = the light is off | 3 = the light is broken | 4 = the sensor is error (value always 0)
  if (ldrVal == 0) {
      responseCode[id] = 4;
  } else {
    if (command == "I") {
      responseCode[id] = (status == 1) ? 1 : 2; 
    } else if (command == "H") {
      responseCode[id] = (status == 1) ? 1 : 3;
    } else if (command == "L") {
      if (status == 0) {
        responseCode[id] = 2;
      }
    }
  }
}

void resetArray(){
  for (int i=0; i<nodeCount; i++){
    nodesStatus[i] = 0;
    ldrValues[i] = 0;
    responseCode[i] = 0;
  }
}