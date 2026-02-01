🛡️ Room Security System (Distributed IoT)
💡 The Concept

A two-node security system using Event-Driven Architecture.

The Sensor (PIR_Node): Acts as a physical interrupt. When motion is detected, it "publishes" an event to the Blynk Cloud.

The Actor (Camera_Node): Subscribed to the cloud event. It triggers a high-res capture (Mugshot) and provides a live MJPEG video stream.

🔌 Hardware

ESP32-CAM (AI-Thinker)

ESP32 (Standard)

PIR Motion Sensor & Passive Buzzer

🔗 The Logic Bridge

The nodes communicate via Blynk Virtual Pins, acting as a virtual circuit wire across the Wi-Fi network.