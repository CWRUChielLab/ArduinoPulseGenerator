ArduinoPulseGenerator
=====================

This is a simple program for generating pulse sequences (with pulse widths on
the order of milliseconds) using an Arduino.  There is an associate GUI that
runs on the local computer, or you can simply connect to the Arduino with a
serial console (9600 baud) and send it commands.  This code has been tested
on the ArduinoMega 2560 and Arduino Due; it may work on other Arduino boards
but this has not been tested.

This is a quick hack for some electrophysiology work being done in our lab, but
if you need specific features please request them by opening a new issue on
github.

For information on using this software, please see the `ArduinoPulseGenerator
website <http://kms15.github.com/ArduinoPulseGenerator/>`_.


Building the Software
=====================

Firmware
--------

To build the firmware that runs on the Arduino, you will first need to install
the Arduino software.  This can be downloaded from the `Arduino site
<http://arduino.cc/en/Main/Software>`_, or on GNU/Linux installed using your
package manager (e.g. on Ubuntu or Debian you can type "sudo apt-get install
arduino" at the command line).

Next you will need to upload the software to your device.  To do this, first
open the Arduino IDE, and under the tools->board menu choose the type of
Arduino board you have.  Next, choose File->Open, and select the
PulseGeneratorFirmware.pde file in the PulseGeneratorFirmware directory
of the ArduinoPulseGenerator source code.  Finally, connect your
Arduino to the computer (if it's not already connected) and choose
File->Upload to upload the firmware to your Arduino.


Graphical User Interface
------------------------

To build the graphical user interface which runs on your computer, you will
first need to install the Qt development tools.  These can be downloaded from
the `Qt Project site <https://qt-project.org/downloads>`_, or on GNU/Linux
installed using your package manager (e.g. on Ubuntu or Debian you can type
"sudo apt-get install libqt4-dev" at the command line).

The GUI can be built by opening a command line, navigating to the directory
where you've place the ArduinoPulseGenerator source code, and then running::

    qmake
    make

For more information on qmake and potential troubleshooting, the Qt project
website has a number of `tutorials
<https://qt-project.org/resources/getting_started>`_.


License
=======

ArduinoPulseGenerator is released under a GPLv3 License (see the file
"COPYING" for details, or the FSF site for a `a discussion of the GPL
license <https://www.gnu.org/licenses/quick-guide-gplv3>`_).

