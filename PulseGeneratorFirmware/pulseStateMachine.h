#ifndef PULSESTATEMACHINE_H
#define PULSESTATEMACHINE_H
#include <stdint.h>

// A duration of time, in microseconds.
typedef uint32_t Microseconds;

// A duration much longer than any duration used in the program.
const Microseconds forever = 0xFFFFFFFF;

// Maximum number of pulse channels supported by the firmware.
const int numChannels = 4;

// Stores the state of a single channel that can generate a square wave.
// call advanceTime to update the on/off state to reflect where you are
// in the waveform.
class PulseChannel {
    private:
        enum { numStates=2 };
        bool m_on;
        Microseconds m_stateTime[numStates];
        Microseconds m_timeInState;

    public:
        // Constructor
        PulseChannel();

        // true iff the channel should be on at this moment in time.
        bool on() const { return m_on; }

        // gets the current amount of time spent in the on state before
        // switching off.
        Microseconds onTime() const { return m_stateTime[true]; }

        // gets the current amount of time spent in the off state before
        // switching on.
        Microseconds offTime() const { return m_stateTime[false]; }

        // sets how long the channel should spend in the on and off state
        // for each period of the square wave.  Note that the sum of the
        // two times is the period of the square wave.
        Microseconds setOnOffTime(Microseconds on, Microseconds off);

        // update the on/off state of the channel to reflect the passage of
        // dt microseconds of time.
        bool advanceTime(Microseconds dt);
};


// A single instruction in a program controlling pulse state.
//
// Examples:
//        # This program turns on channel 1 (i.e. sets the output to high) and
//        # fires off a series of 500 microsecond pulses on channel 3 at
//        # 400 Hz ( = 1 / (2 ms + 0.5 ms)).  After 3 seconds it turns off
//        # both channels (i.e. sets both outputs to 0 volts).
//
//        turn on channel 1
//        change channel 3 to repeat 500 us on 2 ms off
//        wait 3 s
//        turn off channel 1
//        turn off channel 3
//        end
//
// Formal syntax:
//
//    command := "end" |
//               "change" channel "to repeat" time "on" time "off" |
//               "turn on" channel |
//               "turn off" channel |
//               "wait" time;
//    channel := "channel" integer;
//    time := integer timeunit;
//    timeunit := "s" | "ms" | "us" |
//               "\xB5s" |                 # latin 1 micro
//               "\x00B5s" |               # utf8 micro
//               "\x03BCs";                # greek mu
//    integer := "[0-9]+"
//
struct PulseStateCommand {
    public:
        enum Type {
            end,
            setChannel,
            wait
        };

        Type type;
        union {
            struct {
                uint8_t channel;
                Microseconds onTime;
                Microseconds offTime;
            };
            struct {
                Microseconds waitTime;
            };
        };

    public:
        // Constructor.
        PulseStateCommand() : type(end) {};

        // Converts one line of human-readable text (input) into a
        // pulseStateCommand.  The error parameter will be set to NULL
        // if the conversion was successful; otherwise the error parameter
        // will point to a human-readable string describing the parsing error.
        void parseFromString(const char* input, const char** error);

        // Runs the command, updating the on/off times for the channels as
        // needed.  The timeAvailable parameter should initially contain the
        // amount of time that has passed but has not been accounted for.
        // After the call, this will be reduced by the amount of time spent
        // executing the command.  For example, if the command was
        // "wait 100 us" and initially 310 us of time was available, the
        // timeAvailable parameter would initially be 310 us before the call
        // and 210 us after the call.  If time available is 0 after the call,
        // the instruction has not completed yet and should be called again
        // when more time is available.
        void execute(PulseChannel* channels, Microseconds* timeAvailable);
};

#endif /* PULSESTATEMACHINE_H */
