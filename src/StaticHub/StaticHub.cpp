/*
File:   StaticHub.cpp
Author: J. Ian Lindsay
Date:   2014.07.01


Copyright (C) 2014 J. Ian Lindsay
All rights reserved.

This library is free software; you can redistribute it and/or
modify it under the terms of the GNU Lesser General Public
License as published by the Free Software Foundation; either
version 2.1 of the License, or (at your option) any later version.

This library is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public
License along with this library; if not, write to the Free Software
Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA

*/

#include "StaticHub/StaticHub.h"
#include <ManuvrOS/Platform/Platform.h>

#include "ManuvrOS/Drivers/i2c-adapter/i2c-adapter.h"
#include <ManuvrOS/EventManager.h>
#include <ManuvrOS/XenoSession/XenoSession.h>

#include <unistd.h>

#include <ManuvrOS/Drivers/MCP73833/MCP73833.h>
#include <ManuvrOS/Drivers/MGC3130/MGC3130.h>
#include <ManuvrOS/Drivers/ADP8866/ADP8866.h>
#include <ManuvrOS/Drivers/INA219/INA219.h>

// Externs and prototypes...
extern volatile I2CAdapter* i2c;



/****************************************************************************************************
* Scheduler callbacks. Please note that these are NOT part of StaticHub.                            *
****************************************************************************************************/
/*
* Scheduler callback
* This gets called periodically to moderate traffic volume across the USB link
*   until I figure out how to sink a hook into the DMA interrupt to take its place.
*/
void stdout_funnel() {
  if (StaticHub::log_buffer.count()) {
    if (!StaticHub::getInstance()->getVerbosity()) {
      StaticHub::log_buffer.clear();
    }
    else {
      //printf("%s", StaticHub::log_buffer.position(0));
      Serial.print((char*) StaticHub::log_buffer.position(0));
      StaticHub::log_buffer.drop_position(0);
    }
  }
  else {
    StaticHub::getInstance()->disableLogCallback();
  }
}



/****************************************************************************************************
* Static initializers                                                                               *
****************************************************************************************************/
volatile StaticHub* StaticHub::INSTANCE = NULL;

bool StaticHub::mute_logger = false;



/****************************************************************************************************
* Logging members...                                                                                *
****************************************************************************************************/
StringBuilder StaticHub::log_buffer;

/*
* Logger pass-through functions. Please mind the variadics...
*/
volatile void StaticHub::log(int severity, const char *str) {
  if (!INSTANCE->verbosity) return;
  if (log_buffer.count() == 0) ((StaticHub*) INSTANCE)->__scheduler.enableSchedule(INSTANCE->pid_log_moderator);
  log_buffer.concat(str);
}

volatile void StaticHub::log(char *str) {
  if (!INSTANCE->verbosity) return;
  if (log_buffer.count() == 0) ((StaticHub*) INSTANCE)->__scheduler.enableSchedule(INSTANCE->pid_log_moderator);
  log_buffer.concat(str);
}

volatile void StaticHub::log(const char *str) {
  if (!INSTANCE->verbosity) return;
  if (log_buffer.count() == 0) ((StaticHub*) INSTANCE)->__scheduler.enableSchedule(INSTANCE->pid_log_moderator);
  log_buffer.concat(str);
}

volatile void StaticHub::log(const char *fxn_name, int severity, const char *str, ...) {
  if (!INSTANCE->verbosity) return;
  //log_buffer.concat("%d  %s:\t", severity, fxn_name);
  if (log_buffer.count() == 0) {
    if (NULL != INSTANCE) {
       if (INSTANCE->pid_log_moderator) {
         ((StaticHub*) INSTANCE)->__scheduler.enableSchedule(INSTANCE->pid_log_moderator);
       }
     }
  }
  log_buffer.concatf("%d  %s:\t", severity, fxn_name);
  va_list marker;
  
  va_start(marker, str);
  log_buffer.concatf(str, marker);
  va_end(marker);
}

volatile void StaticHub::log(StringBuilder *str) {
  if (!INSTANCE->verbosity) return;
  if (log_buffer.count() == 0) ((StaticHub*) INSTANCE)->__scheduler.enableSchedule(INSTANCE->pid_log_moderator);
  log_buffer.concatHandoff(str);
}


void StaticHub::disableLogCallback(){
  __scheduler.disableSchedule(pid_log_moderator);
}



