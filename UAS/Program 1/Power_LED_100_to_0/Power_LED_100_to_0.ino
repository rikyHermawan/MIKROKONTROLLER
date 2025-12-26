/*
  Simple Serial LED Control
  Commands (newline \n):
  - LED 1        -> LED ON
  - LED 0        -> LED OFF
  - POWER 0..100 -> brightness 0..100 (mapped to PWM 0..255)
  - PING         -> reply "PONG"
*/

const int LED_PIN = 2;       // pin PWM (UNO: 3,5,6,9,10,11)
bool ledState = false;

String line;

void setup() {
  pinMode(LED_PIN, OUTPUT);
  Serial.begin(115200);
  delay(300);
  Serial.println("READY");
}

void setPower(int p) {
  p = constrain(p, 0, 100);
  int pwm = map(p, 0, 100, 0, 255);
  analogWrite(LED_PIN, pwm);
  Serial.print("LED Power ");
  Serial.println(p);
}

void setLED(int on) {
  ledState = (on != 0);
  if (!ledState) {
    analogWrite(LED_PIN, 0);
    Serial.println("LED off");
  } else {
    analogWrite(LED_PIN, 255);
    Serial.println("LED on");
  }
}

void loop() {
  while (Serial.available()) {
    char c = (char)Serial.read();
    if (c == '\r') continue;
    if (c == '\n') {
      line.trim();

      if (line.equalsIgnoreCase("PING")) {
        Serial.println("PONG");
      }
      else if (line.startsWith("LED")) {
        // format: "LED 1" atau "LED 0"
        int sp = line.indexOf(' ');
        int val = (sp > 0) ? line.substring(sp + 1).toInt() : 0;
        setLED(val);
      }
      else if (line.startsWith("POWER")) {
        // format: "POWER 0..100"
        int sp = line.indexOf(' ');
        int val = (sp > 0) ? line.substring(sp + 1).toInt() : 0;
        setPower(val);
      }
      else {
        Serial.print("ERR Unknown: ");
        Serial.println(line);
      }

      line = "";
    } else {
      line += c;
      if (line.length() > 80) line = ""; // prevent overflow
    }
  }
}
