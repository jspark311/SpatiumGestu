#include <FreeRTOS_ARM.h>
#include "FirmwareDefs.h"
#include <Platform/Platform.h>
#include <Kernel.h>
#include <DataStructures/StringBuilder.h>

#include <Drivers/MGC3130/MGC3130.h>
#include <Drivers/ADP8866/ADP8866.h>

#include <Transports/ManuvrSerial/ManuvrSerial.h>
#include <XenoSession/Console/ManuvrConsole.h>

#define HOST_BAUD_RATE  115200

void blink_led();

Kernel* kernel           = NULL;


#if defined(__MK20DX256__) || defined(__MK20DX128__)
TaskHandle_t logger_pid = 0;
TaskHandle_t kernel_pid = 0;

// The LED is attached to pin 13 on the teensy3.
const uint8_t PIN_LED1        = 13;

void vApplicationTickHook() {
  // TODO: This sucks. There must be a better way of having the kernel's
  //   sense of time not being subservient to FreeRTOS's...
  blink_led();
  kernel->advanceScheduler();
}


static void mainThread(void* arg) {
  pinMode(PIN_LED1, OUTPUT);
  ManuvrSerial  _console_xport("U", HOST_BAUD_RATE);  // Indicate USB.
  ManuvrConsole _console((BufferPipe*) &_console_xport);
  kernel->subscribe((EventReceiver*) &_console);
  kernel->subscribe((EventReceiver*) &_console_xport);

  I2CAdapter i2c(0);

  //mgc3130 = new MGC3130(16, 17);
  ADP8866 adp8866(7, 8, 0x27);
  kernel->subscribe((EventReceiver*) &adp8866);
  i2c.addSlaveDevice(&adp8866);

  //mgc3130->init();

  //kernel->createSchedule(100, -1, false, blink_led);
  kernel->provideKernelPID(kernel_pid);
  //kernel->provideLoggerPID(logger_pid);

  kernel->bootstrap();

  while (1) {
    kernel->procIdleFlags();
    taskYIELD();
  }
}



void setup() {
  portBASE_TYPE s1, s2, s3;

  Serial.begin(HOST_BAUD_RATE);
  kernel = Kernel::getInstance();

  // create task at priority one
  s1 = xTaskCreate(mainThread, "Main", 4096, NULL, 1, &kernel_pid);
  //s2 = xTaskCreate(loggerTask, "Log", 1024, NULL, 1, &logger_pid);

  // check for creation errors
  //if (s1 != pdPASS || s2 != pdPASS || s3 != pdPASS ) {
  if (s1 != pdPASS) {
    Serial.println(F("Creation problem"));
    while(1);
  }

  // start scheduler
  vTaskStartScheduler();

  while(1);
}

void loop() {
}

#elif defined(_BOARD_FUBARINO_MINI_)
  // The LED is attached to pin 13 on the Fubarino Mini.
  const uint8_t PIN_LED1        = 13;

  uint32_t timerCallbackScheduler(uint32_t currentTime) {
    if (kernel) kernel->advanceScheduler();
    return (currentTime + (CORE_TICK_RATE * 1));
  }


  void setup() {
    pinMode(PIN_LED1, OUTPUT);
    Serial.begin(HOST_BAUD_RATE);   // USB

    kernel = Kernel::getInstance();

    //mgc3130 = new MGC3130(16, 17);
    ADP8866 adp8866(7, 8, 0x27);
    kernel->subscribe((EventReceiver*) &adp8866);
    i2c.addSlaveDevice(&adp8866);

    //mgc3130->init();

    //kernel->createSchedule(100, -1, false, blink_led);

    attachCoreTimerService(timerCallbackScheduler);
    globalIRQEnable();
  }


  void loop() {
    unsigned char* ser_buffer = (unsigned char*) alloca(255);
    int bytes_read = 0;

    while (1) {   // Service this loop for-ev-ar
      while (Serial.available() == 0) {
        kernel->procIdleFlags();
      }

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
  }
#endif


void blink_led() {  setPin(PIN_LED1, !readPin(PIN_LED1));  }
