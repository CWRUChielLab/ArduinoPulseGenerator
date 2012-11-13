ArduinoPulseGeneratorGui
========================

This is a simple program for generating pulse sequences (with pulse widths on
the order of milliseconds) using an Arduino.  There is an associate GUI that
runs on the local computer, or you can simply connect to the Arduino with a
serial console (9600 baud) and send it commands.

This is a quick hack for some electrophysiology work being done in our lab, but
if you need specific features please request them by opening a new issue on
github.


Building
========

The arduino firmware can be built by opening
PulseGeneratorFirmware/PulseGenerator.pde in the Arduino environment.
Alternatively, the included firmware/Makefile will build and upload (using
"make upload") the firmware for the arduino on Ubuntu 12.10; other systems
may need some modifications to the makefile.

The GUI can be built by running "qmake" and "make" in the project directory
(after installing Qt).


Usage
=====

Pulse sequences are defined by simple programs.  For example, the following
program will generate 15 ms pulses on channel 1 for 3 seconds:

    change channel 1 to repeat 15 ms on 85 ms off
    wait 3 s
    turn off channel 1
    end

Channels 1, 2, 3, and 4 are mapped to arduino pins 13, 2, 3, and 4
respectively.


License
=======

ArduinoPulseGeneratorGui is released under a GPLv3 License (see the file
"COPYING" for details).

