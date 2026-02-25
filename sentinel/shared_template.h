#ifndef SHARED_H
#define SHARED_H

#define BLYNK_TEMPLATE_ID "YOUR IDt"
#define BLYNK_TEMPLATE_NAME "Room Security"
#define BLYNK_AUTH_TOKEN1 "YOUR AUTH"
#define BLYNK_AUTH_TOKEN2 "YOUR AUTH"

#include <Arduino.h>
#include <WiFi.h>
#include <BlynkSimpleEsp32.h>

// Shared WiFi Credentials
extern char ssid[] = "YOUR WIFI NAME";
extern char pass[] = "YOUR WIFI PASSWORD";

#endif