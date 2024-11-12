#include <LoRa.h>
#include <SPI.h>

#define ss 5
#define rst 14
#define dio0 2

const int led1 = 27;
const int ldr1= 12;
const String block = "AB";  // Define the block
const int nodeId = 2; // Define the node ID in the block (0 is anchor, 1 is backup anchor, input sequentially)
const int threshold = 3200;  // threshold for the light to be considered on

int ledIsOn = 0;

void setup() {
  Serial.begin(115200);
  delay(1000);
  
  pinMode(led1, OUTPUT);
  pinMode(ldr1, INPUT);

  LoRa.setPins(ss, rst, dio0);
  if (!LoRa.begin(915E6)) {  // LoRa frequency 915 MHz
    Serial.println("Starting LoRa failed!");
    while (1);
  }else{
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
  int packetSize = LoRa.parsePacket();
  if (packetSize) {
    String received = "";
    while (LoRa.available()) {
      received += (char)LoRa.read();
    }

    // Check if the received message is destined to block
    if (received.startsWith("PJU-3-"+block)) {
      delay(500);
      
      // Handle the message based on the last character
      if (received.endsWith("-I")) {  
        // Print LED status
        delay(1000);
        analogRead(ldr1) >= threshold ? ledIsOn = 1 : ledIsOn = 0;
        sendLoRa("PJU-1-" + block + "-" + String(nodeId) + "-" + String(ledIsOn) + "-" + String(analogRead(ldr1)) + "-S", "responding to anchor");
      } else if (received.endsWith("-H")) {
        // Turn on LED
        digitalWrite(led1, HIGH);
        delay(1000);
        analogRead(ldr1) >= threshold ? ledIsOn = 1 : ledIsOn = 0;
        sendLoRa("PJU-1-" + block + "-" + String(nodeId) + "-" + String(ledIsOn) + "-" + String(analogRead(ldr1)) + "-S", "responding to anchor");
      } else if (received.endsWith("-L")) {
        // Turn off LED
        digitalWrite(led1, LOW);
        delay(1000);
        analogRead(ldr1) >= threshold ? ledIsOn = 1 : ledIsOn = 0;
        sendLoRa("PJU-1-" + block + "-" + String(nodeId) + "-" + String(ledIsOn) + "-" + String(analogRead(ldr1)) + "-S", "responding to anchor");
      }
    }
  }
  delay(100);
}

void sendLoRa(String message, String info){
  for(int i=0; i<5; i++){
    LoRa.beginPacket();
    LoRa.print(message);
    LoRa.endPacket();
    delay(50);
  }
}
