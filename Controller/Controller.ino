#include <LoRa.h>
#include <SPI.h>
#include <WiFi.h>
#include <PubSubClient.h>

const char* ssid = ""; // Enter Wi-Fi SSID where the controller is placed
const char* password = ""; // Enter Wi-Fi password where the controller is placed
const char* mqtt_server = ""; // Enter MQTT Broker/Server
const char* mqtt_user = "";  // Enter MQTT user (if required)
const char* mqtt_password = "";  // Enter MQTT password (if required)

#define ss 5
#define rst 14
#define dio0 2

const unsigned long timeout = 35000;  // 35 milliseconds (35 s) timeout (Can be adjusted)

WiFiClient espClient;
PubSubClient client(espClient);

unsigned long lastSendTime = 0;
unsigned long anchorSendTime = 0;
bool waitingForResponse = false;
bool waitingForBackupResponse = false;
String command;
String block;
String MQTTmessage;

void setup_wifi() {
  delay(10);
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}

// NodeRed MQTT
void reconnect() {
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    if (client.connect("LoRaControl", mqtt_user, mqtt_password)) {
      Serial.println("connected");
      client.subscribe("PJU-Control");
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      delay(5000);
    }
  }
}

void mqtt_callback(char* topic, byte* message, unsigned int length) {
  String msg = "";
  for (int i = 0; i < length; i++) {
    msg += (char)message[i];
  }
  
  Serial.print("Message received on topic: ");
  Serial.print(topic);
  Serial.print(". Message: ");
  Serial.println(msg);

  // Split the message at the dash
  int dashIndex = msg.indexOf('-');
  if (dashIndex != -1) {  // Check if the dash exists
    command = msg.substring(0, dashIndex);  // Before the dash
    block = msg.substring(dashIndex + 1);   // After the dash

    String loRaCommand;

    if (command == "INFO") {
      loRaCommand = "PJU-1-" + block + "-I";
      command = 'I';
    } else if (command == "ON") {
      loRaCommand = "PJU-1-" + block + "-H";
      command = 'H';
    } else if (command == "OFF") {
      loRaCommand = "PJU-1-" + block + "-L";
      command = 'L';
    }

    // Send the constructed LoRa message
    sendLoRa(loRaCommand, "send");
    lastSendTime = millis();
    waitingForResponse = true;
  }
}


void setup() {
  Serial.begin(115200);
  delay(1000);

  LoRa.setPins(ss, rst, dio0);
  if (!LoRa.begin(915E6)) { // LoRa frequency 915 MHz
    Serial.println("Starting LoRa failed!");
    while (1);
  } else {
    Serial.println("LoRa initialization done.");
  }

  //LoRa Settings
  LoRa.setSpreadingFactor(12);
  LoRa.setSignalBandwidth(125E3);
  LoRa.setCodingRate4(8);
  LoRa.setPreambleLength(12);
  LoRa.setTxPower(20);

  setup_wifi();
  client.setServer(mqtt_server, 1883);
  client.setCallback(mqtt_callback);
}

void loop() {
  if (!client.connected()) {
    reconnect();
  }
  client.loop();  // Handle MQTT messages

  // First timeout: Check if we are waiting for a response after sending PJU-1-AB-0
  if (waitingForResponse && (millis() - lastSendTime >= timeout)) {
    sendLoRa("PJU-2-"+block+"-"+command, "Timeout! Sending message to backup anchor.");
    waitingForResponse = false;  // Stop waiting for response
    waitingForBackupResponse = true;  // Start waiting for anchor response
    anchorSendTime = millis();  // Start timing for the second timeout
  }

  // Second timeout: Check if we are waiting for a response after sending PJU-2-AB-0
  if (waitingForBackupResponse && (millis() - anchorSendTime >= timeout)) {
    Serial.println("Timeout! No response");
    waitingForBackupResponse = false;  // Stop waiting for anchor response
    client.publish("PJU-Response", "PJU-0-AB-0"); // Send response to web server via MQTT that indicates the anchor and it's backup is broken
  }

  // Check for incoming LoRa messages
  int packetSize = LoRa.parsePacket();
  if (packetSize) {
    String received = "";
    while (LoRa.available()) {
      received += (char)LoRa.read();
    }

    // Check if the received message starts with "PJU-0"
    if (received.startsWith("PJU-0")) {
      Serial.println("Received response: " + received);
      waitingForResponse = false;
      waitingForBackupResponse = false;
      if(received != MQTTmessage){\
        MQTTmessage = received;
        client.publish("PJU-Response", MQTTmessage.c_str());
      }
    }
  }

  delay(10);
}

void sendLoRa(String message, String info) {
  for(int i=0; i<5; i++){ 
    LoRa.beginPacket();
    LoRa.print(message);
    LoRa.endPacket();
  }
}

void parseResponse(String received){
  int firstDash = received.indexOf('-');
  int secondDash = received.indexOf('-', firstDash + 1);
  int thirdDash = received.indexOf('-', secondDash + 1);

  String blockStr = received.substring(secondDash + 1, thirdDash);
  String responseStr = received.substring(thirdDash + 1, received.length() + 1);
}
