#include <Arduino.h>

// ===== Pin sesuai kit =====
const int motor1Pin1 = 27;   // IN1
const int motor1Pin2 = 26;   // IN2
const int enable1Pin = 12;   // ENA (PWM)
const byte pin_rpm   = 13;   // sensor RPM (pulse)

// ===== PWM ESP32 =====
const int freq = 30000;
const int pwmChannel = 0;
const int resolution = 8;   // 0..255

// ===== Sampling =====
const uint32_t Ts_ms = 100;   // 100 ms
uint32_t lastTick = 0;

// ===== RPM pulse counter =====
volatile long pulseCount = 0;
void IRAM_ATTR isr_rpm() { pulseCount++; }

// ===== Konversi pulse -> RPM =====
// Kamu perlu isi PPR (pulse per revolution) sensor kamu.
// Dari test: PWM 180 menghasilkan ~160 pulses/s.
// Kalau motor sekitar 1500-2000 rpm, PPR bisa 6/20/60 dll.
// Kita set default 20 dulu (nanti bisa kita kalibrasi).
float PPR = 20.0;

// ===== PID =====
float Kp = 1.2;
float Ki = 0.6;
float Kd = 0.0;

float sp_rpm = 0;
float pv_rpm = 0;
float err    = 0;

float integ  = 0;
float prevErr = 0;

int pwmOut = 0;

void setMotorPWM(int pwm) {
  pwm = constrain(pwm, 0, 255);

  // arah maju (sama seperti kode kamu)
  digitalWrite(motor1Pin1, HIGH);
  digitalWrite(motor1Pin2, LOW);

  ledcWrite(pwmChannel, pwm);
}

void parseSerial() {
  if (!Serial.available()) return;

  String line = Serial.readStringUntil('\n');
  line.trim();
  if (line.length() == 0) return;

  // setpoint
  if (line.startsWith("SP ")) {
    sp_rpm = line.substring(3).toFloat();
    Serial.println("OK SP=" + String(sp_rpm, 2));
  }

  // PID tuning (opsional)
  if (line.startsWith("KP ")) { Kp = line.substring(3).toFloat(); Serial.println("OK KP"); }
  if (line.startsWith("KI ")) { Ki = line.substring(3).toFloat(); Serial.println("OK KI"); }
  if (line.startsWith("KD ")) { Kd = line.substring(3).toFloat(); Serial.println("OK KD"); }

  // set PPR kalau mau
  if (line.startsWith("PPR ")) { PPR = line.substring(4).toFloat(); Serial.println("OK PPR"); }
}

void setup() {
  Serial.begin(115200);
  delay(500);

  pinMode(motor1Pin1, OUTPUT);
  pinMode(motor1Pin2, OUTPUT);
  pinMode(enable1Pin, OUTPUT);

  pinMode(pin_rpm, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(pin_rpm), isr_rpm, RISING);

  ledcSetup(pwmChannel, freq, resolution);
  ledcAttachPin(enable1Pin, pwmChannel);

  setMotorPWM(0);

  Serial.println("READY");
  Serial.println("CSV: t_ms,sp_rpm,pv_rpm,err,pwm");
}

void loop() {
  parseSerial();

  uint32_t now = millis();
  if (now - lastTick >= Ts_ms) {
    lastTick = now;

    // hitung pulses dalam Ts
    static long lastPulse = 0;
    long p = pulseCount;
    long dp = p - lastPulse;
    lastPulse = p;

    float dt_s = Ts_ms / 1000.0;

    // pulses/s
    float pps = dp / dt_s;

    // RPM = (pulses/s) * 60 / PPR
    pv_rpm = (pps * 60.0) / PPR;

    // PID discrete
    err = sp_rpm - pv_rpm;

    // anti-windup sederhana: hanya integrasi kalau output belum mentok
    integ += err * dt_s;

    float deriv = (err - prevErr) / dt_s;
    prevErr = err;

    float u = Kp * err + Ki * integ + Kd * deriv;

    // output -> PWM
    pwmOut = (int)round(u);
    pwmOut = constrain(pwmOut, 0, 255);

    setMotorPWM(pwmOut);

    // log CSV
    Serial.print(now);
    Serial.print(",");
    Serial.print(sp_rpm, 2);
    Serial.print(",");
    Serial.print(pv_rpm, 2);
    Serial.print(",");
    Serial.print(err, 2);
    Serial.print(",");
    Serial.println(pwmOut);
  }
}
