#include <WiFi.h>
#include <PubSubClient.h>
#include <Arduino.h>

int ledPin = 2;  
char myData = 0;

int motor1Pin1 = 27;
int motor1Pin2 = 26;
int enable1Pin = 12;

// ===== PWM Motor =====
const int freq = 30000;
const int pwmChannel = 0;
const int resolution = 8;   // 0..255
int dutyCycle = 200;

// ===== PWM LED (ikut speed) =====
const int ledChannel = 1;      // channel berbeda dari motor
const int ledResolution = 8;   // 0..255

// ===== RPM (belum dipakai) =====
const byte pin_rpm = 13;
int volatile rev = 0;

// ===== WiFi =====
const char* ssid = "Griya Palem lantai 1";
const char* password =  "13456789";

// ===== MQTT =====
#define mqttServer "broker.emqx.io"
#define mqttPort 1883

WiFiServer server(80);
WiFiClient espClient;
PubSubClient client(espClient);

String Topic;
String Payload;

// constants
const int baud = 115200;

// ===== Helper: LED mengikuti speed =====
void setLedBySpeed(int speedValue) {
  // speedValue 0..255
  if (speedValue < 0) speedValue = 0;
  if (speedValue > 255) speedValue = 255;

  // versi langsung (LED = speed)
  ledcWrite(ledChannel, speedValue);

  // Kalau LED terlalu redup saat speed kecil, pakai mapping ini (optional):
  // if (speedValue == 0) ledcWrite(ledChannel, 0);
  // else ledcWrite(ledChannel, map(speedValue, 1, 255, 30, 255));
}

void MotorOn()
{
  // Motor forward speed default
  dutyCycle = 205;
  digitalWrite(motor1Pin1, HIGH);
  digitalWrite(motor1Pin2, LOW);
  ledcWrite(pwmChannel, dutyCycle);

  // LED ikut speed motor
  setLedBySpeed(dutyCycle);
}

void MotorOff()
{
  dutyCycle = 0;
  // stop PWM
  ledcWrite(pwmChannel, dutyCycle);

  // optional: lepas arah (coast)
  // digitalWrite(motor1Pin1, LOW);
  // digitalWrite(motor1Pin2, LOW);

  // LED ikut mati
  setLedBySpeed(0);
}

void receivedCallback(char* topic, byte* payload, unsigned int length) {

  // ==== ON/OFF (payload '1' / '0') ====
  if ((char)payload[0] == '1') {
    MotorOn();
    Serial.println("Motor On");
  } 

  if ((char)payload[0] == '0') {
    MotorOff();
    Serial.println("Motor Off");
  }

  // ==== kontrol kecepatan via topic kontrolspeed (payload angka 0..255) ====
  if (String(topic) == "kontrolspeed") {
    String msg = "";
    for (int i = 0; i < (int)length; i++) {
      msg += (char)payload[i];
    }
    msg.trim();

    int speedValue = msg.toInt();
    if (speedValue < 0) speedValue = 0;
    if (speedValue > 255) speedValue = 255;

    // pastikan arah tetap maju
    digitalWrite(motor1Pin1, HIGH);
    digitalWrite(motor1Pin2, LOW);

    // set PWM motor
    dutyCycle = speedValue;
    ledcWrite(pwmChannel, dutyCycle);

    // LED ikut speed motor
    setLedBySpeed(dutyCycle);

    Serial.print("Kecepatan diubah ke: ");
    Serial.println(dutyCycle);
  }
}

void setup()
{
  Serial.begin(115200);

  // pins
  pinMode(motor1Pin1, OUTPUT);
  pinMode(motor1Pin2, OUTPUT);
  pinMode(enable1Pin, OUTPUT);

  pinMode(pin_rpm, INPUT_PULLUP);

  // ===== PWM setup motor =====
  ledcSetup(pwmChannel, freq, resolution);
  ledcAttachPin(enable1Pin, pwmChannel);
  ledcWrite(pwmChannel, 0);

  // ===== PWM setup LED =====
  ledcSetup(ledChannel, freq, ledResolution);
  ledcAttachPin(ledPin, ledChannel);
  ledcWrite(ledChannel, 0);

  Serial.println("Testing DC Motor...");

  // ===== WiFi connect =====
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println();
  Serial.println("WiFi connected");

  // ===== MQTT connect =====
  client.setServer(mqttServer, mqttPort);
  client.setCallback(receivedCallback);

  while (!client.connected()) {
    Serial.println("Connecting to Cloud IoT ...");

    if (client.connect("ESP32_Riky")) {
      Serial.println("connected");
    } else {
      Serial.print("failed with state ");
      Serial.println(client.state());
      delay(2000);
    }
  }

  // subscribe setelah connect (lebih aman)
  client.subscribe("kontrolonoff");
  client.subscribe("kontrolspeed");
}

void loop()
{
  client.loop();

  // kontrol via Serial Monitor (kirim '1' atau '0')
  myData = (char)Serial.read();

  if (myData == '1') {
    MotorOn();
    Serial.println("Motor+LED ON (Serial)");
  }
  else if (myData == '0') {
    MotorOff();
    Serial.println("Motor+LED OFF (Serial)");
  }
}
