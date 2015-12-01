#include "FirmwareDefs.h"
#include <ManuvrOS/Platform/Platform.h>
#include <ManuvrOS/Kernel.h>
#include <StringBuilder/StringBuilder.h>

#define HOST_BAUD_RATE  115200


EventManager* kernel = NULL;


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
  if (kernel) kernel->advanceScheduler(); 
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
  
  EventManager __kernel;  // Instance the kernel.
  kernel = &__kernel;
  
  #if defined(__MK20DX256__) | defined(__MK20DX128__)
  
  // Create the main thread.
  xTaskCreate(mainTaskFxn, "Main", 6000, NULL, 1, NULL );

  // Create the scheduler thread. Let's see if this flies....
  xTaskCreate(schedulerTaskFxn, "Sched", 40, (void*)kernel, 1, NULL );
  xTaskCreate(loggerTask, "Logger", 40, (void*)kernel, 1, NULL);
  
  vTaskStartScheduler();
  for(;;);
  
  #elif defined(_BOARD_FUBARINO_MINI_)
  attachCoreTimerService(timerCallbackScheduler);
  sei();
  #endif
}




#if defined(__MK20DX256__) | defined(__MK20DX128__)

void mainTaskFxn(void *pvParameters) {
  unsigned char* ser_buffer = (unsigned char*) alloca(255);
  int bytes_read = 0;
  
  kernel->createSchedule(40, -1, false, logo_fade);
  kernel->createSchedule(25, -1, false, blink_led);

  /* As per most tasks, this task is implemented in an infinite loop. */
  for(;;) {
    kernel->procIdleFlags();
    kernel->serviceScheduledEvents();
    
    if (Serial.available()) {
      // Zero the buffer.
      bytes_read = 0;
      for (int i = 0; i < 255; i++) *(ser_buffer+i) = 0;
      char c = 0;
      while (Serial.available()) {
        c = Serial.read();
        *(ser_buffer+bytes_read++) = c;
      }
      
      kernel->feedUSBBuffer(ser_buffer, bytes_read, (c == '\r' || c == '\n'));
    }
  }
}


void schedulerTaskFxn(void *pvParameters) {
  for(;;) {
    // TODO: This sucks. There must be a better way of having the kernel's
    //   sense of time not being subservient to FreeRTOS's...
    kernel->advanceScheduler();
    vTaskDelay( 2 / portTICK_PERIOD_MS );
  }
}


void loggerTask(void *pvParameters) {
  for(;;) {
    if (EventManager::log_buffer.count()) {
      if (!kernel->getVerbosity()) {
        EventManager::log_buffer.clear();
      }
      else {
        //printf("%s", Kernel::log_buffer.position(0));
        Serial.print((char*) EventManager::log_buffer.position(0));
        EventManager::log_buffer.drop_position(0);
      }
    }
    else {
      // Nothing more to do.
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
      kernel->procIdleFlags();
      kernel->serviceScheduledEvents();
    }

    // Zero the buffer.
    bytes_read = 0;
    for (int i = 0; i < 255; i++) *(ser_buffer+i) = 0;
    char c = 0;
    while (Serial.available()) {
      c = Serial.read();
      *(ser_buffer+bytes_read++) = c;
    }
    
    kernel->feedUSBBuffer(ser_buffer, bytes_read, (c == '\r' || c == '\n'));
  }
}

#endif
