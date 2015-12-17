#include "FirmwareDefs.h"
#include <ManuvrOS/Platform/Platform.h>
#include <ManuvrOS/Kernel.h>
#include <DataStructures/StringBuilder.h>

#include <ManuvrOS/Drivers/MCP73833/MCP73833.h>
#include <ManuvrOS/Drivers/MGC3130/MGC3130.h>
#include <ManuvrOS/Drivers/ADP8866/ADP8866.h>
#include <ManuvrOS/Drivers/INA219/INA219.h>

#define HOST_BAUD_RATE  115200


Kernel* kernel = NULL;


#if defined(__MK20DX256__) | defined(__MK20DX128__)
const int PIN_LED1        = 13;
const int MANUVR_LOGO_LED = 4;

#include <FreeRTOS_ARM.h>

void mainTaskFxn( void *pvParameters );
void schedulerTaskFxn( void *pvParameters );
void debugDumpTaskFxn( void *pvParameters );
void serialIOTaskFxn( void *pvParameters );
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


TaskHandle_t logger_pid = 0;
TaskHandle_t kernel_pid = 0;


void setup() {
  Serial.begin(HOST_BAUD_RATE);   // USB
  pinMode(PIN_LED1,         OUTPUT);
  pinMode(MANUVR_LOGO_LED,  OUTPUT);
  
  Kernel __kernel;  // Instance the kernel.
  kernel = &__kernel;
  

  #if defined(__MK20DX256__) | defined(__MK20DX128__)
  
  // Create the main thread.
  xTaskCreate(mainTaskFxn, "Main", 3000, NULL, 1, &kernel_pid);

  // Create the scheduler thread. Let's see if this flies....
  xTaskCreate(schedulerTaskFxn, "Sched", 3000, (void*)kernel, 1, NULL );
  xTaskCreate(debugDumpTaskFxn, "Debug", 2000, (void*)kernel, 1, NULL );
  xTaskCreate(serialIOTaskFxn, "Serial", 512,  (void*)kernel, 1, NULL );
  
  xTaskCreate(loggerTask, "Logger", 100, (void*)kernel, 2, &logger_pid);
  __kernel.provideKernelPID(kernel_pid);
  __kernel.provideLoggerPID(logger_pid);


  kernel->createSchedule(40, -1, false, logo_fade);
  kernel->createSchedule(25, -1, false, blink_led);

//  mcp73833 = new MCP73833(9, 10);
  
  // Setup the first i2c adapter and Subscribe it to Kernel.
  I2CAdapter i2c(0);
//  mgc3130 = new MGC3130(16, 17);

  INA219 ina219(0x4A);
  ADP8866 adp8866(7, 8, 0x27);

  __kernel.subscribe((EventReceiver*) &i2c);
  __kernel.subscribe((EventReceiver*) &adp8866);

  i2c.addSlaveDevice(&ina219);
  i2c.addSlaveDevice(&adp8866);
  
//  mgc3130->init();

  __kernel.bootstrap();
  vTaskStartScheduler();
 
  for(;;);
  
  #elif defined(_BOARD_FUBARINO_MINI_)
  attachCoreTimerService(timerCallbackScheduler);
  sei();
  #endif
}




#if defined(__MK20DX256__) | defined(__MK20DX128__)

void mainTaskFxn(void *pvParameters) {
  int k_return = 0;
  
  /* As per most tasks, this task is implemented in an infinite loop. */
  for(;;) {
    k_return = kernel->procIdleFlags();

    if (0 == k_return) {
      vTaskSuspend(NULL);   // Nothing more to do.
    }
  }
}


void serialIOTaskFxn(void *pvParameters) {
  unsigned char* ser_buffer = (unsigned char*) alloca(255);
  int bytes_read = 0;
  for(;;) {
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
    else {
      vTaskDelay(11 / portTICK_PERIOD_MS);
    }
  }
}


void schedulerTaskFxn(void *pvParameters) {
  int s_return = 0;
  for(;;) {
    // TODO: This sucks. There must be a better way of having the kernel's
    //   sense of time not being subservient to FreeRTOS's...
    kernel->advanceScheduler();
    s_return = kernel->serviceScheduledEvents();
    vTaskDelay( 10 / portTICK_PERIOD_MS );
  }
}

void debugDumpTaskFxn(void *pvParameters) {
  StringBuilder output;
  for(;;) {
    kernel->printDebug(&output);
    Kernel::log(&output);
    vTaskDelay( 5000 / portTICK_PERIOD_MS );
  }
}


void loggerTask(void *pvParameters) {
  for(;;) {
    if (Kernel::log_buffer.count()) {
      taskENTER_CRITICAL();
      if (!kernel->getVerbosity()) {
        Kernel::log_buffer.clear();
      }
      else {
        Serial.print((char*) Kernel::log_buffer.position(0));
        Kernel::log_buffer.drop_position(0);
      }
      taskEXIT_CRITICAL();
    }
    else {
      vTaskSuspend(NULL);   // Nothing more to do.
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
