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
#include "ManuvrOS/Drivers/i2c-adapter/i2c-adapter.h"
#include <ManuvrOS/EventManager.h>
#include <ManuvrOS/XenoSession/XenoSession.h>

#if defined(__MK20DX256__) | defined(__MK20DX128__)
  #include <Time/Time.h>
#elif defined(__PIC32MX2XX__) | defined(__PIC32MX3XX__) | defined(__PIC32MZ__)
  #include <wiring.h>
#else
  #include <sys/time.h>
#endif  
  
#include <unistd.h>

#include <ManuvrOS/Drivers/MGC3130/MGC3130.h>
#include <ManuvrOS/Drivers/ADP8866/ADP8866.h>
#include <ManuvrOS/Drivers/INA219/INA219.h>

// Externs and prototypes...
extern volatile I2CAdapter* i2c;
uint32_t profiler_mark_0 = 0;
uint32_t profiler_mark_1 = 0;

unsigned long start_time_micros = 0;



// These are only here until they are migrated to each receiver that deals with them.
const MessageTypeDef message_defs_dev_specific[] = {
  {  MANUVR_MSG_GESTURE_LEGEND            , MSG_FLAG_EXPORTABLE,  "GESTURE_LEGEND",            ManuvrMsg::MSG_ARGS_NONE }, //
  {  MANUVR_MSG_GESTURE_DEFINITION        , MSG_FLAG_EXPORTABLE,  "GESTURE_DEFINITION",        ManuvrMsg::MSG_ARGS_NONE }, //
  {  MANUVR_MSG_GESTURE_OBLITERATE        , MSG_FLAG_EXPORTABLE,  "GESTURE_OBLITERATE",        ManuvrMsg::MSG_ARGS_NONE }, //
  {  MANUVR_MSG_GESTURE_LINK              , MSG_FLAG_EXPORTABLE,  "GESTURE_LINK",              ManuvrMsg::MSG_ARGS_NONE }, //
  {  MANUVR_MSG_GESTURE_UNLINK            , MSG_FLAG_EXPORTABLE,  "GESTURE_UNLINK",            ManuvrMsg::MSG_ARGS_NONE }, //
  {  MANUVR_MSG_GESTURE_RECOGNIZED        , MSG_FLAG_EXPORTABLE,  "GESTURE_RECOGNIZED",        ManuvrMsg::MSG_ARGS_NONE }, //
  {  MANUVR_MSG_GESTURE_NUANCE            , MSG_FLAG_EXPORTABLE,  "GESTURE_NUANCE",            ManuvrMsg::MSG_ARGS_NONE }, //
  {  MANUVR_MSG_GESTURE_DISASSERT         , MSG_FLAG_EXPORTABLE,  "GESTURE_DISASSERT",         ManuvrMsg::MSG_ARGS_NONE }, //
  {  MANUVR_MSG_GESTURE_ONE_SHOT          , MSG_FLAG_EXPORTABLE,  "GESTURE_ONE_SHOT",          ManuvrMsg::MSG_ARGS_NONE }, //
  {  MANUVR_MSG_SENSOR_MGC3130            , MSG_FLAG_IDEMPOTENT,  "MGC3130",                   ManuvrMsg::MSG_ARGS_NONE }, //
  {  MANUVR_MSG_SENSOR_MGC3130_INIT       , MSG_FLAG_IDEMPOTENT,  "MGC3130_INIT",              ManuvrMsg::MSG_ARGS_NONE }, //
};




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
volatile uint32_t StaticHub::next_random_int[STATICHUB_RNG_CARRY_CAPACITY];
volatile uint32_t StaticHub::millis_since_reset = 1;   // Start at one because WWDG.
volatile uint8_t  StaticHub::watchdog_mark = 42;

bool StaticHub::mute_logger = false;



/****************************************************************************************************
* Functions that convert from #define codes to something readable by a human...                     *
****************************************************************************************************/
const char* StaticHub::getRTCStateString(uint32_t code) {
  switch (code) {
    case MANUVR_RTC_STARTUP_UNKNOWN:     return "RTC_STARTUP_UNKNOWN";
    case MANUVR_RTC_OSC_FAILURE:         return "RTC_OSC_FAILURE";
    case MANUVR_RTC_STARTUP_GOOD_UNSET:  return "RTC_STARTUP_GOOD_UNSET";
    case MANUVR_RTC_STARTUP_GOOD_SET:    return "RTC_STARTUP_GOOD_SET";
    default:                             return "RTC_STARTUP_UNDEFINED";
  }
}



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
* Various small utility functions...                                                                *
****************************************************************************************************/

