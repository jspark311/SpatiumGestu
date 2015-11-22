/*
File:   ADP8866.cpp
Author: J. Ian Lindsay
Date:   2014.05.27


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
#include "ADP8866.h"


/*
* Constructor. Takes i2c address as argument.
*/
ADP8866::ADP8866(uint8_t reset_pin, uint8_t irq_pin, uint8_t addr) : I2CDeviceWithRegisters() {
  __class_initializer();
  _dev_addr = addr;
  
  init_complete = false;
//  defineRegister(ADP8866_BANK_A_CURRENT,     (uint8_t) 0x90, true,  false, true);
//  defineRegister(ADP8866_BANK_B_CURRENT,     (uint8_t) 0x90, true,  false, true);
//  defineRegister(ADP8866_BANK_C_CURRENT,     (uint8_t) 0x90, true,  false, true);
//  defineRegister(ADP8866_CHANNEL_ENABLE,     (uint8_t) 0x3F, true, false, true);
//  defineRegister(ADP8866_GLOBAL_PWM_DIM,     (uint8_t) 0xA0, true, false, true);
//  defineRegister(ADP8866_BANK_A_PWM_DUTY,    (uint8_t) 0xDC, true,  false, true);
//  defineRegister(ADP8866_BANK_B_PWM_DUTY,    (uint8_t) 0xDC, true,  false, true);
//  defineRegister(ADP8866_BANK_C_PWM_DUTY,    (uint8_t) 0xDC, true,  false, true);
//  defineRegister(ADP8866_TEST_MODE,          (uint8_t) 0x00, false, false, false);
//  defineRegister(ADP8866_LED_SHORT_GND,      (uint8_t) 0x00, false, false, false);
//  defineRegister(ADP8866_LED_FAULT,          (uint8_t) 0x00, false, false, false);
//  defineRegister(ADP8866_CONFIG_REG,         (uint8_t) 0x00, false, false, true);
//  defineRegister(ADP8866_SOFTWARE_RESET,     (uint8_t) 0x00, false, false, true);
//  defineRegister(ADP8866_TEMPERATURE_OFFSET, (uint8_t) 0x00, false, false, false);
//  defineRegister(ADP8866_LED_SHUTDOWN_TEMP,  (uint8_t) 0x00, false, false, false);
//  defineRegister(ADP8866_TABLE_ENABLE_BP,    (uint8_t) 0x00, false, false, false);
//  defineRegister(ADP8866_SI_DIODE_DV_DT,     (uint8_t) 0x00, false, false, false);
}

/*
* Destructor.
*/
ADP8866::~ADP8866(void) {
}


int8_t ADP8866::init() {
  if (syncRegisters() == I2C_ERR_CODE_NO_ERROR) {
    //writeIndirect(ADP8866_BANK_A_CURRENT,  0x90, true);
    //writeIndirect(ADP8866_BANK_B_CURRENT,  0x90, true);
    //writeIndirect(ADP8866_BANK_C_CURRENT,  0x90, true);
    //writeIndirect(ADP8866_CHANNEL_ENABLE,  0x3F, true);
    //writeIndirect(ADP8866_GLOBAL_PWM_DIM,  0xB4, true);
    //writeIndirect(ADP8866_BANK_A_PWM_DUTY, 0xDC, true);
    //writeIndirect(ADP8866_BANK_B_PWM_DUTY, 0xDC, true);
    //writeIndirect(ADP8866_BANK_C_PWM_DUTY, 0xDC);
    //writeIndirect(ADP8866_CONFIG_REG, 0x04);
    init_complete = true;
  }
  else {
    StaticHub::log("ADP8866::init():\tFailed to sync registers. Init fails.\n");
    init_complete = false;
    return -1;
  }
  return 0;
}


/****************************************************************************************************
* These are overrides from I2CDeviceWithRegisters.                                                  *
****************************************************************************************************/

void ADP8866::operationCompleteCallback(I2CQueuedOperation* completed) {
  I2CDeviceWithRegisters::operationCompleteCallback(completed);
  
  int i = 0;
  DeviceRegister *temp_reg = reg_defs.get(i++);
  while (temp_reg != NULL) {
    switch (temp_reg->addr) {
      default:
        temp_reg->unread = false;
        break;
    }
    temp_reg = reg_defs.get(i++);
  }
}



/**
* Debug support function.
*
* @return a pointer to a string constant.
*/
const char* ADP8866::getReceiverName() {  return "ADP8866";  }


