#include "StaticHub/StaticHub.h"
#include "FirmwareDefs.h"
#include <ManuvrOS/EventManager.h>
#include <ManuvrOS/Scheduler.h>
#include <StringBuilder/StringBuilder.h>

// Pin declarations

#define HOST_BAUD_RATE  115200


StaticHub*    sh            = NULL;
Scheduler*    scheduler     = NULL;
EventManager* event_manager = NULL;


void blink_led() {  digitalWrite(13, !digitalRead(13));  }

#if defined(__MK20DX256__) | defined(__MK20DX128__)
IntervalTimer timer0;               // Scheduler
void timerCallbackScheduler() {  if (scheduler) scheduler->advanceScheduler(); }

#else
uint32_t timerCallbackScheduler(uint32_t currentTime) {  
  if (scheduler) scheduler->advanceScheduler(); 
  return (currentTime + (CORE_TICK_RATE * 1));
}
#endif


void setup() {
  Serial.begin(HOST_BAUD_RATE);   // USB debugging.
  pinMode(13, OUTPUT);
  sh = StaticHub::getInstance();
  sh->bootstrap();

  event_manager = sh->fetchEventManager();
  scheduler     = sh->fetchScheduler();

  scheduler->createSchedule(100, -1, false, blink_led);
#if defined(__MK20DX256__) | defined(__MK20DX128__)
  timer0.begin(timerCallbackScheduler, 1000);   // Turn on the periodic interrupts...
#else
  attachCoreTimerService(timerCallbackScheduler);
#endif
  sei();
}


void loop() {
  unsigned char* ser_buffer = (unsigned char*) alloca(255);
  int bytes_read = 0;

  while (1) {   // Service this loop for-ev-ar
    while (Serial.available() == 0) {
      event_manager->procIdleFlags();
      scheduler->serviceScheduledEvents();
    }

    // Zero the buffer.
    bytes_read = 0;
    for (int i = 0; i < 255; i++) *(ser_buffer+i) = 0;
    char c;
    while (Serial.available()) {
      c = Serial.read();
      *(ser_buffer+bytes_read++) = c;
    }

    sh->feedUSBBuffer(ser_buffer, bytes_read, (c == '\r' || c == '\n'));
  }
}