/****************************************************************************************************
 ▄▄▄▄▄▄▄▄▄▄▄  ▄▄        ▄  ▄▄▄▄▄▄▄▄▄▄▄  ▄▄▄▄▄▄▄▄▄▄▄ 
▐░░░░░░░░░░░▌▐░░▌      ▐░▌▐░░░░░░░░░░░▌▐░░░░░░░░░░░▌
 ▀▀▀▀█░█▀▀▀▀ ▐░▌░▌     ▐░▌ ▀▀▀▀█░█▀▀▀▀  ▀▀▀▀█░█▀▀▀▀ 
     ▐░▌     ▐░▌▐░▌    ▐░▌     ▐░▌          ▐░▌     
     ▐░▌     ▐░▌ ▐░▌   ▐░▌     ▐░▌          ▐░▌     
     ▐░▌     ▐░▌  ▐░▌  ▐░▌     ▐░▌          ▐░▌     
     ▐░▌     ▐░▌   ▐░▌ ▐░▌     ▐░▌          ▐░▌     
     ▐░▌     ▐░▌    ▐░▌▐░▌     ▐░▌          ▐░▌     
 ▄▄▄▄█░█▄▄▄▄ ▐░▌     ▐░▐░▌ ▄▄▄▄█░█▄▄▄▄      ▐░▌     
▐░░░░░░░░░░░▌▐░▌      ▐░░▌▐░░░░░░░░░░░▌     ▐░▌     
 ▀▀▀▀▀▀▀▀▀▀▀  ▀        ▀▀  ▀▀▀▀▀▀▀▀▀▀▀       ▀     

StaticHub bootstrap and setup fxns. This code is only ever called to initiallize this or that thing,
  and then becomes inert.
****************************************************************************************************/
  
/*
* The primary init function. Calling this will bring the entire system online if everything is
*   working nominally.
*/
int8_t StaticHub::bootstrap() {
  log_buffer.concatf("\n\n%s v%s    Build date: %s %s\nBootstrap beginning...\n", IDENTITY_STRING, VERSION_STRING, __DATE__, __TIME__);

  event_manager.subscribe((EventReceiver*) &__scheduler);    // Subscribe the Scheduler.
  event_manager.subscribe((EventReceiver*) this);            // Subscribe StaticHub as top priority in EventManager.

  mcp73833 = new MCP73833(9, 10);
  
  // Setup the first i2c adapter and Subscribe it to EventManager.
  i2c     = new I2CAdapter(0);
  mgc3130 = new MGC3130(16, 17);

  ina219      = new INA219(0x4A);
  adp8866     = new ADP8866(7, 8, 0x27);

  event_manager.subscribe((EventReceiver*) i2c);
  event_manager.subscribe((EventReceiver*) adp8866);

  ((I2CAdapter*) i2c)->addSlaveDevice(ina219);
  ((I2CAdapter*) i2c)->addSlaveDevice(adp8866);
  
  platformInit();    // Start the platform-specific machinery.
  
  // We know we will need some schedules...
  pid_log_moderator = __scheduler.createSchedule(8,  -1, false, stdout_funnel);

//  mgc3130->init();

  ManuvrEvent *boot_completed_ev = EventManager::returnEvent(MANUVR_MSG_SYS_BOOT_COMPLETED);
  //boot_completed_ev->priority = EVENT_PRIORITY_HIGHEST;
  raiseEvent(boot_completed_ev);
  return 0;
}


/*
* All external access to StaticHub's non-static members should obtain it's reference via this fxn...
*   Note that services that are dependant on SH during the bootstrap phase should have an SH reference
*   passed into their constructors, rather than forcing them to call this and risking an infinite 
*   recursion.
*/
StaticHub* StaticHub::getInstance() {
  if (StaticHub::INSTANCE == NULL) {
    StaticHub::INSTANCE = new StaticHub();
  }
  // And that is how the singleton do...
  return (StaticHub*) StaticHub::INSTANCE;
}


StaticHub::StaticHub() {
  StaticHub::INSTANCE = this;
  scheduler = &__scheduler;
  bootstrap();
}




/****************************************************************************************************
* Resource fetch functions...                                                                       *
****************************************************************************************************/

Scheduler*    StaticHub::fetchScheduler(void) {     return &__scheduler;     }
EventManager* StaticHub::fetchEventManager(void) {  return &event_manager;   }
MGC3130*      StaticHub::fetchMGC3130(void) {       return mgc3130;          }


