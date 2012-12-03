#include "pulseStateMachine.h"

const int maxInputLength = 80;
char inputLine[maxInputLength + 1];
int numChars = 0;

int lineNum = 1;
const int maxCommands = 200;
PulseStateCommand commands[maxCommands];
int numCommands = 0;
unsigned repeatDepth = 0;

int channelPins[numChannels] = { 2, 3, 4, 5, 8, 9, 10, 11 };

void setup() {
    // set up the pins as outputs
    for (unsigned int i = 0; i < numChannels; ++i) {
        pinMode(channelPins[i], OUTPUT);
    }

    // set up the serial port
    Serial.begin(9600);
    while (!Serial) {  // wait needed on Arduino Leonardo
    }

    Serial.print("1: ");
}

void loop() {
    // get any incoming bytes until we have a complete line:
    if (Serial.available() > 0) {
        int thisChar = Serial.read();
        Serial.write(thisChar);


        // if we have a complete line...
        if (thisChar == '\r' || thisChar == '\n') {
            inputLine[numChars] = '\0';

            Serial.println();

            if (numChars == maxInputLength) {
                Serial.print("Line too long (must be shorter than ");
                Serial.print(maxInputLength);
                Serial.println(" characters)\07");
            } else {
                const char* error = NULL;
                commands[numCommands].parseFromString(inputLine, &error, &repeatDepth);

                if (error) {
                    Serial.print("error: ");
                    Serial.print(error);
                    Serial.println("\07");
                } else if (commands[numCommands].type == PulseStateCommand::endProgram) {
                    // run the program
                    Serial.println("Running program...");

                    PulseChannel channels[numChannels];
                    RepeatStack stack;
                    int runningCommandIndex = 0;
                    Microseconds prevTime = micros();
                    Microseconds timeInState = 0;

                    Microseconds maxError = 0;
                    while (runningCommandIndex < numCommands) {
                        Microseconds newTime = micros();
                        Microseconds timeAvailable = newTime - prevTime;
                        Microseconds lastTimeAvailable = timeAvailable;

                        // track the maximum iteration length
                        if (timeAvailable > maxError) {
                            maxError = timeAvailable;
                        }

                        // update the channel states
                        for (unsigned int i = 0; i < numChannels; ++i) {
                            channels[i].advanceTime(timeAvailable);
                        }

                        int step;

                        // run commands until we're out of time
                        while (0 != (step = commands[runningCommandIndex].execute(
                                    channels, &stack, runningCommandIndex,
                                    timeInState, &timeAvailable))) {
                            runningCommandIndex += step;
                            timeInState = 0;
                            lastTimeAvailable = timeAvailable;
                        }
                        timeInState += lastTimeAvailable;

                        // update the pins
                        for (unsigned int i = 0; i < numChannels; ++i) {
                            digitalWrite(channelPins[i], channels[i].on() ? HIGH : LOW);
                        }

                        prevTime = newTime;
                    }

                    lineNum = 1;
                    numCommands = 0;
                    // N.B.: This message must be kept in sync with
                    // ProgramGuiWindow.cpp
                    Serial.print("done.  (timing precision was better than ");
                    Serial.print(maxError);
                    Serial.println(" microseconds)\07");

                } else if (numCommands == maxCommands - 1) {
                    Serial.print("error: program too long (max ");
                    Serial.print(maxCommands);
                    Serial.println(" commands)\07");
                    lineNum = 1;
                    numCommands = 0;
                    repeatDepth = 0;
                } else if (commands[numCommands].type == PulseStateCommand::noOp) {
                    lineNum++;
                } else {
                    lineNum++;
                    numCommands++;
                }
            }

            numChars = 0;
            Serial.print(lineNum);
            Serial.print(": ");
        } else if (numChars < maxInputLength) {
            inputLine[numChars++] = thisChar;
        }
    }
}
