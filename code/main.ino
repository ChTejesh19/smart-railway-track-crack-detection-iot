#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <UniversalTelegramBot.h>

/* -------- WiFi -------- */
const char* WIFI_SSID = "teja";
const char* WIFI_PASSWORD = "12345678";

/* -------- Telegram -------- */
#define BOT_TOKEN "8217394049:AAFzZwZK0eQuGOXsBRgQ-F0Qa3j-jTI0dic"
#define CHAT_ID "6685170312"

WiFiClientSecure client;
UniversalTelegramBot bot(BOT_TOKEN, client);

/* -------- Pin Definitions -------- */
#define IR_PIN 15
#define TRIG_LEFT 25
#define ECHO_LEFT 13
#define TRIG_RIGHT 26
#define ECHO_RIGHT 27

#define SOUND_SPEED 0.0343
#define CRACK_THRESHOLD 6.0
#define POLE_DEBOUNCE 600

/* -------- Variables -------- */
int pole_Count = 0;
float left_Distance = 0;
float right_Distance = 0;
int lastCrackPole = -1;

bool poleLock = false;
unsigned long poleLockTime = 0;

/* -------- Ultrasonic Function -------- */
float readUltrasonic(int trigPin, int echoPin)
{
  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);

  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);

  long duration = pulseIn(echoPin, HIGH, 30000);

  if (duration == 0) return -1;

  float distance = (duration * SOUND_SPEED) / 2;

  if (distance < 1 || distance > 100) return -1;

  return distance;
}

/* -------- WiFi Connect -------- */
void connectWiFi()
{
  Serial.print("Connecting to WiFi");

  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  int retry = 0;

  while (WiFi.status() != WL_CONNECTED && retry < 20)
  {
    delay(500);
    Serial.print(".");
    retry++;
  }

  if (WiFi.status() == WL_CONNECTED)
  {
    Serial.println("\nWiFi Connected!");
    Serial.print("IP: ");
    Serial.println(WiFi.localIP());
  }
  else
  {
    Serial.println("\nWiFi Failed!");
  }
}

/* -------- Setup -------- */
void setup()
{
  Serial.begin(115200);

  pinMode(IR_PIN, INPUT);
  pinMode(TRIG_LEFT, OUTPUT);
  pinMode(ECHO_LEFT, INPUT);
  pinMode(TRIG_RIGHT, OUTPUT);
  pinMode(ECHO_RIGHT, INPUT);

  digitalWrite(TRIG_LEFT, LOW);
  digitalWrite(TRIG_RIGHT, LOW);

  connectWiFi();

  client.setInsecure();

  if (WiFi.status() == WL_CONNECTED)
  {
    bot.sendMessage(CHAT_ID, "🚆 Railway Crack Monitoring Started!", "");
  }
}

/* -------- Loop -------- */
void loop()
{
  // Auto reconnect WiFi if disconnected
  if (WiFi.status() != WL_CONNECTED)
  {
    connectWiFi();
  }

  /* ----- Pole Counting ----- */
  int irValue = digitalRead(IR_PIN);

  if (irValue == LOW && !poleLock)
  {
    pole_Count++;
    poleLock = true;
    poleLockTime = millis();

    Serial.print("Pole Passed: ");
    Serial.println(pole_Count);
  }

  if (poleLock && (millis() - poleLockTime > POLE_DEBOUNCE))
  {
    poleLock = false;
  }

  /* ----- Read Sensors ----- */
  left_Distance = readUltrasonic(TRIG_LEFT, ECHO_LEFT);
  right_Distance = readUltrasonic(TRIG_RIGHT, ECHO_RIGHT);

  Serial.print("Left: ");
  Serial.print(left_Distance);
  Serial.print(" cm | Right: ");
  Serial.print(right_Distance);
  Serial.println(" cm");

  /* ----- Crack Detection ----- */
  if ((left_Distance > CRACK_THRESHOLD || right_Distance > CRACK_THRESHOLD)
      && pole_Count != lastCrackPole
      && WiFi.status() == WL_CONNECTED)
  {
    lastCrackPole = pole_Count;

    String message = "⚠ CRACK DETECTED!\n";
    message += "Pole: " + String(pole_Count) + "\n";
    message += "Left: " + String(left_Distance) + " cm\n";
    message += "Right: " + String(right_Distance) + " cm";

    bot.sendMessage(CHAT_ID, message, "");

    Serial.println("Telegram Alert Sent!");
  }

  delay(500);
}