/****************************************************************************************************
*  ▄▄▄▄▄▄▄▄▄▄▄  ▄               ▄  ▄▄▄▄▄▄▄▄▄▄▄  ▄▄        ▄  ▄▄▄▄▄▄▄▄▄▄▄  ▄▄▄▄▄▄▄▄▄▄▄ 
* ▐░░░░░░░░░░░▌▐░▌             ▐░▌▐░░░░░░░░░░░▌▐░░▌      ▐░▌▐░░░░░░░░░░░▌▐░░░░░░░░░░░▌
* ▐░█▀▀▀▀▀▀▀▀▀  ▐░▌           ▐░▌ ▐░█▀▀▀▀▀▀▀▀▀ ▐░▌░▌     ▐░▌ ▀▀▀▀█░█▀▀▀▀ ▐░█▀▀▀▀▀▀▀▀▀ 
* ▐░▌            ▐░▌         ▐░▌  ▐░▌          ▐░▌▐░▌    ▐░▌     ▐░▌     ▐░▌          
* ▐░█▄▄▄▄▄▄▄▄▄    ▐░▌       ▐░▌   ▐░█▄▄▄▄▄▄▄▄▄ ▐░▌ ▐░▌   ▐░▌     ▐░▌     ▐░█▄▄▄▄▄▄▄▄▄ 
* ▐░░░░░░░░░░░▌    ▐░▌     ▐░▌    ▐░░░░░░░░░░░▌▐░▌  ▐░▌  ▐░▌     ▐░▌     ▐░░░░░░░░░░░▌
* ▐░█▀▀▀▀▀▀▀▀▀      ▐░▌   ▐░▌     ▐░█▀▀▀▀▀▀▀▀▀ ▐░▌   ▐░▌ ▐░▌     ▐░▌      ▀▀▀▀▀▀▀▀▀█░▌
* ▐░▌                ▐░▌ ▐░▌      ▐░▌          ▐░▌    ▐░▌▐░▌     ▐░▌               ▐░▌
* ▐░█▄▄▄▄▄▄▄▄▄        ▐░▐░▌       ▐░█▄▄▄▄▄▄▄▄▄ ▐░▌     ▐░▐░▌     ▐░▌      ▄▄▄▄▄▄▄▄▄█░▌
* ▐░░░░░░░░░░░▌        ▐░▌        ▐░░░░░░░░░░░▌▐░▌      ▐░░▌     ▐░▌     ▐░░░░░░░░░░░▌
*  ▀▀▀▀▀▀▀▀▀▀▀          ▀          ▀▀▀▀▀▀▀▀▀▀▀  ▀        ▀▀       ▀       ▀▀▀▀▀▀▀▀▀▀▀ 
* 
* These are overrides from EventReceiver interface...
****************************************************************************************************/

/*
* Some peripherals and operations need a bit of time to complete. This function is called from a
*   one-shot schedule and performs all of the cleanup for latent consequences of bootstrap().
*/
int8_t StaticHub::bootComplete() {
  maskableInterrupts(true);  // Now configure interrupts, lift interrupt masks, and let the madness begin.
  return 1;
}



/**
* If we find ourselves in this fxn, it means an event that this class built (the argument)
*   has been serviced and we are now getting the chance to see the results. The argument 
*   to this fxn will never be NULL.
*
* Depending on class implementations, we might choose to handle the completed Event differently. We 
*   might add values to event's Argument chain and return RECYCLE. We may also free() the event
*   ourselves and return DROP. By default, we will return REAP to instruct the EventManager
*   to either free() the event or return it to it's preallocate queue, as appropriate. If the event
*   was crafted to not be in the heap in its own allocation, we will return DROP instead.
*
* @param  event  The event for which service has been completed.
* @return A callback return code.
*/
int8_t StaticHub::callback_proc(ManuvrEvent *event) {
  /* Setup the default return code. If the event was marked as mem_managed, we return a DROP code.
     Otherwise, we will return a REAP code. Downstream of this assignment, we might choose differently. */ 
  int8_t return_value = event->eventManagerShouldReap() ? EVENT_CALLBACK_RETURN_REAP : EVENT_CALLBACK_RETURN_DROP;
  
  /* Some class-specific set of conditionals below this line. */
  switch (event->event_code) {
    case MANUVR_MSG_SYS_BOOT_COMPLETED:
      StaticHub::log("Boot complete.\n");
      boot_completed = true;
      break;
    default:
      break;
  }
  
  return return_value;
}



