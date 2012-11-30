#ifndef PULSESTATEMACHINE_H
#define PULSESTATEMACHINE_H
#include <stdint.h>

// A duration of time, in microseconds.
typedef uint32_t Microseconds;

// A duration much longer than any duration used in the program.
const Microseconds forever = 0xFFFFFFFF;

// Maximum number of pulse channels supported by the firmware.
const unsigned numChannels = 8;

// Maximum number of nested repeats
const unsigned maxRepeatNesting = 20;

// Store the state of repeats in a running program.
class RepeatStack {
    private:
        int m_loopTarget[maxRepeatNesting];
        uint32_t m_repeatCount[maxRepeatNesting];
        unsigned m_size;

    public:
        RepeatStack() { m_size = 0; };

        // enter into a new repeat loop
        void pushRepeat(int loopTarget, uint32_t repeatCount) {
            m_loopTarget[m_size] = loopTarget;
            m_repeatCount[m_size] = repeatCount;
            ++m_size;
        };

        // decrement and return the number of iterations remaining
        uint32_t decrementRepeatCount() {
            return --(m_repeatCount[m_size - 1]);
        };

        // get the current loop target
        int getLoopTarget() { return m_loopTarget[m_size - 1]; }

        // exit the current loop
        void pop() { --m_size; }
};


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
        void setOnOffTime(Microseconds on, Microseconds off);

        // update the on/off state of the channel to reflect the passage of
        // dt microseconds of time.
        void advanceTime(Microseconds dt);

        // Compute the minimum time that must advance for the next state
        // change to occur.
        Microseconds timeUntilNextStateChange() const;
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
            endProgram,
            setChannel,
            wait,
            repeat,
            endRepeat,
            noOp
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
            struct {
                uint32_t repeatCount;
            };
        };

    public:
        // Constructor.
        PulseStateCommand() : type(noOp) {};

        // Converts one line of human-readable text (input) into a
        // pulseStateCommand.  The error parameter will be set to NULL
        // if the conversion was successful; otherwise the error parameter
        // will point to a human-readable string describing the parsing error.
        // Repeat depth contains the current depth of nested repeats, and
        // will be automatically incremented or decremented by the parsed
        // command.
        void parseFromString(const char* input, const char** error,
                unsigned* repeatDepth);

        // Runs the command, updating the on/off times for the channels
        // (passed in the channels parameter) as needed.
        //
        // The timeInState parameter should contain the total
        // time spent in previous ticks since this state has been entered,
        // e.g. if we've used up 130 us in previous ticks without finishing
        // this command, this should be 130.
        //
        // The timeAvailable parameter should initially contain the amount of
        // time that has passed this tick but has not been accounted for.
        // After the call, this will be reduced by the amount of time spent
        // executing the command.  For example, if the command was "wait 100
        // us" and initially 310 us of time was available, the timeAvailable
        // parameter would initially be 310 us before the call and 210 us after
        // the call.
        //
        // The return value is the number of commands to advance (0 iff the
        // command was not completed this tick, 1 when a normal command
        // completed, and the relative distance to the jump target for jumps)
        int execute(PulseChannel* channels, RepeatStack* stack,
                int commandId, Microseconds timeInState,
                Microseconds* timeAvailable);
};

#endif /* PULSESTATEMACHINE_H */
