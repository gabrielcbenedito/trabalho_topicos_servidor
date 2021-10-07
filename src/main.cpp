#include <Arduino.h>
#include <WiFi.h>

#include "ModbusServerWiFi.h"
#include "CoilData.h"

#include "callbacks.h"

// WiFi
#ifndef MY_SSID
#define MY_SSID "MY_SSID"
#endif
#ifndef MY_PASS
#define MY_PASS "MY_PASS"
#endif

char ssid[] = MY_SSID;
char pass[] = MY_PASS;

// Modbus
ModbusServerWiFi MB;
const uint16_t PORT(502);

// LED
CoilData led_coil(1);
bool led_triggered = false;
int duty_cycle = 255;

// ultrasonic sensor
uint16_t distance_mm;
const int trig_pin = 5;
const int echo_pin = 18;
#define SOUND_SPEED 0.034

uint16_t read_distance_mm()
{
  // ultrassonic sensor read
  // clears the trig_pin
  digitalWrite(trig_pin, LOW);
  delayMicroseconds(2);

  // sets the trig_pin on HIGH state for 10 micro seconds
  digitalWrite(trig_pin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trig_pin, LOW);

  // reads the echo_pin, returns the sound wave travel time in microseconds
  long duration = pulseIn(echo_pin, HIGH);

  // calculate the distance
  float distanceCm = duration * SOUND_SPEED / 2;

  // return distance in mm
  return (uint16_t)(distanceCm * 10);
}

void setup()
{
  // serial
  Serial.begin(115200);
  while (!Serial)
  {
  }
  Serial.println("__ OK __");

  // wifi
  WiFi.begin(ssid, pass);
  delay(200);
  while (WiFi.status() != WL_CONNECTED)
  {
    Serial.print(". ");
    delay(1000);
  }
  IPAddress wIP = WiFi.localIP();
  Serial.printf("WIFi IP address: %u.%u.%u.%u\n", wIP[0], wIP[1], wIP[2], wIP[3]);

  // register callbacks for function codes
  MB.registerWorker(1, READ_COIL, FC01);
  MB.registerWorker(1, READ_HOLD_REGISTER, FC03);
  MB.registerWorker(1, READ_INPUT_REGISTER, FC03);
  MB.registerWorker(1, WRITE_COIL, FC05);
  MB.registerWorker(1, WRITE_HOLD_REGISTER, FC06);
  MB.registerWorker(1, WRITE_MULT_COILS, FC0F);

  // start the server on port PORT, max. 2 concurrent clients served, 2s timeout to close a connection
  MB.start(PORT, 2, 2000);

  // led
  ledcSetup(0, 5000, 8);
  ledcAttachPin(BUILTIN_LED, 0);

  // ultrasonic sensor pinout
  pinMode(trig_pin, OUTPUT);
  pinMode(echo_pin, INPUT);
}

// loop() - watch for the trigger
void loop()
{
  // timer
  static uint32_t lastMillis = 0;

  // Set LED if triggered
  if (led_triggered)
  {
    // reset LED trigger
    led_triggered = false;
    if (led_coil[0])
    {
      ledcWrite(0, duty_cycle);
    }
    else
    {
      ledcWrite(0, 0);
    }
  }

  // read if more than 100 ms has passed
  if (millis() - lastMillis > 1000)
  {
    lastMillis = millis();
    distance_mm = read_distance_mm();
  }
}