int8_t StaticHub::notify(ManuvrEvent *active_event) {
  int8_t return_value = 0;
  
  switch (active_event->event_code) {
    case MANUVR_MSG_USER_DEBUG_INPUT:
      last_user_input.concatHandoff(&usb_rx_buffer);
      procDirectDebugInstruction(&last_user_input);
      return_value++;
      break;

    case MANUVR_MSG_SYS_ISSUE_LOG_ITEM:
      {
        StringBuilder *log_item;
        if (0 == active_event->getArgAs(&log_item)) {
          log_buffer.concatHandoff(log_item);
        }
        __scheduler.enableSchedule(pid_log_moderator);
      }
      break;
      
    default:
      return_value += EventReceiver::notify(active_event);
      break;
  }
  return return_value;
}



/****************************************************************************************************
 ▄▄▄▄▄▄▄▄▄▄▄  ▄▄▄▄▄▄▄▄▄▄▄  ▄▄▄▄▄▄▄▄▄▄▄       ▄            ▄▄▄▄▄▄▄▄▄▄▄  ▄▄        ▄  ▄▄▄▄▄▄▄▄▄▄  
▐░░░░░░░░░░░▌▐░░░░░░░░░░░▌▐░░░░░░░░░░░▌     ▐░▌          ▐░░░░░░░░░░░▌▐░░▌      ▐░▌▐░░░░░░░░░░▌ 
 ▀▀▀▀█░█▀▀▀▀ ▐░█▀▀▀▀▀▀▀▀▀ ▐░█▀▀▀▀▀▀▀█░▌     ▐░▌          ▐░█▀▀▀▀▀▀▀█░▌▐░▌░▌     ▐░▌▐░█▀▀▀▀▀▀▀█░▌
     ▐░▌     ▐░▌          ▐░▌       ▐░▌     ▐░▌          ▐░▌       ▐░▌▐░▌▐░▌    ▐░▌▐░▌       ▐░▌
     ▐░▌     ▐░█▄▄▄▄▄▄▄▄▄ ▐░█▄▄▄▄▄▄▄█░▌     ▐░▌          ▐░█▄▄▄▄▄▄▄█░▌▐░▌ ▐░▌   ▐░▌▐░▌       ▐░▌
     ▐░▌     ▐░░░░░░░░░░░▌▐░░░░░░░░░░░▌     ▐░▌          ▐░░░░░░░░░░░▌▐░▌  ▐░▌  ▐░▌▐░▌       ▐░▌
     ▐░▌      ▀▀▀▀▀▀▀▀▀█░▌▐░█▀▀▀▀█░█▀▀      ▐░▌          ▐░█▀▀▀▀▀▀▀█░▌▐░▌   ▐░▌ ▐░▌▐░▌       ▐░▌
     ▐░▌               ▐░▌▐░▌     ▐░▌       ▐░▌          ▐░▌       ▐░▌▐░▌    ▐░▌▐░▌▐░▌       ▐░▌
 ▄▄▄▄█░█▄▄▄▄  ▄▄▄▄▄▄▄▄▄█░▌▐░▌      ▐░▌      ▐░█▄▄▄▄▄▄▄▄▄ ▐░▌       ▐░▌▐░▌     ▐░▐░▌▐░█▄▄▄▄▄▄▄█░▌
▐░░░░░░░░░░░▌▐░░░░░░░░░░░▌▐░▌       ▐░▌     ▐░░░░░░░░░░░▌▐░▌       ▐░▌▐░▌      ▐░░▌▐░░░░░░░░░░▌ 
 ▀▀▀▀▀▀▀▀▀▀▀  ▀▀▀▀▀▀▀▀▀▀▀  ▀         ▀       ▀▀▀▀▀▀▀▀▀▀▀  ▀         ▀  ▀        ▀▀  ▀▀▀▀▀▀▀▀▀▀ 

Interrupt service routine support functions...                                                                    
A quick note is in order. These functions are static class members that are called directly from  
  the ISRs in stm32f4xx_it.c. They are not themselves ISRs. Keep them as short as possible.                                     

TODO: Keep them as short as possible.                                                             
****************************************************************************************************/


