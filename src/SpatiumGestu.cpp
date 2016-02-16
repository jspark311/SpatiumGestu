/*
 * Example to demonstrate thread definition, semaphores, and thread sleep.
 */
#include <FreeRTOS_ARM.h>
#include "FirmwareDefs.h"
#include <ManuvrOS/Platform/Platform.h>
#include <ManuvrOS/Kernel.h>
#include <DataStructures/StringBuilder.h>


// The LED is attached to pin 13 on Arduino.
const uint8_t PIN_LED1 = 13;


Kernel* kernel = NULL;

void blink_led() {  digitalWrite(PIN_LED1, !digitalRead(PIN_LED1));  }



// Declare a semaphore handle.
SemaphoreHandle_t sem;
//------------------------------------------------------------------------------
/*
 * Thread 1, turn the LED off when signalled by thread 2.
 */
// Declare the thread function for thread 1.
static void Thread1(void* arg) {
  while (1) {
    // Wait for signal from thread 2.
    xSemaphoreTake(sem, portMAX_DELAY);
    vTaskDelay( 10 / portTICK_PERIOD_MS );
  }
}


void serialIOTaskFxn(void *pvParameters) {
  unsigned char* ser_buffer = (unsigned char*) alloca(255);
  int bytes_read = 0;
  char tmp_char  = 0;
  for(;;) {
    if (Serial.available()) {
      blink_led();
      // Zero the buffer.
      while (Serial.available()) {
        tmp_char = Serial.read();
        *(ser_buffer+bytes_read++) = tmp_char;
        if (tmp_char == '\r' || tmp_char == '\n') {
          *(ser_buffer+bytes_read) = 0;
          kernel->accumulateConsoleInput(ser_buffer, bytes_read, true);
          bytes_read = 0;
          for (int i = 0; i < 255; i++) *(ser_buffer+i) = 0;
        }
      }
    }
    else {
      vTaskDelay(11 / portTICK_PERIOD_MS);
    }
  }
}


static void schedulerTaskFxn(void *pvParameters) {
  for(;;) {
    // TODO: This sucks. There must be a better way of having the kernel's
    //   sense of time not being subservient to FreeRTOS's...
    kernel->advanceScheduler();
    vTaskDelay( 10 / portTICK_PERIOD_MS );
  }
}


void loggerTask(void *pvParameters) {
  for(;;) {
    if (Kernel::log_buffer.count()) {
      if (!kernel->getVerbosity()) {
        Kernel::log_buffer.clear();
      }
      else {
        Serial.print((char*) Kernel::log_buffer.position(0));
        Kernel::log_buffer.drop_position(0);
      }
    }
    else {
      //vTaskSuspend(NULL);   // Nothing more to do.
      vTaskDelay( 10 / portTICK_PERIOD_MS );
    }
  }
}


//------------------------------------------------------------------------------
/*
 * Thread 2, turn the LED on and signal thread 1 to turn the LED off.
 */
// Declare the thread function for thread 2.
static void Thread2(void* arg) {

  pinMode(PIN_LED1, OUTPUT);

  while (1) {
    // Sleep for 200 milliseconds.
    vTaskDelay((200L * configTICK_RATE_HZ) / 1000L);
    kernel->procIdleFlags();
    kernel->advanceScheduler();
    Serial.println(F("Hold"));

    // Signal thread 1 to turn LED off.
    xSemaphoreGive(sem);
  }
}



void setup() {
  portBASE_TYPE s1, s2, s3, s4, s5;
  
  TaskHandle_t logger_pid = 0;
  TaskHandle_t kernel_pid = 0;
  
  Serial.begin(115200);
  kernel = new Kernel();
  
  // initialize semaphore
  sem = xSemaphoreCreateCounting(1, 0);

  // create task at priority two
  s1 = xTaskCreate(Thread1, NULL, configMINIMAL_STACK_SIZE, NULL, 2, NULL);

  // create task at priority one
  s2 = xTaskCreate(Thread2, NULL, 3076, NULL, 1, &kernel_pid);
  
  s3 = xTaskCreate(loggerTask, NULL, 1024, NULL, 1, &logger_pid);
  s4 = xTaskCreate(serialIOTaskFxn, NULL, 1024, NULL, 1, &logger_pid);
  s5 = xTaskCreate(schedulerTaskFxn, NULL, 128, NULL, 1, NULL);

  // check for creation errors
  if (sem== NULL || s1 != pdPASS || s2 != pdPASS || s3 != pdPASS ) {
    Serial.println(F("Creation problem"));
    while(1);
  }
  
  //kernel->createSchedule(100, -1, false, blink_led);

  // start scheduler
  kernel->bootstrap();
  kernel->provideKernelPID(kernel_pid);
  kernel->provideLoggerPID(logger_pid);

  while(1);
}

void loop() {
}
