###########################################################################
# Makefile for Spatium Gestu
# Author: J. Ian Lindsay
# Date:   2015.06.20
#
#
###########################################################################


# Environmental awareness...
###########################################################################
SHELL          = /bin/sh
WHO_I_AM       = $(shell whoami)
WHERE_I_AM     = $(shell pwd)
HOME_DIRECTORY = /home/$(WHO_I_AM)
MPEIDE_PATH    = $(HOME_DIRECTORY)/mpide-0023-linux64-20140821
PLATFORM_PATH  = $(MPEIDE_PATH)/hardware/pic32/


# Variables for the firmware compilation...
###########################################################################
FIRMWARE_NAME      = spatiumgenstu

TOOLCHAIN          = $(MPEIDE_PATH)/hardware/pic32/compiler/pic32-tools/bin
C_CROSS            = $(TOOLCHAIN)/pic32-gcc
CPP_CROSS          = $(TOOLCHAIN)/pic32-g++
AR_CROSS           = $(TOOLCHAIN)/pic32-ar
OBJCOPY            = $(TOOLCHAIN)/pic32-objcopy
SZ_CROSS           = $(TOOLCHAIN)/pic32-size
BIN2HEX_CROSS      = $(TOOLCHAIN)/pic32-bin2hex
TEENSY_LOADER_PATH = $(MPEIDE_PATH)/hardware/tools/teensy_loader_cli

OUTPUT_PATH        = build


MCU                = 32MX250F128D
CPU_SPEED          = 48000000L
OPTIMIZATION       = -Os
C_STANDARD         = gnu99
CPP_STANDARD       = gnu++11
FORMAT             = ihex
BOARD_DEF          = _BOARD_FUBARINO_MINI_


###########################################################################
# Source files, includes, and linker directives...
###########################################################################
INCLUDES     = -iquote. -iquotesrc/ 
INCLUDES    += -I./ -I$(PLATFORM_PATH)/libraries -I$(MPEIDE_PATH)/libraries
INCLUDES    += -I$(PLATFORM_PATH)/cores/pic32
INCLUDES    += -I./
INCLUDES    += -I$(PLATFORM_PATH)/variants/Fubarino_Mini
INCLUDES    += -I$(PLATFORM_PATH)/libraries/Wire/utility/

LD_FILE     = $(PLATFORM_PATH)/cores/pic32/chipKIT-application-32MX250F128.ld

# Libraries to link
LIBS = -lm -lstdc++ -larm_cortexM4l_math -lc

# Wrap the include paths into the flags...
CFLAGS = $(INCLUDES)
CFLAGS += $(OPTIMIZATION) -Wall

CFLAGS += -DF_CPU=$(CPU_SPEED)
CFLAGS += -D$(BOARD_DEF) 
CFLAGS += -mprocessor=$(MCU) 

CFLAGS += -ffunction-sections -fdata-sections
CFLAGS += -mno-smart-io -G1024 -g -mdebugger -Wcast-align -fno-short-double

CFLAGS += -DMPIDEVER=16777998 -DMPIDE=23 -DARDUINO=23


CPP_FLAGS = -fno-exceptions -D_USE_USB_FOR_SERIAL_



###########################################################################
# Are we on a 64-bit system? If so, we'll need to specify
#   that we want a 32-bit build...
# Thanks, estabroo...
# http://www.linuxquestions.org/questions/programming-9/how-can-make-makefile-detect-64-bit-os-679513/
###########################################################################
LBITS = $(shell getconf LONG_BIT)
ifeq ($(LBITS),64)
  TARGET_WIDTH = -m32
else
  TARGET_WIDTH =
endif


CFLAGS += $(CPP_FLAGS)


###########################################################################
# Source file definitions...
###########################################################################

