#include "pulseStateMachine.h"

const int maxInputLength = 80;
char inputLine[maxInputLength + 1];
int numChars = 0;

int lineNum = 0;
const int maxLines = 80;
PulseStateCommand commands[maxLines];

int channelPins[numChannels] = { 2, 3, 4, 5 };

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
                Serial.println(" characters)");
            } else {
                const char* error = NULL;
                commands[lineNum].parseFromString(inputLine, &error);

                if (error) {
                    Serial.print("error: ");
                    Serial.println(error);
                } else if (commands[lineNum].type == PulseStateCommand::end) {
                    // run the program
                    Serial.println("Running program...");

                    PulseChannel channels[numChannels];
                    int runningLine = 0;
                    Microseconds prevTime = micros();
                    Microseconds timeInState = 0;

                    Microseconds maxError = 0;
                    while (runningLine < lineNum) {
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

                        // run commands until we're out of time
                        while (commands[runningLine].execute(
                                    channels, timeInState, &timeAvailable)) {
                            ++runningLine;
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

                    lineNum = 0;
                    Serial.print("done.  (timing precision was better than ");
                    Serial.print(maxError);
                    Serial.println(" microseconds)");

                } else if (lineNum == maxLines - 1) {
                    Serial.print("Error: program too long (max ");
                    Serial.print(maxLines);
                    Serial.println(" lines)");
                    lineNum = 0;
                } else {
                    lineNum++;
                }
            }

            numChars = 0;
            Serial.print(lineNum + 1);
            Serial.print(": ");
        } else if (numChars < maxInputLength) {
            inputLine[numChars++] = thisChar;
        }
    }
}
