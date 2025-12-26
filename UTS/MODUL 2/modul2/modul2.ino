#include <WiFi.h>
#include <PubSubClient.h>
#include <Arduino.h>

int ledPin = 2;  
char myData = 0;

int motor1Pin1 = 27;
int motor1Pin2 = 26;
int enable1Pin = 12;

// Setting PWM properties
const int freq = 30000;
const int pwmChannel = 0;
const int resolution = 8;
int dutyCycle = 200;

const byte pin_rpm = 13;
int volatile rev = 0;
//int rpm = 0;

const char* ssid = "Griya Palem lantai 1"; // Enter your WiFi name
const char* password =  "13456789"; // Enter WiFi password

#define mqttServer "broker.emqx.io"
#define mqttPort 1883

WiFiServer server(80);
WiFiClient espClient;
PubSubClient client(espClient);

String Topic;
String Payload;

// constants
const int baud = 115200;       // serial baud rate


void setup()
{
  pinMode(ledPin, OUTPUT);  
  Serial.begin(115200);
  // sets the pins
  pinMode(motor1Pin1, OUTPUT);
  pinMode(motor1Pin2, OUTPUT);
  pinMode(enable1Pin, OUTPUT);
  pinMode(pin_rpm, INPUT_PULLUP);
  //  pinMode(pin_rpm, INPUT);
  //attachInterrupt(digitalPinToInterrupt(pin_rpm), isr, RISING);

  // configure LED PWM functionalitites
  ledcSetup(pwmChannel, freq, resolution);

  // attach the channel to the GPIO to be controlled
  ledcAttachPin(enable1Pin, pwmChannel);

  // testing
  Serial.print("Testing DC Motor...");

  // Connect to WiFi network
  Serial.println();
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
 
  // Connect to Server IoT (CloudMQTT)
  client.setServer(mqttServer, mqttPort);
  client.setCallback(receivedCallback);
 
  while (!client.connected()) {
    Serial.println("Connecting to CLoud IoT ...");
 
//    if (client.connect("ESP32Client", mqttUser, mqttPassword )) {
     if (client.connect("ESP32_Riky")) { 

      Serial.println("connected");
      Serial.print("Message received: ");
   
    } else {
      Serial.print("failed with state ");
      Serial.print(client.state());
      delay(2000);
    }
    client.subscribe("kontrolonoff");
    client.subscribe("kontrolspeed");

  }
}

void MotorOn()
{
  // Move DC motor forward with increasing speed
  dutyCycle = 205;
  digitalWrite(motor1Pin1, HIGH);
  digitalWrite(motor1Pin2, LOW);
  ledcWrite(pwmChannel, dutyCycle);
}

void MotorOff()
{
  dutyCycle = 0;
  digitalWrite(motor1Pin1, HIGH);
  digitalWrite(motor1Pin2, LOW);
  ledcWrite(pwmChannel, dutyCycle);
}


void receivedCallback(char* topic, byte* payload, unsigned int length) {

/* we got '1' -> MotorOn() */
  if ((char)payload[0] == '1') {
        digitalWrite(ledPin, HIGH);   // LED ikut ON
        MotorOn();
        Serial.println("Motor On");
  } 
  
/* we got '0' -> Motoroff */
if ((char)payload[0] == '0') {
      digitalWrite(ledPin, LOW);    // LED ikut OFF
      MotorOff(); 
      Serial.println("Motor Off");  
}

    // === Tambahan: kontrol kecepatan via slider ===
  if (String(topic) == "kontrolspeed") {
    String msg = "";
    for (int i = 0; i < length; i++) {
      msg += (char)payload[i];
    }

    int speedValue = msg.toInt(); // ubah payload ke integer
    if (speedValue < 0) speedValue = 0;
    if (speedValue > 255) speedValue = 255;

    ledcWrite(pwmChannel, speedValue);
    Serial.print("Kecepatan diubah ke: ");
    Serial.println(speedValue);
  }
 
}


void loop()
{

client.loop();

myData = int(Serial.read());

if (myData == '1'){  
  Serial.println("LED is on !!!");  
  digitalWrite(ledPin, HIGH);
  MotorOn();
}
else if (myData == '0'){  
  Serial.println("LED is off !!!");  
  digitalWrite(ledPin, LOW);
  MotorOff();
}


}
