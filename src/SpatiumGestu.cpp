/*
 * Example to demonstrate thread definition, semaphores, and thread sleep.
 */
#include <FreeRTOS_ARM.h>
#include "FirmwareDefs.h"
#include <ManuvrOS/Platform/Platform.h>
#include <ManuvrOS/Kernel.h>
#include <DataStructures/StringBuilder.h>

#include <ManuvrOS/Drivers/MCP73833/MCP73833.h>
#include <ManuvrOS/Drivers/MGC3130/MGC3130.h>
#include <ManuvrOS/Drivers/ADP8866/ADP8866.h>
#include <ManuvrOS/Drivers/INA219/INA219.h>

void vApplicationTickHook(void);

// The LED is attached to pin 13 on Arduino.
const uint8_t PIN_LED1 = 13;

TaskHandle_t logger_pid = 0;
TaskHandle_t kernel_pid = 0;
  

Kernel* kernel = NULL;

void blink_led() {  digitalWrite(PIN_LED1, !digitalRead(PIN_LED1));  }



void vApplicationTickHook(void) {
  // TODO: This sucks. There must be a better way of having the kernel's
  //   sense of time not being subservient to FreeRTOS's...
  blink_led();
  kernel->advanceScheduler();
}



static void serialIOTaskFxn(void *pvParameters) {
  unsigned char* ser_buffer = (unsigned char*) alloca(255);
  int bytes_read = 0;
  char tmp_char  = 0;
  bool mark_end  = false;
  for(;;) {
    if (Serial.available()) {
      blink_led();
      // Zero the buffer.
      while (Serial.available()) {
        tmp_char = Serial.read();
        *(ser_buffer+bytes_read++) = tmp_char;
        
        if (tmp_char == '\r' || tmp_char == '\n') {
          mark_end  = true;
          *(ser_buffer+bytes_read) = 0;
          kernel->accumulateConsoleInput(ser_buffer, bytes_read, mark_end);
          bytes_read = 0;
          for (int i = 0; i < 255; i++) *(ser_buffer+i) = 0;
        }
      }
    }
    else {
      vTaskDelay(11 / portTICK_PERIOD_MS);
      //vTaskSuspend(NULL);   // Nothing more to do.
    }
  }
}


static void loggerTask(void *pvParameters) {
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
      vTaskSuspend(NULL);   // Nothing more to do.
    }
  }
}



static void mainThread(void* arg) {
  pinMode(PIN_LED1, OUTPUT);

  I2CAdapter i2c(0);

  //mgc3130 = new MGC3130(16, 17);
  //INA219 ina219(0x4A);
  ADP8866 adp8866(7, 8, 0x27);
  
  kernel->subscribe((EventReceiver*) &i2c);
  kernel->subscribe((EventReceiver*) &adp8866);
  
  //i2c.addSlaveDevice(&ina219);
  i2c.addSlaveDevice(&adp8866);

  //mgc3130->init();
  

  //kernel->createSchedule(100, -1, false, blink_led);

  kernel->provideKernelPID(kernel_pid);
  kernel->provideLoggerPID(logger_pid);
  kernel->bootstrap();

  while (1) {
    kernel->procIdleFlags();
    taskYIELD();
  }
}



void setup() {
  portBASE_TYPE s1, s2, s3;
  
  Serial.begin(115200);
  kernel = Kernel::getInstance();
  
  // create task at priority one
  s1 = xTaskCreate(mainThread, "Main", 4096, NULL, 1, &kernel_pid);
  s2 = xTaskCreate(loggerTask, "Log", 1024, NULL, 1, &logger_pid);
  s3 = xTaskCreate(serialIOTaskFxn,  "SerIO", 1024, NULL, 1, NULL);

  // check for creation errors
  if (s1 != pdPASS || s2 != pdPASS || s3 != pdPASS ) {
    Serial.println(F("Creation problem"));
    while(1);
  }
  
  // start scheduler
  vTaskStartScheduler();

  while(1);
}

void loop() {
}

