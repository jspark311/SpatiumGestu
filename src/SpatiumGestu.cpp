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


#if defined(__MK20DX256__) | defined(__MK20DX128__)
IntervalTimer timer0;               // Scheduler
void timerCallbackScheduler() {  if (scheduler) scheduler->advanceScheduler(); }

const int PIN_LED1        = 13;
const int MANUVR_LOGO_LED = 4;

#else
const int MANUVR_LOGO_LED = 4;

uint32_t timerCallbackScheduler(uint32_t currentTime) {  
  if (scheduler) scheduler->advanceScheduler(); 
  return (currentTime + (CORE_TICK_RATE * 1));
}
#endif


uint8_t analog_write_val = 0;
int8_t direction = 1;   // Zero for hold, 1 for brighten, -1 for darken.



void blink_led() {  digitalWrite(PIN_LED1, !digitalRead(PIN_LED1));  }

void logo_fade() {
  if (direction < 0) analog_write_val--;
  else if (direction > 0) analog_write_val++;
  else return;
  
  if (0 == analog_write_val) {
    direction = direction * -1;
  }
  else if (200 == analog_write_val) {
    direction = direction * -1;
  }
  analogWrite(MANUVR_LOGO_LED, analog_write_val);
}



void setup() {
  Serial.begin(HOST_BAUD_RATE);   // USB
  pinMode(PIN_LED1,         OUTPUT);
  pinMode(MANUVR_LOGO_LED,  OUTPUT);
  
  sh = StaticHub::getInstance();

  event_manager = sh->fetchEventManager();
  scheduler     = sh->fetchScheduler();
  
  scheduler->createSchedule(40, -1, false, logo_fade);
  scheduler->createSchedule(25, -1, false, blink_led);
#if defined(__MK20DX256__) | defined(__MK20DX128__)
  timer0.begin(timerCallbackScheduler, 1000);   // Turn on the periodic interrupts...
#elif defined(_BOARD_FUBARINO_MINI_)
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
    char c = 0;
    while (Serial.available()) {
      c = Serial.read();
      *(ser_buffer+bytes_read++) = c;
    }
    
    sh->feedUSBBuffer(ser_buffer, bytes_read, (c == '\r' || c == '\n'));
  }
}

