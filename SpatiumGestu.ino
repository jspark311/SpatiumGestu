#include "StaticHub.h"
#include "FirmwareDefs.h"
#include <ManuvrOS/EventManager.h>
#include <ManuvrOS/Scheduler.h>
#include <StringBuilder/StringBuilder.h>

// Pin declarations


IntervalTimer timer0;               // Scheduler

StaticHub *sh;
Scheduler*    scheduler = NULL;

void blink_led() {
  digitalWrite(13, !digitalRead(13));
}


void timerCallbackScheduler() {  if (scheduler) scheduler->advanceScheduler(); }


void setup() {
  Serial.begin(115200);   // USB debugging.
  pinMode(13, OUTPUT);
  sh = StaticHub::getInstance();
  sh->bootstrap();

  timer0.begin(timerCallbackScheduler, 1000);   // Turn on the periodic interrupts...
  sei();
}


void loop() {
  EventManager* em  = sh->fetchEventManager();
  scheduler = sh->fetchScheduler();
  
  scheduler->createSchedule(100, -1, false, blink_led);
  
  while (true) {
    while (Serial.available() == 0) {
      em->procIdleFlags();
      scheduler->serviceScheduledEvents();
    }
    
    char c = Serial.read();
    sh->feedUSBBuffer((uint8_t*) &c, 1, (c == '\r' || c == '\n'));
  }
}

