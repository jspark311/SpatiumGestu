/*
File:   main.cpp
Author: J. Ian Lindsay
Date:   2015.06.01

Copyright 2016 Manuvr, Inc

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.

   __  ____   ___  ______ __ __ __ ___  ___      ___   ____  __  ______ __ __
  (( \ || \\ // \\ | || | || || || ||\\//||     // \\ ||    (( \ | || | || ||
   \\  ||_// ||=||   ||   || || || || \/ ||    (( ___ ||==   \\    ||   || ||
  \_)) ||    || ||   ||   || \\_// ||    ||     \\_|| ||___ \_))   ||   \\_//

This has tacit support for Fubarino Mini, Teensy3, and Raspberry Pi.
*/


#include <DataStructures/StringBuilder.h>
#include <Platform/Platform.h>

#include <Drivers/MGC3130/MGC3130.h>
#include <Drivers/ADP8866/ADP8866.h>

#include <Transports/ManuvrSerial/ManuvrSerial.h>
#include <XenoSession/Console/ManuvrConsole.h>

#define HOST_BAUD_RATE  115200

void blink_led();

Kernel* kernel           = nullptr;


#if defined(__MK20DX256__) || defined(__MK20DX128__)
  const uint8_t PIN_LED1 = 13;  // The LED is attached to pin 13 on the teensy3.
#elif defined(_BOARD_FUBARINO_MINI_)
  const uint8_t PIN_LED1 = 13; // The LED is attached to pin 13 on the Fubarino Mini.
#elif defined(RASPI) || defined(RASPI2)
  const uint8_t PIN_LED1 = 14; // The LED is attached to pin 14 on the RasPi.
#endif

#if defined(__MANUVR_FREERTOS)
  unsigned long logger_pid = 0;
  unsigned long kernel_pid = 0;

  void vApplicationTickHook() {
    // TODO: This sucks. There must be a better way of having the kernel's
    //   sense of time not being subservient to FreeRTOS's...
    blink_led();
    platform.advanceScheduler();
  }

  static void* mainThread(void* arg) {
    //kernel->createSchedule(100, -1, false, blink_led);
    //kernel->provideKernelPID(kernel_pid);
    //kernel->provideLoggerPID(logger_pid);

    while (1) {
      kernel->procIdleFlags();
      blink_led();
      //taskYIELD();
    }
    return nullptr;
  }
#endif  // __MANUVR_FREERTOS


void setup() {}


void loop() {
  platform.platformPreInit();
  gpioDefine(PIN_LED1, OUTPUT);

  kernel = platform.kernel();

  ManuvrSerial  _console_xport("U", HOST_BAUD_RATE);  // Indicate USB.
  kernel->subscribe((EventReceiver*) &_console_xport);

  I2CAdapter i2c(0);
  kernel->subscribe((EventReceiver*) &i2c);

  //MGC3130 mgc3130(16, 17);
  //mgc3130->init();
  //kernel->subscribe((EventReceiver*) &mgc3130);

  ADP8866 adp8866(7, 8, 0x27);
  kernel->subscribe((EventReceiver*) &adp8866);
  i2c.addSlaveDevice((I2CDevice*) &adp8866);
  for (int i = 0; i < 20; i++) {
    bool _state = true;
    for (int q = 0; q < 50000; q++) { setPin(PIN_LED1, _state);   }
    _state = !_state;
  }
  for (int q = 0; q < 100000; q++) {   setPin(PIN_LED1, false);   }

  platform.bootstrap();

  ManuvrConsole _console((BufferPipe*) &_console_xport);
  kernel->subscribe((EventReceiver*) &_console);

  #if defined(__MANUVR_FREERTOS)
    // create task at priority one
    int s1 = createThread(&kernel_pid, nullptr, mainThread,  (void*) kernel);

    // check for creation errors
    if (0 != s1) {
      Serial.println(F("Creation problem"));
      while(1);
    }

    // start scheduler
    vTaskStartScheduler();
    while(1);
  #else
    // Simple kernel-service looper.
    while (1) {
      kernel->procIdleFlags();
    }
  #endif
}


void blink_led() {  setPin(PIN_LED1, !readPin(PIN_LED1));  }


#if defined(__MANUVR_LINUX)
  // For linux builds, we provide a shunt into the loop function.
  int main(int argc, char *argv[]) {
    setup();
    loop();
  }
#endif
