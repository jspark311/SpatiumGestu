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

#include <ManuvrOS/Platform/Platform.h>
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
class ADP8866;
class INA219;
class MCP73833;



/*
* This is the actual class...
*/
class StaticHub : public EventReceiver {
  public:
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

    // Global system resource handles...
    EventManager event_manager;            // This is our asynchronous message queue. 
    Scheduler __scheduler;

    StringBuilder usb_rx_buffer;
    StringBuilder last_user_input;

    // Scheduler PIDs that will be heavilly used...
    uint32_t pid_log_moderator;

    MCP73833* mcp73833;
    MGC3130*  mgc3130;
    INA219*   ina219;
    ADP8866*  adp8866;
    
    void procDirectDebugInstruction(StringBuilder*);
};

#ifdef __cplusplus
}
#endif 

#endif