/**
* Sometimes we question the size of the stack.
*
* @return the stack pointer at call time.
*/
volatile uint32_t StaticHub::getStackPointer(void) {
  uint32_t test;
  test = (uint32_t) &test;  // Store the pointer.
  return test;
}


/**
* Called by the RNG ISR to provide new random numbers to StaticHub.
*
* @param    nu_rnd The supplied random number.
* @return   True if the RNG should continue supplying us, false if it should take a break until we need more.
*/
volatile bool StaticHub::provide_random_int(uint32_t nu_rnd) {
  for (uint8_t i = 0; i < STATICHUB_RNG_CARRY_CAPACITY; i++) {
    if (next_random_int[i] == 0) {
      next_random_int[i] = nu_rnd;
      return (i == STATICHUB_RNG_CARRY_CAPACITY-1) ? false : true;
    }
  }
  return false;
}


/**
* Dead-simple interface to the RNG. Despite the fact that it is interrupt-driven, we may resort
*   to polling if random demand exceeds random supply. So this may block until a random number
*   is actually availible (next_random_int != 0).
*
* @return   A 32-bit unsigned random number. This can be cast as needed.
*/
uint32_t StaticHub::randomInt(void) {
  uint32_t return_value = rand();
  return return_value;
}


/*
* Given an RFC2822 datetime string, decompose it and set the time and date.
* We would prefer RFC2822, but we should try and cope with things like missing
*   time or timezone.
* Returns false if the date failed to set. True if it did.
*/
bool StaticHub::setTimeAndDate(char* nu_date_time) {
  return false;
}


/*
* Returns an integer representing the current datetime.
*/
uint32_t StaticHub::currentTimestamp(void) {
  uint32_t return_value = 0;
  return return_value;
}

/*
* Same, but writes a string representation to the argument.
*/
void StaticHub::currentTimestamp(StringBuilder* target) {
}

/*
* Writes a human-readable datetime to the argument.
* Returns ISO 8601 datetime string.
* 2004-02-12T15:19:21+00:00
*/
void StaticHub::currentDateTime(StringBuilder* target) {
	if (target != NULL) {
	  #ifdef STM32F4XX
		  RTC_TimeTypeDef RTC_TimeStructure;
		  RTC_DateTypeDef RTC_DateStructure;
		  RTC_GetTimeStamp(RTC_Format_BIN, &RTC_TimeStructure, &RTC_DateStructure);
		  
		  TM_RTC_Time_t current_datetime;
		  TM_RTC_GetDateTime(&current_datetime, TM_RTC_Format_BIN);
		  target->concatf("%02d.%02d.%04d %02d:%02d:%02d  Unix: %u\n",
                  current_datetime.date,
                  current_datetime.month,
                  current_datetime.year + 2000,
                  current_datetime.hours,
                  current_datetime.minutes,
                  current_datetime.seconds,
                  current_datetime.unix);
		  
		  target->concatf("%d-%d-%dT", RTC_DateStructure.RTC_Year, RTC_DateStructure.RTC_Month, RTC_DateStructure.RTC_Date);
		  target->concatf("%d:%d:%d+00:00", RTC_TimeStructure.RTC_Hours, RTC_TimeStructure.RTC_Minutes, RTC_TimeStructure.RTC_Seconds);
		#elif defined(__MK20DX256__) | defined(__MK20DX128__)
		  target->concatf("%04d-%02d-%02dT", year(), month(), day());
		  target->concatf("%02d:%02d:%02d+00:00", hour(), minute(), second());
		#else
		  target->concat("<UNSUPPORTED PLATFORM>\n");
		#endif
	}
}




/****************************************************************************************************
* Functions that deal with clock and mode switching...                                              *
****************************************************************************************************/

/*
* Call this with a boolean to enable or disable maskable interrupts globally.
* NOTE: This includes USB and SysTick. So no host communication, and no scheduler.
*       Events ought to still work, however.
*/
void StaticHub::maskable_interrupts(bool enable) {
  if (enable) {
    sei();
    StaticHub::log("All interrupts enabled\n");
  }
  else {
    cli();
    StaticHub::log("All interrupts masked\n");
    EventManager::raiseEvent(MANUVR_MSG_INTERRUPTS_MASKED, NULL);
  }
}


