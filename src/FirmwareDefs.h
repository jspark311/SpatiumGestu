/*
File:   FirmwareDefs.h
Author: J. Ian Lindsay
Date:   2015.06.01

This is one of the files that the application author is required to provide. This is where definition of
  (application or device)-specific event codes ought to go. We also define some fields that will be used
  during communication with other devices, so some things here are mandatory.

*/

#ifndef __FIRMWARE_DEFS_H
#define __FIRMWARE_DEFS_H


/*
* These are required fields.
*
* PROTOCOL_MTU is required for constraining communication length due to memory restrictions at
*   one-or-both sides. Since the protocol currently supports up to (2^24)-1 bytes in a single transaction,
*   a microcontroller would want to limit it's counterparty's use of precious RAM. PROTOCOL_MTU, therefore,
*   determines the effective maximum packet size for this device.
*/
#define PROTOCOL_MTU              2000                  // See MTU notes above....
#define VERSION_STRING            "0.1.3"               // We should be able to communicate version so broken behavior can be isolated.
#define HW_VERSION_STRING         "1"                   // First revision of the hardware.
#define IDENTITY_STRING           "Spatium Gestu"       // Might also be a hash....
//#define EXTENDED_DETAIL_STRING    "RasPiBuild"           // Optional. User-defined.
#define PROTOCOL_VERSION          0x00000001             // The protocol version we are using.



/* Codes that are specific to Spatium Gestu */

#if defined(__MK20DX256__) | defined(__MK20DX128__)
#else
inline void sei() {};
inline void cli() {};
#endif



#ifdef __cplusplus
extern "C" {
#endif

// Function prototypes
#ifndef TEST_BENCH
  volatile void jumpToBootloader(void);
  volatile void reboot(void);
#endif

#ifdef __cplusplus
}
#endif

#endif