MANUVROS_SRCS = src/StringBuilder/*.cpp src/ManuvrOS/*.cpp src/ManuvrOS/XenoSession/*.cpp src/ManuvrOS/ManuvrMsg/*.cpp src/ManuvrOS/Transports/*.cpp
I2C_DRIVERS   = src/ManuvrOS/Drivers/i2c-adapter/*.cpp

SPATIUM_GESTU_DRIVERS  =  $(I2C_DRIVERS) src/ManuvrOS/Drivers/MGC3130/*.cpp 

SRCS   = src/SpatiumGestu.cpp src/StaticHub/*.cpp $(MANUVROS_SRCS) $(SPATIUM_GESTU_DRIVERS)



###########################################################################
# Rules for building the firmware follow...
###########################################################################

.PHONY: lib $(OUTPUT_PATH)/$(FIRMWARE_NAME).elf


all: lib $(OUTPUT_PATH)/$(FIRMWARE_NAME).elf
	$(SZ_CROSS) $(OUTPUT_PATH)/$(FIRMWARE_NAME).elf


lib:
	mkdir -p $(OUTPUT_PATH)
	$(CPP_CROSS) -O2  -g1  -c  -Wa,--gdwarf-2 $(CFLAGS) -DMPIDEVER=16777998  -DMPIDE=23   -D_USE_USB_FOR_SERIAL_   -I/home/ian/mpide-0023-linux64-20140821/hardware/pic32/cores/pic32   -I/home/ian/mpide-0023-linux64-20140821/hardware/pic32/variants/Fubarino_Mini    $(PLATFORM_PATH)/cores/pic32/pic32_software_reset.S  -o  $(OUTPUT_PATH)/pic32_software_reset.S.o
	$(CPP_CROSS) -O2  -g1  -c  -Wa,--gdwarf-2 $(CFLAGS) -DMPIDEVER=16777998  -DMPIDE=23   -D_USE_USB_FOR_SERIAL_   -I/home/ian/mpide-0023-linux64-20140821/hardware/pic32/cores/pic32   -I/home/ian/mpide-0023-linux64-20140821/hardware/pic32/variants/Fubarino_Mini    $(PLATFORM_PATH)/cores/pic32/cpp-startup.S  -o  $(OUTPUT_PATH)/cpp-startup.S.o
	$(CPP_CROSS) -O2  -g1  -c  -Wa,--gdwarf-2 $(CFLAGS) -DMPIDEVER=16777998  -DMPIDE=23   -D_USE_USB_FOR_SERIAL_   -I/home/ian/mpide-0023-linux64-20140821/hardware/pic32/cores/pic32   -I/home/ian/mpide-0023-linux64-20140821/hardware/pic32/variants/Fubarino_Mini    $(PLATFORM_PATH)/cores/pic32/vector_table.S  -o $(OUTPUT_PATH)/vector_table.S.o
	$(CPP_CROSS) -O2  -g1  -c  -Wa,--gdwarf-2 $(CFLAGS) -DMPIDEVER=16777998  -DMPIDE=23   -D_USE_USB_FOR_SERIAL_   -I/home/ian/mpide-0023-linux64-20140821/hardware/pic32/cores/pic32   -I/home/ian/mpide-0023-linux64-20140821/hardware/pic32/variants/Fubarino_Mini    $(PLATFORM_PATH)/cores/pic32/crti.S  -o  $(OUTPUT_PATH)/crti.S.o
	$(CPP_CROSS) -O2  -g1  -c  -Wa,--gdwarf-2 $(CFLAGS) -DMPIDEVER=16777998  -DMPIDE=23   -D_USE_USB_FOR_SERIAL_   -I/home/ian/mpide-0023-linux64-20140821/hardware/pic32/cores/pic32   -I/home/ian/mpide-0023-linux64-20140821/hardware/pic32/variants/Fubarino_Mini    $(PLATFORM_PATH)/cores/pic32/crtn.S  -o  $(OUTPUT_PATH)/crtn.S.o
	
	
	$(CPP_CROSS) -O2 -c $(CFLAGS) $(PLATFORM_PATH)/libraries/Wire/Wire.cpp  -o         $(OUTPUT_PATH)/Wire.cpp.o
	$(CPP_CROSS) -O2 -c $(CFLAGS) $(PLATFORM_PATH)/cores/pic32/main.cpp  -o            $(OUTPUT_PATH)/main.cpp.o
	$(CPP_CROSS) -O2 -c $(CFLAGS) $(PLATFORM_PATH)/cores/pic32/WString.cpp  -o         $(OUTPUT_PATH)/WString.cpp.o
	$(CPP_CROSS) -O2 -c $(CFLAGS) $(PLATFORM_PATH)/cores/pic32/Tone.cpp  -o            $(OUTPUT_PATH)/Tone.cpp.o
	$(CPP_CROSS) -O2 -c $(CFLAGS) $(PLATFORM_PATH)/cores/pic32/Print.cpp  -o           $(OUTPUT_PATH)/Print.cpp.o
	$(CPP_CROSS) -O2 -c $(CFLAGS) $(PLATFORM_PATH)/cores/pic32/HardwareSerial.cpp  -o  $(OUTPUT_PATH)/HardwareSerial.cpp.o
	$(CPP_CROSS) -O2 -c $(CFLAGS) $(PLATFORM_PATH)/cores/pic32/WMath.cpp  -o           $(OUTPUT_PATH)/WMath.cpp.o

	$(C_CROSS)   -O2 -c $(CFLAGS) $(PLATFORM_PATH)/cores/pic32/WMath.cpp  -o           $(OUTPUT_PATH)/WMath.cpp.o

	$(C_CROSS)   -O2 -c $(CFLAGS) $(PLATFORM_PATH)/cores/pic32/HardwareSerial_cdcacm.c  -o  $(OUTPUT_PATH)/HardwareSerial_cdcacm.c.o
	$(C_CROSS)   -O2 -c $(CFLAGS) $(PLATFORM_PATH)/cores/pic32/wiring_shift.c        -o  $(OUTPUT_PATH)/wiring_shift.c.o
	$(C_CROSS)   -O2 -c $(CFLAGS) $(PLATFORM_PATH)/cores/pic32/HardwareSerial_usb.c  -o  $(OUTPUT_PATH)/HardwareSerial_usb.c.o
	$(C_CROSS)   -O2 -c $(CFLAGS) $(PLATFORM_PATH)/cores/pic32/wiring_pulse.c        -o  $(OUTPUT_PATH)/wiring_pulse.c.o
	$(C_CROSS)   -O2 -c $(CFLAGS) $(PLATFORM_PATH)/cores/pic32/wiring_digital.c      -o  $(OUTPUT_PATH)/wiring_digital.c.o
	$(C_CROSS)   -O2 -c $(CFLAGS) $(PLATFORM_PATH)/cores/pic32/WInterrupts.c         -o  $(OUTPUT_PATH)/WInterrupts.c.o
	$(C_CROSS)   -O2 -c $(CFLAGS) $(PLATFORM_PATH)/cores/pic32/wiring.c              -o  $(OUTPUT_PATH)/wiring.c.o
	$(C_CROSS)   -O2 -c $(CFLAGS) $(PLATFORM_PATH)/cores/pic32/pins_arduino.c        -o  $(OUTPUT_PATH)/pins_arduino.c.o
	$(C_CROSS)   -O2 -c $(CFLAGS) $(PLATFORM_PATH)/cores/pic32/task_manager.c        -o  $(OUTPUT_PATH)/task_manager.c.o
	$(C_CROSS)   -O2 -c $(CFLAGS) $(PLATFORM_PATH)/cores/pic32/exceptions.c          -o  $(OUTPUT_PATH)/exceptions.c.o
	$(C_CROSS)   -O2 -c $(CFLAGS) $(PLATFORM_PATH)/cores/pic32/WSystem.c             -o  $(OUTPUT_PATH)/WSystem.c.o
	$(C_CROSS)   -O2 -c $(CFLAGS) $(PLATFORM_PATH)/cores/pic32/wiring_analog.c       -o  $(OUTPUT_PATH)/wiring_analog.c.o


$(OUTPUT_PATH)/$(FIRMWARE_NAME).elf:
	$(shell mkdir $(OUTPUT_PATH))

	$(CPP_CROSS) -c $(CFLAGS) $(SRCS)
	$(AR_CROSS) rcs $(OUTPUT_PATH)/core.a $(OUTPUT_PATH)/*.o *.o 
	$(CPP_CROSS) $(OPTIMIZATION) -Wl,--gc-sections -mdebugger -mno-peripheral-libs -nostartfiles -o $(OUTPUT_PATH)/$(FIRMWARE_NAME).elf c -L$(OUTPUT_PATH) $(LIBS) -T$(LD_FILE) -T/home/ian/mpide-0023-linux64-20140821/hardware/pic32/cores/pic32/chipKIT-application-COMMON.ld

	@echo
	@echo $(MSG_FLASH) $@
	$(OBJCOPY) -O $(FORMAT) -j .eeprom --set-section-flags=.eeprom=alloc,load --no-change-warnings --change-section-lma .eeprom=0 $(OUTPUT_PATH)/$(FIRMWARE_NAME).elf $(OUTPUT_PATH)/$(FIRMWARE_NAME).eep 
	$(BIN2HEX_CROSS) -a $(OUTPUT_PATH)/$(FIRMWARE_NAME).elf



program: $(OUTPUT_PATH)/$(FIRMWARE_NAME).elf
	$(TEENSY_LOADER_PATH) -mmcu=mk20dx128 -w -v $(OUTPUT_PATH)/$(FIRMWARE_NAME).hex


fullclean:
	rm -f *.d *.o *.su *~ testbench
	rm -rf doc/doxygen/*
	rm -rf $(OUTPUT_PATH)

clean:
	rm -f *.d *.o *.su *~ testbench
	rm -rf $(OUTPUT_PATH)

doc:
	mkdir -p doc/doxygen/
	doxygen Doxyfile

stats:
	find ./src -type f \( -name \*.cpp -o -name \*.h \) -exec wc -l {} +