/*
* Dump this item to the dev log.
*/
void ADP8866::printDebug(StringBuilder* temp) {
  if (NULL == temp) return;
  EventReceiver::printDebug(temp);
  I2CDeviceWithRegisters::printDebug(temp);
  temp->concatf("\tinit_complete:      %s\n", init_complete ? "yes" :"no");
  temp->concatf("\tpower_mode:        %d\n", power_mode);
}



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
/**
* There is a NULL-check performed upstream for the scheduler member. So no need 
*   to do it again here.
*
* @return 0 on no action, 1 on action, -1 on failure.
*/
int8_t ADP8866::bootComplete() {
  EventReceiver::bootComplete();

  //writeDirtyRegisters();  // If i2c is broken, this will hang the boot process...
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
int8_t ADP8866::callback_proc(ManuvrEvent *event) {
  /* Setup the default return code. If the event was marked as mem_managed, we return a DROP code.
     Otherwise, we will return a REAP code. Downstream of this assignment, we might choose differently. */ 
  int8_t return_value = event->eventManagerShouldReap() ? EVENT_CALLBACK_RETURN_REAP : EVENT_CALLBACK_RETURN_DROP;
  
  /* Some class-specific set of conditionals below this line. */
  switch (event->event_code) {
    default:
      break;
  }
  
  return return_value;
}



int8_t ADP8866::notify(ManuvrEvent *active_event) {
  int8_t return_value = 0;
  
  switch (active_event->event_code) {
    case MANUVR_MSG_SYS_POWER_MODE:
      break;
      
    default:
      return_value += EventReceiver::notify(active_event);
      break;
  }
      
  if (local_log.length() > 0) {    StaticHub::log(&local_log);  }
  return return_value;
}



void ADP8866::procDirectDebugInstruction(StringBuilder *input) {
  char* str = input->position(0);
  ManuvrEvent *event = NULL;  // Pitching events is a common thing in this fxn...

  uint8_t temp_byte = 0;
  if (*(str) != 0) {
    temp_byte = atoi((char*) str+1);
  }

  switch (*(str)) {
    default:
      EventReceiver::procDirectDebugInstruction(input);
      break;
  }
  
  if (local_log.length() > 0) {    StaticHub::log(&local_log);  }
}



/****************************************************************************************************
* Functions specific to this class....                                                              *
****************************************************************************************************/
/*
* This is a function that holds high-level macros for LED behavior that is generic.
* Example:
*   Some classes might wish to take direct control over the LEDs to do something fancy.
*   Other classes might not care about micromanaging LED behavior, but would like to use
*     them as indirectly as possible. This is the type of class this function is meant for. 
*/
void ADP8866::set_led_mode(uint8_t num) {
}


void ADP8866::quell_all_timers() {
}


/*
* Set the global brightness for LEDs managed by this chip.
* Stores the previous value.
*/
void ADP8866::set_brightness(uint8_t nu_brightness) {
  //writeIndirect(ADP8866_GLOBAL_PWM_DIM, nu_brightness);
}

/*
* Set the global brightness for LEDs managed by this chip.
* Exchanges the current value and the previously-stored value.
*/
void ADP8866::toggle_brightness(void) {
  //writeIndirect(ADP8866_GLOBAL_PWM_DIM, stored_dimmer_val);
}

/*
* Enable or disable a specific LED. If something needs to be written to the
*   chip, will do that as well.
*/
void ADP8866::enable_channel(uint8_t chan, bool en) {
  //writeIndirect(ADP8866_CHANNEL_ENABLE, new_val);
}

/*
* Returns the boolean answer to the question: Is the given channel enabled?
*/
bool ADP8866::channel_enabled(uint8_t chan) {
  //return (regValue(ADP8866_CHANNEL_ENABLE) & (1 << chan));
  return false;
}

/*
* Perform a software reset.
*/
void ADP8866::reset() {
  //writeIndirect(ADP8866_SOFTWARE_RESET, 0x80);
}

/*
* When a power mode broadcast is seen, this fxn will be called with the new
*   power profile identifier.
*/
void ADP8866::set_power_mode(uint8_t nu_power_mode) {
  power_mode = nu_power_mode;
  switch (power_mode) {
    case 0:
      //writeIndirect(ADP8866_BANK_A_CURRENT, 0x8E, true);
      //writeIndirect(ADP8866_BANK_B_CURRENT, 0x8E, true);
      //writeIndirect(ADP8866_BANK_C_CURRENT, 0x8E, true);
      //writeIndirect(ADP8866_SOFTWARE_RESET, 0x00);
      break;
    case 1:
      //writeIndirect(ADP8866_BANK_A_CURRENT, 0x50, true);
      //writeIndirect(ADP8866_BANK_B_CURRENT, 0x50, true);
      //writeIndirect(ADP8866_BANK_C_CURRENT, 0x50, true);
      //writeIndirect(ADP8866_SOFTWARE_RESET, 0x00);
      break;
    case 2:   // Enter standby.
      quell_all_timers();
      //writeIndirect(ADP8866_SOFTWARE_RESET, 0x40);
      break;
    default:
      break;
  }
  StaticHub::log("ADP8866 Power mode set. \n");
}




