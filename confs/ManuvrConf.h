/*
File:   FirmwareDefs.h
Author: J. Ian Lindsay
Date:   2015.06.01


This is one of the files that the application author is required to provide.
This is where definition of (application or device)-specific parameters ought to go.
*/

#ifndef __FIRMWARE_DEFS_H
#define __FIRMWARE_DEFS_H

/*
* Particulars of this Manuvrable.
*/

//#define MANUVR_STORAGE
#define EVENT_MANAGER_PREALLOC_COUNT     8
#define SCHEDULER_MAX_SKIP_BEFORE_RESET  6
#define PLATFORM_RNG_CARRY_CAPACITY     32
//#define MANUVR_HAS_CRYPT_WRAPPER
//#define WITH_MBEDTLS
//#define WITH_BLIND_CRYPTO

#define MANUVR_SUPPORT_SERIAL

#define MANUVR_EVENT_PROFILER
#define MANUVR_DEBUG


#define FIRMWARE_NAME     "SpatiumGestu"

// This would be the version of the Manuvrable's firmware (this program).
#define VERSION_STRING    "0.0.1"

// Hardware is versioned. Manuvrables that are strictly-software should say -1 here.
#define HW_VERSION_STRING "2"


#endif