/*
* This is called from the USB peripheral. It is called when the short static
* character array that forms the USB rx buffer is either filled up, or we see
* a new-line character on the wire.
*/
void StaticHub::feedUSBBuffer(uint8_t *buf, int len, bool terminal) {
  usb_rx_buffer.concat(buf, len);
  
  if (terminal) {
    // If the ISR saw a CR or LF on the wire, we tell the parser it is ok to
    // run in idle time.
    ManuvrEvent* event = EventManager::returnEvent(MANUVR_MSG_USER_DEBUG_INPUT);
    event->specific_target = (EventReceiver*) this;
    raiseEvent(event);
  }
}



/****************************************************************************************************
 ▄▄▄▄▄▄▄▄▄▄   ▄▄▄▄▄▄▄▄▄▄▄  ▄▄▄▄▄▄▄▄▄▄   ▄         ▄  ▄▄▄▄▄▄▄▄▄▄▄ 
▐░░░░░░░░░░▌ ▐░░░░░░░░░░░▌▐░░░░░░░░░░▌ ▐░▌       ▐░▌▐░░░░░░░░░░░▌
▐░█▀▀▀▀▀▀▀█░▌▐░█▀▀▀▀▀▀▀▀▀ ▐░█▀▀▀▀▀▀▀█░▌▐░▌       ▐░▌▐░█▀▀▀▀▀▀▀▀▀ 
▐░▌       ▐░▌▐░▌          ▐░▌       ▐░▌▐░▌       ▐░▌▐░▌          
▐░▌       ▐░▌▐░█▄▄▄▄▄▄▄▄▄ ▐░█▄▄▄▄▄▄▄█░▌▐░▌       ▐░▌▐░▌ ▄▄▄▄▄▄▄▄ 
▐░▌       ▐░▌▐░░░░░░░░░░░▌▐░░░░░░░░░░▌ ▐░▌       ▐░▌▐░▌▐░░░░░░░░▌
▐░▌       ▐░▌▐░█▀▀▀▀▀▀▀▀▀ ▐░█▀▀▀▀▀▀▀█░▌▐░▌       ▐░▌▐░▌ ▀▀▀▀▀▀█░▌
▐░▌       ▐░▌▐░▌          ▐░▌       ▐░▌▐░▌       ▐░▌▐░▌       ▐░▌
▐░█▄▄▄▄▄▄▄█░▌▐░█▄▄▄▄▄▄▄▄▄ ▐░█▄▄▄▄▄▄▄█░▌▐░█▄▄▄▄▄▄▄█░▌▐░█▄▄▄▄▄▄▄█░▌
▐░░░░░░░░░░▌ ▐░░░░░░░░░░░▌▐░░░░░░░░░░▌ ▐░░░░░░░░░░░▌▐░░░░░░░░░░░▌
 ▀▀▀▀▀▀▀▀▀▀   ▀▀▀▀▀▀▀▀▀▀▀  ▀▀▀▀▀▀▀▀▀▀   ▀▀▀▀▀▀▀▀▀▀▀  ▀▀▀▀▀▀▀▀▀▀▀ 
 
Code in here only exists for as long as it takes to debug something. Don't write against these fxns.
****************************************************************************************************/

/**
* Debug support function.
*
* @return a pointer to a string constant.
*/
const char* StaticHub::getReceiverName() {  return "StaticHub";  }


/**
* Debug support method. This fxn is only present in debug builds. 
*
* @param   StringBuilder* The buffer into which this fxn should write its output.
*/
void StaticHub::printDebug(StringBuilder* output) {
  if (NULL == output) return;
  uint32_t initial_sp = getStackPointer();
  uint32_t final_sp = getStackPointer();
  
  EventReceiver::printDebug(output);
  output->concatf("--- %s v%s    Build date: %s %s\n\n", IDENTITY_STRING, VERSION_STRING, __DATE__, __TIME__);
  output->concatf("---\n--- boot_completed  %s\n", (boot_completed) ? "yes" : "no");
  output->concatf("---\n--- stack grows %s\n", (final_sp > initial_sp) ? "up" : "down");
  output->concatf("--- getStackPointer()   0x%08x\n---\n", getStackPointer());

  output->concatf("--- millis()            0x%08x\n", millis());
  output->concatf("--- micros()            0x%08x\n\n\n", micros());
  currentDateTime(output);
}




