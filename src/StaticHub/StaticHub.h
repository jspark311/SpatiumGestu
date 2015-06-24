/*
File:   StaticHub.h
Author: J. Ian Lindsay
Date:   2014.07.01


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


This is the Spatium Gestu version of StaticHub.

*/


#ifndef __STATIC_HUB_H__
#define __STATIC_HUB_H__

#include "FirmwareDefs.h"

  // System-level includes.
  #include <inttypes.h>
  #include <stdlib.h>
  #include <stdarg.h>
  #include <string.h>
  #include <stdio.h>
  #include <stdint.h>

  #define LOG_EMERG   0    /* system is unusable */
  #define LOG_ALERT   1    /* action must be taken immediately */
  #define LOG_CRIT    2    /* critical conditions */
  #define LOG_ERR     3    /* error conditions */
  #define LOG_WARNING 4    /* warning conditions */
  #define LOG_NOTICE  5    /* normal but significant condition */
  #define LOG_INFO    6    /* informational */
  #define LOG_DEBUG   7    /* debug-level messages */

  #include <ManuvrOS/Scheduler.h>
  #include <ManuvrOS/EventManager.h>
  #include <StringBuilder/StringBuilder.h>

#ifdef ARDUINO
  #include <Arduino.h>
#endif

#ifdef __cplusplus
 extern "C" {
#endif 

// Forward-declare some things we are going to support....
class MGC3130;

/*
* These are just lables. We don't really ever care about the *actual* integers being defined here. Only
*   their consistency.
*/
#define MANUVR_RTC_STARTUP_UNINITED       0x00000000
#define MANUVR_RTC_STARTUP_UNKNOWN        0x23196400
#define MANUVR_RTC_OSC_FAILURE            0x23196401
#define MANUVR_RTC_STARTUP_GOOD_UNSET     0x23196402
#define MANUVR_RTC_STARTUP_GOOD_SET       0x23196403


/*
* These are constants where we care about the number.
*/
#define STATICHUB_RNG_CARRY_CAPACITY           10     // How many random numbers should StaticHub cache?





/*
* This stuff is not long for this world. It is only being used to test the efficiency
*   of these functions on different architectures.
*/
#if defined(__MK20DX256__) | defined(__MK20DX128__)
// computes limit((val >> rshift), 2**bits)
static inline int32_t signed_saturate_rshift(int32_t val, int bits, int rshift) __attribute__((always_inline, unused));
static inline int32_t signed_saturate_rshift(int32_t val, int bits, int rshift)
{
	int32_t out;
	asm volatile("ssat %0, %1, %2, asr %3" : "=r" (out) : "I" (bits), "r" (val), "I" (rshift));
	return out;
}

// computes ((a[31:0] * b[15:0]) >> 16)
static inline int32_t signed_multiply_32x16b(int32_t a, uint32_t b) __attribute__((always_inline, unused));
static inline int32_t signed_multiply_32x16b(int32_t a, uint32_t b)
{
	int32_t out;
	asm volatile("smulwb %0, %1, %2" : "=r" (out) : "r" (a), "r" (b));
	return out;
}

// computes ((a[31:0] * b[31:16]) >> 16)
static inline int32_t signed_multiply_32x16t(int32_t a, uint32_t b) __attribute__((always_inline, unused));
static inline int32_t signed_multiply_32x16t(int32_t a, uint32_t b)
{
	int32_t out;
	asm volatile("smulwt %0, %1, %2" : "=r" (out) : "r" (a), "r" (b));
	return out;
}

// computes (((int64_t)a[31:0] * (int64_t)b[31:0]) >> 32)
static inline int32_t multiply_32x32_rshift32(int32_t a, int32_t b) __attribute__((always_inline, unused));
static inline int32_t multiply_32x32_rshift32(int32_t a, int32_t b)
{
	int32_t out;
	asm volatile("smmul %0, %1, %2" : "=r" (out) : "r" (a), "r" (b));
	return out;
}

// computes (((int64_t)a[31:0] * (int64_t)b[31:0] + 0x8000000) >> 32)
static inline int32_t multiply_32x32_rshift32_rounded(int32_t a, int32_t b) __attribute__((always_inline, unused));
static inline int32_t multiply_32x32_rshift32_rounded(int32_t a, int32_t b)
{
	int32_t out;
	asm volatile("smmulr %0, %1, %2" : "=r" (out) : "r" (a), "r" (b));
	return out;
}

// computes sum + (((int64_t)a[31:0] * (int64_t)b[31:0] + 0x8000000) >> 32)
static inline int32_t multiply_accumulate_32x32_rshift32_rounded(int32_t sum, int32_t a, int32_t b) __attribute__((always_inline, unused));
static inline int32_t multiply_accumulate_32x32_rshift32_rounded(int32_t sum, int32_t a, int32_t b)
{
	int32_t out;
	asm volatile("smmlar %0, %2, %3, %1" : "=r" (out) : "r" (sum), "r" (a), "r" (b));
	return out;
}

// computes sum - (((int64_t)a[31:0] * (int64_t)b[31:0] + 0x8000000) >> 32)
static inline int32_t multiply_subtract_32x32_rshift32_rounded(int32_t sum, int32_t a, int32_t b) __attribute__((always_inline, unused));
static inline int32_t multiply_subtract_32x32_rshift32_rounded(int32_t sum, int32_t a, int32_t b)
{
	int32_t out;
	asm volatile("smmlsr %0, %2, %3, %1" : "=r" (out) : "r" (sum), "r" (a), "r" (b));
	return out;
}


// computes (a[31:16] | (b[31:16] >> 16))
static inline uint32_t pack_16t_16t(int32_t a, int32_t b) __attribute__((always_inline, unused));
static inline uint32_t pack_16t_16t(int32_t a, int32_t b)
{
	int32_t out;
	asm volatile("pkhtb %0, %1, %2, asr #16" : "=r" (out) : "r" (a), "r" (b));
	return out;
}

// computes (a[31:16] | b[15:0])
static inline uint32_t pack_16t_16b(int32_t a, int32_t b) __attribute__((always_inline, unused));
static inline uint32_t pack_16t_16b(int32_t a, int32_t b)
{
	int32_t out;
	asm volatile("pkhtb %0, %1, %2" : "=r" (out) : "r" (a), "r" (b));
	return out;
}

// computes ((a[15:0] << 16) | b[15:0])
static inline uint32_t pack_16b_16b(int32_t a, int32_t b) __attribute__((always_inline, unused));
static inline uint32_t pack_16b_16b(int32_t a, int32_t b)
{
	int32_t out;
	asm volatile("pkhbt %0, %1, %2, lsl #16" : "=r" (out) : "r" (b), "r" (a));
	return out;
}

// computes ((a[15:0] << 16) | b[15:0])
static inline uint32_t pack_16x16(int32_t a, int32_t b) __attribute__((always_inline, unused));
static inline uint32_t pack_16x16(int32_t a, int32_t b)
{
	int32_t out;
	asm volatile("pkhbt %0, %1, %2, lsl #16" : "=r" (out) : "r" (b), "r" (a));
	return out;
}

// computes (((a[31:16] + b[31:16]) << 16) | (a[15:0 + b[15:0]))
static inline uint32_t signed_add_16_and_16(uint32_t a, uint32_t b) __attribute__((always_inline, unused));
static inline uint32_t signed_add_16_and_16(uint32_t a, uint32_t b)
{
	int32_t out;
	asm volatile("qadd16 %0, %1, %2" : "=r" (out) : "r" (a), "r" (b));
	return out;
}

// computes (sum + ((a[31:0] * b[15:0]) >> 16))
static inline int32_t signed_multiply_accumulate_32x16b(int32_t sum, int32_t a, uint32_t b) __attribute__((always_inline, unused));
static inline int32_t signed_multiply_accumulate_32x16b(int32_t sum, int32_t a, uint32_t b)
{
	int32_t out;
	asm volatile("smlawb %0, %2, %3, %1" : "=r" (out) : "r" (sum), "r" (a), "r" (b));
	return out;
}

// computes (sum + ((a[31:0] * b[31:16]) >> 16))
static inline int32_t signed_multiply_accumulate_32x16t(int32_t sum, int32_t a, uint32_t b) __attribute__((always_inline, unused));
static inline int32_t signed_multiply_accumulate_32x16t(int32_t sum, int32_t a, uint32_t b)
{
	int32_t out;
	asm volatile("smlawt %0, %2, %3, %1" : "=r" (out) : "r" (sum), "r" (a), "r" (b));
	return out;
}

// computes logical and, forces compiler to allocate register and use single cycle instruction
static inline uint32_t logical_and(uint32_t a, uint32_t b) __attribute__((always_inline, unused));
static inline uint32_t logical_and(uint32_t a, uint32_t b)
{
	asm volatile("and %0, %1" : "+r" (a) : "r" (b));
	return a;
}

// computes ((a[15:0] * b[15:0]) + (a[31:16] * b[31:16]))
static inline int32_t multiply_16tx16t_add_16bx16b(uint32_t a, uint32_t b) __attribute__((always_inline, unused));
static inline int32_t multiply_16tx16t_add_16bx16b(uint32_t a, uint32_t b)
{
	int32_t out;
	asm volatile("smuad %0, %1, %2" : "=r" (out) : "r" (a), "r" (b));
	return out;
}

// computes ((a[15:0] * b[31:16]) + (a[31:16] * b[15:0]))
static inline int32_t multiply_16tx16b_add_16bx16t(uint32_t a, uint32_t b) __attribute__((always_inline, unused));
static inline int32_t multiply_16tx16b_add_16bx16t(uint32_t a, uint32_t b)
{
	int32_t out;
	asm volatile("smuadx %0, %1, %2" : "=r" (out) : "r" (a), "r" (b));
	return out;
}

// computes ((a[15:0] * b[15:0])
static inline int32_t multiply_16bx16b(uint32_t a, uint32_t b) __attribute__((always_inline, unused));
static inline int32_t multiply_16bx16b(uint32_t a, uint32_t b)
{
	int32_t out;
	asm volatile("smulbb %0, %1, %2" : "=r" (out) : "r" (a), "r" (b));
	return out;
}

// computes ((a[15:0] * b[31:16])
static inline int32_t multiply_16bx16t(uint32_t a, uint32_t b) __attribute__((always_inline, unused));
static inline int32_t multiply_16bx16t(uint32_t a, uint32_t b)
{
	int32_t out;
	asm volatile("smulbt %0, %1, %2" : "=r" (out) : "r" (a), "r" (b));
	return out;
}

// computes ((a[31:16] * b[15:0])
static inline int32_t multiply_16tx16b(uint32_t a, uint32_t b) __attribute__((always_inline, unused));
static inline int32_t multiply_16tx16b(uint32_t a, uint32_t b)
{
	int32_t out;
	asm volatile("smultb %0, %1, %2" : "=r" (out) : "r" (a), "r" (b));
	return out;
}

// computes ((a[31:16] * b[31:16])
static inline int32_t multiply_16tx16t(uint32_t a, uint32_t b) __attribute__((always_inline, unused));
static inline int32_t multiply_16tx16t(uint32_t a, uint32_t b)
{
	int32_t out;
	asm volatile("smultt %0, %1, %2" : "=r" (out) : "r" (a), "r" (b));
	return out;
}

// computes (a - b), result saturated to 32 bit integer range
static inline int32_t substract_32_saturate(uint32_t a, uint32_t b) __attribute__((always_inline, unused));
static inline int32_t substract_32_saturate(uint32_t a, uint32_t b)
{
	int32_t out;
	asm volatile("qsub %0, %1, %2" : "=r" (out) : "r" (a), "r" (b));
	return out;
}

#elif defined(STM32F4XX)

#elif defined(__PIC32MZ__)


#elif defined(__PIC32MX__)


#endif








/*
* This is the actual class...
*/
class StaticHub : public EventReceiver {
  public:
    volatile static uint32_t millis_since_reset;
    volatile static uint8_t  watchdog_mark;

    static StringBuilder log_buffer;
    static bool mute_logger; 

    StaticHub(void);
    static StaticHub* getInstance(void);
    int8_t bootstrap(void);
    
    // These are functions that should be reachable from everywhere in the application.
    volatile static void log(const char *fxn_name, int severity, const char *str, ...);  // Pass-through to the logger class, whatever that happens to be.
    volatile static void log(int severity, const char *str);                             // Pass-through to the logger class, whatever that happens to be.
    volatile static void log(const char *str);                                           // Pass-through to the logger class, whatever that happens to be.
    volatile static void log(char *str);                                           // Pass-through to the logger class, whatever that happens to be.
    volatile static void log(StringBuilder *str);
    
    /*
    * Nice utility functions.
    */
    static uint32_t randomInt(void);                                // Fetches one of the stored randoms and blocks until one is available.
    static volatile bool provide_random_int(uint32_t);              // Provides a new random to StaticHub from the RNG ISR.
    static volatile uint32_t getStackPointer(void);                 // Returns the value of the stack pointer and prints some data.
    
        
    bool setTimeAndDate(char*);   // Takes a string of the form given by RFC-2822: "Mon, 15 Aug 2005 15:52:01 +0000"   https://www.ietf.org/rfc/rfc2822.txt
    uint32_t currentTimestamp(void);         // Returns an integer representing the current datetime.
    void currentTimestamp(StringBuilder*);   // Same, but writes a string representation to the argument.
    void currentDateTime(StringBuilder*);    // Writes a human-readable datetime to the argument.
    

    // Call this to accumulate characters from the USB layer into a buffer.
    // Pass terminal=true to cause StaticHub to proc an accumulated command from the host PC.
    void feedUSBBuffer(uint8_t *buf, int len, bool terminal);


    /*
    * These are global resource accessor functions. They are called once from each class that
    *   requires them. That class can technically call this accessor for each use, but this should
    *   be discouraged, as the instances fetched by these functions should never change.
    */
    // Services...
    EventManager* fetchEventManager(void);
    Scheduler* fetchScheduler(void);

    MGC3130*    fetchMGC3130(void);
    
    // Volatile statics that serve as ISRs...
    volatile static void advanceScheduler(void);
    

    /* Overrides from EventReceiver */
    const char* getReceiverName();
    void printDebug(StringBuilder*);
    int8_t notify(ManuvrEvent*);
    int8_t callback_proc(ManuvrEvent *);


    void disableLogCallback();


  protected:
    int8_t bootComplete();       // Called as a result of bootstrap completed being raised.

    
  private:
    volatile static StaticHub* INSTANCE;
    volatile static uint32_t next_random_int[STATICHUB_RNG_CARRY_CAPACITY];  // Stores the last 10 random numbers.

    // Global system resource handles...
    EventManager event_manager;            // This is our asynchronous message queue. 
    Scheduler __scheduler;

	StringBuilder usb_rx_buffer;
    StringBuilder last_user_input;

    // Scheduler PIDs that will be heavilly used...
    uint32_t pid_log_moderator;
    uint32_t pid_profiler_report;
    uint32_t pid_prog_run_delay;

    uint32_t rtc_startup_state;
    
    MGC3130* mgc3130;

    // These fxns do string conversion for integer type-codes, and are only useful for logging.
    const char* getRTCStateString(uint32_t code);
    
    void print_type_sizes(void);

    // These functions handle various stages of bootstrap...
    void gpioSetup(void) volatile;        // We call this once on bootstrap. Sets up GPIO not covered by other classes.
    void nvicConf(void) volatile;         // We call this once on bootstrap. Sets up IRQs not covered by other classes.
    void init_RNG(void) volatile;         // Fire up the random number generator.
    void initRTC(void) volatile;          // We call this once on bootstrap. Sets up the RTC.
    void initSchedules(void);    // We call this once on bootstrap. Sets up all schedules.
    
    void procDirectDebugInstruction(StringBuilder*);
    
    void off_class_interrupts(bool enable);
    void maskable_interrupts(bool enable);
};

#ifdef __cplusplus
}
#endif 

#endif
