######################################################################
# Automatically generated by qmake (2.01a) Sat Apr 9 21:20:03 2011
######################################################################

TEMPLATE = app
TARGET = ArduinoPulseGenerator
DEPENDPATH += .
INCLUDEPATH += . PulseStateMachine
CONFIG += no_keywords

QT       += core gui
greaterThan(QT_MAJOR_VERSION, 4): QT += widgets printsupport concurrent

# libraries
include(libs/qwt/src/qwt.pri)
include(libs/qextserialport/src/qextserialport.pri)

# support for building universal binaries on the mac
isEmpty( MACTARGET ) {
   MACTARGET = intel
}
macx {
   contains( MACTARGET, intel ) {
      message( "Adding support for intel macs" )
      CONFIG += x86
   }

   contains( MACTARGET, ppc ) {
      message( "Adding support for ppc macs" )
      CONFIG += ppc

      # if we are building a universal binary set the SDK path to the universal SDK
      contains( MACTARGET, intel ) {
         QMAKE_MAC_SDK = /Developer/SDKs/MacOSX10.4u.sdk
      }
   }
}

# Input
HEADERS += ProgramGuiWindow.h
SOURCES += ProgramGuiWindow.cpp PulseGeneratorGui.cpp PulseStateMachine/pulseStateMachine.cpp