/*
* From this fxn will grow the IRQ aspect of our mode-switching (power use).
*   It would be prefferable to contain this machinery within the classes that
*   each block pertains to, but for debugging sake, they are centrallized here
*   for the moment.
* The problem is related to hard faults when the peripherals are allowed to bring
*   their own interrupts online. Solve that before trying to make this pretty.
*/
void StaticHub::off_class_interrupts(bool enable) {
  if (enable) {  sei();  }
  else {         cli();  }
}


volatile void jumpToBootloader(void) {
  #if defined(__MK20DX256__) | defined(__MK20DX128__)
    _reboot_Teensyduino_();
  #elif defined(__PIC32MX2XX__) | defined(__PIC32MX3XX__)
    executeSoftReset(0);
  #endif
}


volatile void reboot(void) {
  #if defined(__MK20DX256__) | defined(__MK20DX128__)
    *((uint32_t *)0xE000ED0C) = 0x5FA0004;
  #elif defined(__PIC32MX2XX__) | defined(__PIC32MX3XX__)
    executeSoftReset(ENTER_BOOTLOADER_ON_BOOT);
  #endif
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
* When the system comes to life, we will want to setup some periodic schedules.
* Even for schedules that are meant to be one-shots, we will try to avoid a malloc()/free()
*   cycle if it's going to be a regular event. Just pre-build some schedules that we know
*   we will need and let them sit dormant until needed.
*/
void StaticHub::initSchedules(void) {
  // This schedule marches the data into the USB VCP at a throttled rate.
  pid_log_moderator = __scheduler.createSchedule(8,  -1, false, stdout_funnel);
  //__scheduler.delaySchedule(pid_log_moderator, 5000);
  //__scheduler.disableSchedule(pid_log_moderator);
}


/*
* Init the RNG. Short and sweet.
*/
void StaticHub::init_RNG(void) volatile {
  // Zero the random number cache.
  for (uint8_t i = 0; i < STATICHUB_RNG_CARRY_CAPACITY; i++) next_random_int[i] = 0;
  
  #ifdef ARDUINO
    //randomSeed(analogRead(0));
  #endif
}


/*
* Setup the realtime clock module.
* Informed by code here:
* http://autoquad.googlecode.com/svn/trunk/onboard/rtc.c
*/
#if defined(__MK20DX256__) | defined(__MK20DX128__)
  time_t getTeensy3Time() {   return Teensy3Clock.get();   }
#endif

void StaticHub::initRTC(void) volatile {
  #if defined(__MK20DX256__) | defined(__MK20DX128__)
    setSyncProvider(getTeensy3Time);
    if (timeStatus() != timeSet) {
      rtc_startup_state = MANUVR_RTC_STARTUP_GOOD_UNSET;
    }
    else {
      rtc_startup_state = MANUVR_RTC_STARTUP_GOOD_SET;
    }
  #endif
}



/*
* This fxn should be called once on boot to setup the CPU pins that are not claimed
*   by other classes. GPIO pins at the command of this-or-that class should be setup 
*   in the class that deals with them. An example of this would be the CPLD pins (GPIO
*   case) or the i2c pins (AF case).
* Pending peripheral-level init of pins, we should just enable everything and let 
*   individual classes work out their own requirements.
* Since we are using a 100-pin QFP part, we don't have ports higher than E.
*/
void StaticHub::gpioSetup() volatile {
}



/*
* The last step in our bootstrap is to call this function. By this point, all peripherals and GPIO
*   should have been claimed and setup. Now we need to conf the interrupts we want.
*/
void StaticHub::nvicConf() volatile {
}

  
/*
* The primary init function. Calling this will bring the entire system online if everything is
*   working nominally.
*/
int8_t StaticHub::bootstrap() {
  log_buffer.concatf("\n\n%s v%s    Build date: %s %s\nBootstrap beginning...\n", IDENTITY_STRING, VERSION_STRING, __DATE__, __TIME__);

  // One of the first things we need to do is populate the EventManager with all of the
  // message codes that come with this firmware.
  int mes_count = sizeof(message_defs_dev_specific) / sizeof(MessageTypeDef);
  for (int i = 0; i < mes_count; i++) {
    ManuvrMsg::message_defs_extended.insert(&message_defs_dev_specific[i]);
//    log_buffer.concatf("Adding %s\n", message_defs_dev_specific[i].debug_label);
  }

  event_manager.subscribe((EventReceiver*) &__scheduler);    // Subscribe the Scheduler.
  event_manager.subscribe((EventReceiver*) this);            // Subscribe StaticHub as top priority in EventManager.

  // Setup the first i2c adapter and Subscribe it to EventManager.
  i2c     = new I2CAdapter(0);
  mgc3130 = new MGC3130(16, 17);

  ina219      = new INA219(0x4A);
  adp8866     = new ADP8866(7, 8, 0x27);

  event_manager.subscribe((EventReceiver*) i2c);
  ((I2CAdapter*) i2c)->addSlaveDevice(ina219);
  ((I2CAdapter*) i2c)->addSlaveDevice(adp8866);

  event_manager.subscribe((EventReceiver*) adp8866);
  
  init_RNG();      // Fire up the RNG...
  initRTC();
  initSchedules();   // We know we will need some schedules...

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
  start_time_micros = micros();
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
  //EventReceiver::bootComplete()  <--- Note that we aren't going to do this. We already know about the scheduler.
  off_class_interrupts(true);  // Now configure interrupts, lift interrupt masks, and let the madness begin.
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
  StringBuilder output;
  int8_t return_value = 0;
  
  switch (active_event->event_code) {
    case MANUVR_MSG_USER_DEBUG_INPUT:
      last_user_input.concatHandoff(&usb_rx_buffer);
      procDirectDebugInstruction(&last_user_input);
      return_value++;
      break;
    case MANUVR_MSG_SYS_REBOOT:
      maskable_interrupts(false);
      reboot();
      break;
    case MANUVR_MSG_SYS_BOOTLOADER:
      maskable_interrupts(false);
      jumpToBootloader();
      break;


    case MANUVR_MSG_RNG_BUFFER_EMPTY:
      output.concatf("RNG underrun.\n");
      break;

      
    case MANUVR_MSG_SESS_ESTABLISHED:
      // When we see session establishment, we broadcast our
      // legends, and self-describe.
      
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
  if (output.length() > 0) {    StaticHub::log(&output);  }
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
* Called from the SysTick handler once per ms.
*/
volatile void StaticHub::advanceScheduler(void) {
  // No watchdog timer on this platform. Therefore, we call
  //   the scheduler from the main loop.
}


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
  output->concatf("--- micros()            0x%08x\n", micros());
  currentDateTime(output);
  output->concatf("\nrtc_startup_state %s\n", getRTCStateString(rtc_startup_state));
}



void StaticHub::print_type_sizes(void) {
  StringBuilder temp("---< Type sizes >-----------------------------\n");
  StaticHub::log(&temp);
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
        temp_byte = (temp_byte == 0) ? STATICHUB_RNG_CARRY_CAPACITY : temp_byte;
        for (uint8_t i = 0; i < temp_byte; i++) {
          uint32_t x = StaticHub::randomInt();
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
      else if (3 == temp_byte) {
        print_type_sizes();
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

    case 'o':
      if (temp_byte) {
        event = new ManuvrEvent(MANUVR_MSG_SCHED_PROFILER_STOP);
        event->addArg(temp_byte);

        local_log.concatf("Stopped profiling schedule %d\n", temp_byte);
        event->printDebug(&local_log);
      }
      break;
    case 'O':
      if (temp_byte) {
        event = new ManuvrEvent(MANUVR_MSG_SCHED_PROFILER_START);
        event->addArg(temp_byte);

        local_log.concatf("Now profiling schedule %d\n", temp_byte);
        event->printDebug(&local_log);
      }
      break;

    case 'P':
      if (1 == temp_byte) {
        event = new ManuvrEvent(MANUVR_MSG_SCHED_DUMP_META);
        raiseEvent(event);   // Raise an event with a message. No need to clean it up.
      }
      else if (2 == temp_byte) {
        event = new ManuvrEvent(MANUVR_MSG_SCHED_DUMP_SCHEDULES);
        raiseEvent(event);   // Raise an event with a message. No need to clean it up.
      }
      else if (3 == temp_byte) {
        event = new ManuvrEvent(MANUVR_MSG_SCHED_PROFILER_DUMP);
        raiseEvent(event);   // Raise an event with a message. No need to clean it up.
      }
      else if (4 == temp_byte) {
        //ManuvrEvent *deferred = new ManuvrEvent(MANUVR_MSG_SCHED_DUMP_META);
        //event = new ManuvrEvent(MANUVR_MSG_SCHED_DEFERRED_EVENT);
        //event->addArg(deferred);
        //EventManager::raiseEvent(event);   // Raise an event with a message. No need to clean it up.
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

