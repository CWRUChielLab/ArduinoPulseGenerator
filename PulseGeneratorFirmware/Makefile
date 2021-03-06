# the name of the main file and sources to be used
APPNAME=PulseGeneratorFirmware
SOURCES=$(APPNAME).o pulseStateMachine.o
TEST_SOURCES = pulseStateMachine_test.o

# default target
all: $(APPNAME).hex

# manual dependencies
pulseStateMachine.o pulseStateMachine_test.o : pulseStateMachine.h
PulseGenerator.o : pulseStateMachine.h

ARDUINO_SOURCES_DIR=/usr/share/arduino/hardware/arduino/cores/arduino
ARDUINO_VARIANT_DIR=/usr/share/arduino/hardware/arduino/variants/mega
ARDUINO_SPI_LIB_DIR=/usr/share/arduino/libraries/SPI

# port the arduino is connected to
# and CPU type as defined by gcc and AVR-DUDE
PORT=/dev/ttyACM0
GCC_MMCU=atmega2560
AVRDUDE_STK=stk500v2
AVRDUDE_MCU=atmega2560
AVRDUDE_BAUD=115200

# CPU Clock speed (cycles per second)
CLOCKSPEED=16000000

CC=avr-gcc
CXX=avr-g++

SHAREDFLAGS= -gstabs -Os \
		-funsigned-char -funsigned-bitfields -fpack-struct \
		-fshort-enums \
		-I$(ARDUINO_SOURCES_DIR) \
		-I$(ARDUINO_VARIANT_DIR) \
		-I$(ARDUINO_SPI_LIB_DIR) \
		-mmcu=$(GCC_MMCU) -DF_CPU=$(CLOCKSPEED)

CFLAGS=-std=gnu99 -Wstrict-prototypes $(SHAREDFLAGS)
CXXFLAGS=$(SHAREDFLAGS)
NOISYFLAGS=-Wall -Wextra -Werror
#NOISYFLAGS=-Wall -Wextra -pedantic
#NOISYFLAGS=
CXX_WORKAROUND_FLAGS=-Wno-variadic-macros -Wno-ignored-qualifiers

ARDUINO_SOURCES=	CDC.o \
			HardwareSerial.o \
			HID.o \
			IPAddress.o \
			main.o \
			new.o \
			Print.o \
			Stream.o \
			Tone.o \
			USBCore.o \
			WInterrupts.o \
			wiring_analog.o \
			wiring.o \
			wiring_digital.o \
			wiring_pulse.o \
			wiring_shift.o \
			WMath.o \
			WString.o

%.o : %.c
	$(CC) $(CFLAGS) $(NOISYFLAGS) -c $< -o $@

%.o : %.cpp $(HEADERS)
	$(CXX) $(NOISYFLAGS) $(CXXFLAGS) $(CXX_WORKAROUND_FLAGS) -c $< -o $@

%.cpp : %.pde
	echo "#include <Arduino.h>" > $@
	cat $< >> $@

%.o : $(ARDUINO_SOURCES_DIR)/%.c
	$(CC) $(CFLAGS) -c $< -o $@

%.o : $(ARDUINO_SOURCES_DIR)/%.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

%.o : $(ARDUINO_SPI_LIB_DIR)/%.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

# all:
# see above.

clean:
	rm -f *.o *.a *.hex $(APPNAME)
	rm -f $(SOURCES)
	rm -f $(TEST_SOURCES)
	rm -f run_tests

upload: $(APPNAME).hex
	stty -F $(PORT) hupcl # e.g. reset the arduino
	avrdude -v -c $(AVRDUDE_STK) -p $(AVRDUDE_MCU) \
		-b $(AVRDUDE_BAUD) -P $(PORT) -U flash:w:$(APPNAME).hex

%.hex : %
	avr-objcopy -O ihex -R .eeprom $< $@

libarduinocore.a: $(ARDUINO_SOURCES)
	ar rc $@ $^

$(APPNAME) : $(SOURCES) libarduinocore.a
	$(CXX) $(CXXFLAGS) $(NOISYFLAGS) $^ -o $@ -L. -larduinocore


test: run_tests
	./run_tests

run_tests: $(SOURCES) $(TEST_SOURCES)
	$(CXX) $(CXXFLAGS) -o run_tests $^