void StaticHub::procDirectDebugInstruction(StringBuilder* input) {
  char *str = (char *) input->string();
  char c = *(str);
  uint8_t temp_byte = 0;        // Many commands here take a single integer argument.
  if (*(str) != 0) {
    temp_byte = atoi((char*) str+1);
  }
  ManuvrEvent *event = NULL;  // Pitching events is a common thing in this fxn...
  
  StringBuilder parse_mule;
  
  switch (c) {
    case 'B':
      if (temp_byte == 128) {
        EventManager::raiseEvent(MANUVR_MSG_SYS_BOOTLOADER, NULL);
        break;
      }
      local_log.concatf("Will only jump to bootloader if the number '128' follows the command.\n");
      break;
    case 'b':
      if (temp_byte == 128) {
        EventManager::raiseEvent(MANUVR_MSG_SYS_REBOOT, NULL);
        break;
      }
      local_log.concatf("Will only reboot if the number '128' follows the command.\n");
      break;

    case '6':        // Read so many random integers...
      { // TODO: I don't think the RNG is ever being turned off. Save some power....
        temp_byte = (temp_byte == 0) ? PLATFORM_RNG_CARRY_CAPACITY : temp_byte;
        for (uint8_t i = 0; i < temp_byte; i++) {
          uint32_t x = randomInt();
          if (x) {
            local_log.concatf("Random number: 0x%08x\n", x);
          }
          else {
            local_log.concatf("Restarting RNG\n");
            init_RNG();
          }
        }
      }
      break;

#ifdef __MANUVR_CONSOLE_SUPPORT

    case 'u':
      switch (temp_byte) {
        case 1:
          EventManager::raiseEvent(MANUVR_MSG_SELF_DESCRIBE, NULL);
          break;
        case 3:
          EventManager::raiseEvent(MANUVR_MSG_LEGEND_MESSAGES, NULL);
          break;
        default:
          break;
      }
      break;

    case 'y':    // Power mode.
      switch (temp_byte) {
        case 255:
          break;
        default:
          event = EventManager::returnEvent(MANUVR_MSG_SYS_POWER_MODE);
          event->addArg((uint8_t) temp_byte);
          raiseEvent(event);
          local_log.concatf("Power mode is now %d.\n", temp_byte);
          break;
      }
      break;

    case 'i':   // StaticHub debug prints.
      if (1 == temp_byte) {
        local_log.concat("EventManager profiling enabled.\n");
        event_manager.profiler(true);
      }
      else if (2 == temp_byte) {
        event_manager.printDebug(&local_log);
      }
      else if (5 == temp_byte) {
        event_manager.clean_first_discard();
        local_log.concat("EventManager cleaned the head of the discard queue.\n");
      }
      else if (6 == temp_byte) {
        local_log.concat("EventManager profiling disabled.\n");
        event_manager.profiler(false);
      }
      else {
        printDebug(&local_log);
      }
      break;


    case 'v':           // Set log verbosity.
      parse_mule.concat(str);
      local_log.concatf("parse_mule split (%s) into %d positions. \n", str, parse_mule.split(" "));
      parse_mule.drop_position(0);
      
      event = new ManuvrEvent(MANUVR_MSG_SYS_LOG_VERBOSITY);
      switch (parse_mule.count()) {
        case 2:
          event->specific_target = event_manager.getSubscriberByName((const char*) (parse_mule.position_trimmed(1)));
          local_log.concatf("Directing verbosity change to %s.\n", (NULL == event->specific_target) ? "NULL" : event->specific_target->getReceiverName());
        case 1:
          event->addArg((uint8_t) parse_mule.position_as_int(0));
          break;
        default:
          break;
      }
      raiseEvent(event);
      break;

    case 'p':
      ina219->sync();
      break;
    case 'n':
      ina219->init();
      break;
    case 'k':
      ina219->readSensor();
      break;

    case 'm':
      input->cull(1);
      mgc3130->procDirectDebugInstruction(input);
      break;

    case 'l':
      input->cull(1);
      adp8866->procDirectDebugInstruction(input);
      break;

    case 'z':
      input->cull(1);
      ((I2CAdapter*) i2c)->procDirectDebugInstruction(input);
      break;
    #endif

    default:
      break;
  }
  if (local_log.length() > 0) StaticHub::log(&local_log);
  last_user_input.clear();
}

