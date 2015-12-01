#include "StaticHub/StaticHub.h"
#include "FirmwareDefs.h"
#include <ManuvrOS/EventManager.h>
#include <ManuvrOS/Scheduler.h>
#include <StringBuilder/StringBuilder.h>

#define HOST_BAUD_RATE  115200


StaticHub*    sh            = NULL;
Scheduler*    scheduler     = NULL;
EventManager* event_manager = NULL;


#if defined(__MK20DX256__) | defined(__MK20DX128__)
const int PIN_LED1        = 13;
const int MANUVR_LOGO_LED = 4;

#include "FreeRTOS_ARM.h"

void mainTaskFxn( void *pvParameters );
void schedulerTaskFxn( void *pvParameters );
void loggerTask( void *pvParameters );


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
  
  #if defined(__MK20DX256__) | defined(__MK20DX128__)
  
  // Create the main thread.
  xTaskCreate(mainTaskFxn, "Main", 6000, (void*)sh, 1, NULL );
  
  // Create the scheduler thread. Let's see if this flies....
  xTaskCreate(schedulerTaskFxn, "Sched", 40, (void*)scheduler, 1, NULL );
  xTaskCreate(loggerTask, "Logger", 40, (void*)sh, 1, NULL);
  
  vTaskStartScheduler();
  for( ;; );
  
  #elif defined(_BOARD_FUBARINO_MINI_)
  attachCoreTimerService(timerCallbackScheduler);
  sei();
  #endif
}




#if defined(__MK20DX256__) | defined(__MK20DX128__)

void mainTaskFxn(void *pvParameters) {
  unsigned char* ser_buffer = (unsigned char*) alloca(128);
  int bytes_read = 0;
  
  sh = StaticHub::getInstance();
  event_manager = sh->fetchEventManager();
  scheduler     = sh->fetchScheduler();
  
  scheduler->createSchedule(40, -1, false, logo_fade);
  scheduler->createSchedule(25, -1, false, blink_led);

  /* As per most tasks, this task is implemented in an infinite loop. */
  for( ;; ) {
    if (sh) {
      sh->fetchEventManager()->procIdleFlags();
      sh->fetchScheduler()->serviceScheduledEvents();
      
      if (Serial.available()) {
        // Zero the buffer.
        bytes_read = 0;
        for (int i = 0; i < 128; i++) *(ser_buffer+i) = 0;
        char c = 0;
        while (Serial.available()) {
          c = Serial.read();
          *(ser_buffer+bytes_read++) = c;
        }
        
        sh->feedUSBBuffer(ser_buffer, bytes_read, (c == '\r' || c == '\n'));
      }
    }
  }
}


void schedulerTaskFxn(void *pvParameters) {
  /* As per most tasks, this task is implemented in an infinite loop. */
  for( ;; ) {
    if (sh) sh->fetchScheduler()->advanceScheduler();
    vTaskDelay( 2 / portTICK_PERIOD_MS );
  }
}


void loggerTask(void *pvParameters) {
  /* As per most tasks, this task is implemented in an infinite loop. */
  for(;;) {
    if (StaticHub::log_buffer.count()) {
      if (!sh->getVerbosity()) {
        StaticHub::log_buffer.clear();
      }
      else {
        //printf("%s", StaticHub::log_buffer.position(0));
        Serial.print((char*) StaticHub::log_buffer.position(0));
        StaticHub::log_buffer.drop_position(0);
      }
    }
    else {
      taskYIELD();
    }
  }
}

void loop() {
}


#elif defined(_BOARD_FUBARINO_MINI_)

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

#endif
