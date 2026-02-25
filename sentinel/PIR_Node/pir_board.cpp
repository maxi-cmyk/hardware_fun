#include "../shared.h"

const int PIR_PIN = 13; 
const int BUZZER_PIN = 12; 
const int LED_PIN = 22;    

bool isArmed = false;     
bool panicActive = false; 

void playWarbleSiren() {
  for (int freq = 600; freq < 1200; freq += 20) {
    tone(BUZZER_PIN, freq); digitalWrite(LED_PIN, HIGH); delay(10);
  }
  for (int freq = 1200; freq > 600; freq -= 20) {
    tone(BUZZER_PIN, freq); digitalWrite(LED_PIN, LOW); delay(10);
  }
}

BLYNK_CONNECTED() { Blynk.syncAll(); }
BLYNK_WRITE(V0) { isArmed = param.asInt(); }
BLYNK_WRITE(V3) { 
  panicActive = param.asInt(); 
  if (!panicActive) Blynk.virtualWrite(V4, 0); 
}

void setup() {
  Serial.begin(115200);
  pinMode(PIR_PIN, INPUT);
  pinMode(BUZZER_PIN, OUTPUT);
  pinMode(LED_PIN, OUTPUT);
  Blynk.begin(BLYNK_AUTH_TOKEN2, ssid, pass);
}

void loop() {
  Blynk.run();
  
  if (panicActive) {
    Blynk.virtualWrite(V1, "!!! MANUAL PANIC !!!");
    Blynk.virtualWrite(V4, 255);
    playWarbleSiren();
  } else {
    noTone(BUZZER_PIN);
    
    if (digitalRead(PIR_PIN) == HIGH) {
      // --- THE HANDSHAKE ---
      Blynk.virtualWrite(V2, 1);     // Graph spike
      Blynk.virtualWrite(V6, 1);     // Trigger Camera!
      
      // Crucial: Small delay & immediate reset so V6 is ready for the next motion
      delay(500); 
      Blynk.virtualWrite(V6, 0); 

      Blynk.logEvent("intrusion_alert", "Checking camera...");

      if (isArmed) {
        Blynk.virtualWrite(V1, "ALARM TRIGGERED!");
        unsigned long start = millis();
        while(millis() - start < 5000) { 
          playWarbleSiren(); 
          Blynk.run(); // Keep connection alive during siren
        }
      } else {
        Blynk.virtualWrite(V1, "Motion Detected (Silent)");
      }
      Blynk.virtualWrite(V2, 0);
    } else {
      Blynk.virtualWrite(V1, isArmed ? "Armed & Scanning" : "Monitoring (Silent)");
      digitalWrite(LED_PIN, LOW);
    }
  }
}