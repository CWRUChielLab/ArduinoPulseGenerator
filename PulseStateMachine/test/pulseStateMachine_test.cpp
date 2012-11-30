#include <iostream>
#include <assert.h>
#include <stdlib.h>
#include "pulseStateMachine.h"
#include "string.h"

using std::cout;
using std::endl;

#define assertClose(X, Y) assert(abs((X) - (Y)) <= 1)

void runPulseChannelTests() {
    // should be off by default
    {
        PulseChannel p;
        assert(p.on() == false);
        assert(p.onTime() == 0);
        //assert(p.offTime() == 0); // no off pulse to finish
        p.advanceTime(1);
        assert(p.on() == false);
        p.advanceTime(1);
        assert(p.on() == false);
    }

    // should be able to set on and off times, resulting in appropriate on and
    // off intervals.
    {
        PulseChannel p;
        p.setOnOffTime(20, 50);
        assert(p.on() == true);
        p.advanceTime(19);
        assert(p.on() == true);
        p.advanceTime(1);
        assert(p.on() == false);
        p.advanceTime(49);
        assert(p.on() == false);
        p.advanceTime(1);
        assert(p.on() == true);
        p.advanceTime(19);
        assert(p.on() == true);
        p.advanceTime(1);
        assert(p.on() == false);
        p.advanceTime(49);
        assert(p.on() == false);
        p.advanceTime(1);
        assert(p.on() == true);
    }

    // setting 0 for on time should turn off and leave off
    {
        PulseChannel p;
        p.setOnOffTime(0, 10);
        assert(p.on() == false);
        p.setOnOffTime(20, 50);
        assert(p.on() == true);
        p.setOnOffTime(0, 10);
        assert(p.on() == false);
    }

    // setting 0 for on time should turn on and leave on
    {
        PulseChannel p;
        p.setOnOffTime(10, 0);
        assert(p.on() == true);
        p.setOnOffTime(20, 50);
        assert(p.on() == true);
        p.setOnOffTime(10, 0);
        assert(p.on() == true);
    }

    // should correctly compute time until next state change
    {
        PulseChannel p;
        p.setOnOffTime(20, 50);
        assert(p.timeUntilNextStateChange() == 20);
        p.advanceTime(11);
        assert(p.timeUntilNextStateChange() == 9);
        p.advanceTime(9);
        assert(p.timeUntilNextStateChange() == 50);
        p.advanceTime(23);
        assert(p.timeUntilNextStateChange() == 27);
        p.advanceTime(27);
        assert(p.timeUntilNextStateChange() == 20);
    }
}


void runPulseStateCommandParsingTests() {
    // Basic parsing tests
    {
        // completely empty line
        PulseStateCommand c;
        const char* error;
        c.parseFromString("", &error, NULL);
        assert(error == NULL);
        assert(c.type == PulseStateCommand::noOp);
    }
    {
        // line with only whitespace
        PulseStateCommand c;
        const char* error;
        c.parseFromString(" \t", &error, NULL);
        assert(error == NULL);
        assert(c.type == PulseStateCommand::noOp);
    }
    {
        // line with just a comment
        PulseStateCommand c;
        const char* error;
        c.parseFromString("# a comment", &error, NULL);
        assert(error == NULL);
        assert(c.type == PulseStateCommand::noOp);
    }
    {
        // line with indented comment
        PulseStateCommand c;
        const char* error;
        c.parseFromString("   # a comment", &error, NULL);
        assert(error == NULL);
        assert(c.type == PulseStateCommand::noOp);
    }
    {
        PulseStateCommand c;
        const char* error;
        unsigned repeatDepth = 0;
        c.parseFromString("end program", &error, &repeatDepth);
        assert(error == NULL);
        assert(c.type == PulseStateCommand::endProgram);
    }
    {
        // line ending with a comment
        PulseStateCommand c;
        const char* error;
        unsigned repeatDepth = 0;
        c.parseFromString("end program # forever", &error, &repeatDepth);
        assert(error == NULL);
        assert(c.type == PulseStateCommand::endProgram);
    }
    {
        PulseStateCommand c;
        const char* error;
        // fractional s, fractional Hz
        c.parseFromString("set channel 2 to 2.31 s pulses at 0.125 Hz", &error, NULL);
        assert(error == NULL);
        assert(c.type == PulseStateCommand::setChannel);
        assert(c.channel == 2);
        assertClose(c.onTime, 2310000);
        assertClose(c.offTime, 8000000 - 2310000);
    }
    {
        PulseStateCommand c;
        const char* error;
        // ms, fractional Hz
        c.parseFromString("set channel 2 to 30 ms pulses at .5 Hz", &error, NULL);
        assert(error == NULL);
        assert(c.type == PulseStateCommand::setChannel);
        assert(c.channel == 2);
        assert(c.onTime == 30000);
        assert(c.offTime == 2000000 - 30000);
    }
    {
        PulseStateCommand c;
        const char* error;
        // us micro, integer Hz
        c.parseFromString("set channel 3 to 213 us pulses at 2000 Hz", &error, NULL);
        assert(error == NULL);
        assert(c.type == PulseStateCommand::setChannel);
        assert(c.channel == 3);
        assert(c.onTime == 213);
        assert(c.offTime == 500 - 213);
    }
    {
        PulseStateCommand c;
        const char* error;
        // Latin-1 micro, integer kHz
        c.parseFromString("set channel 2 to  182 \xB5s pulses at 4 kHz", &error, NULL);
        assert(error == NULL);
        assert(c.type == PulseStateCommand::setChannel);
        assert(c.channel == 2);
        assert(c.onTime == 182);
        assert(c.offTime == 250 - 182);
    }
    {
        PulseStateCommand c;
        const char* error;
        // utf-8 micro and decimal kHz, extra whitespace
        c.parseFromString("  set channel  1  to  13  \u00B5s  pulses  at 2.5 kHz", &error, NULL);
        assert(error == NULL);
        assert(c.type == PulseStateCommand::setChannel);
        assert(c.channel == 1);
        assert(c.onTime == 13);
        assert(c.offTime == 400 - 13);
    }
    {
        PulseStateCommand c;
        const char* error;
        // utf-8 greek lowercase mu and fractional Hz, extra tabs
        c.parseFromString("\tset channel  4  to \t 13000  \u03BCs  \t pulses  at  \t2.5 Hz", &error, NULL);
        assert(error == NULL);
        assert(c.type == PulseStateCommand::setChannel);
        assert(c.channel == 4);
        assert(c.onTime == 13000);
        assert(c.offTime == 400000 - 13000);
    }
    {
        PulseStateCommand c;
        const char* error;
        c.parseFromString("turn off channel 4", &error, NULL);
        assert(error == NULL);
        assert(c.type == PulseStateCommand::setChannel);
        assert(c.channel == 4);
        assert(c.onTime == 0);
        assert(c.offTime == forever);
    }
    {
        PulseStateCommand c;
        const char* error;
        c.parseFromString("turn on channel 2", &error, NULL);
        assert(error == NULL);
        assert(c.type == PulseStateCommand::setChannel);
        assert(c.channel == 2);
        assert(c.onTime == forever);
        assert(c.offTime == 0);
    }
    {
        PulseStateCommand c;
        const char* error;
        c.parseFromString("wait 273 ms", &error, NULL);
        assert(error == NULL);
        assert(c.type == PulseStateCommand::wait);
        assert(c.waitTime == 273000);
    }
    {
        PulseStateCommand c;
        const char* error;
        unsigned repeatDepth = 3;
        c.parseFromString("repeat 10 times:", &error, &repeatDepth);
        assert(error == NULL);
        assert(c.type == PulseStateCommand::repeat);
        assert(c.repeatCount == 10);
        assert(repeatDepth == 4);
    }
    {
        PulseStateCommand c;
        const char* error;
        unsigned repeatDepth = 6;
        c.parseFromString("end repeat", &error, &repeatDepth);
        assert(error == NULL);
        assert(c.type == PulseStateCommand::endRepeat);
        assert(repeatDepth == 5);
    }

    // Error tests
    {
        PulseStateCommand c;
        const char* error;
        c.parseFromString("turn off channel 2 some day", &error, NULL);
        assert(error && strcmp(error, "unexpected text found after end of command") == 0);
    }
    {
        PulseStateCommand c;
        const char* error;
        assert(numChannels == 8);
        c.parseFromString("turn off channel 9", &error, NULL);
        assert(error && strcmp(error, "channel number must be between 1 and 8") == 0);
    }
    {
        PulseStateCommand c;
        const char* error;
        c.parseFromString("end", &error, NULL);
        assert(error && strcmp(error, "expected \"repeat\" or \"program\"") == 0);
    }
    {
        PulseStateCommand c;
        const char* error;
        unsigned repeatDepth = 0;
        c.parseFromString("end repeat", &error, &repeatDepth);
        assert(error && strcmp(error, "found \"end repeat\" without matching \"repeat\"") == 0);
    }
    {
        PulseStateCommand c;
        const char* error;
        unsigned repeatDepth = 1;
        c.parseFromString("end program", &error, &repeatDepth);
        assert(error && strcmp(error, "found \"end program\" while still expecting an \"end repeat\"") == 0);
    }
    {
        PulseStateCommand c;
        const char* error;
        unsigned repeatDepth = maxRepeatNesting;
        c.parseFromString("repeat 10 times:", &error, &repeatDepth);
        assert(error && strcmp(error, "repeats nested too deeply") == 0);
    }
    {
        // number too large - this isn't a great error message, but it's not
        // a common error.
        PulseStateCommand c;
        const char* error;
        unsigned repeatDepth = 0;
        c.parseFromString("repeat 5000000000 times:", &error, &repeatDepth);
        assert(error && strcmp(error, "expected repeat count") == 0);
    }
    {
        // number too large edge case
        PulseStateCommand c;
        const char* error;
        unsigned repeatDepth = 0;
        c.parseFromString("repeat 4294967296 times:", &error, &repeatDepth);
        assert(error && strcmp(error, "expected repeat count") == 0);
    }
    // TODO: tests for other error messages.

    // cout << "Error: " << (error ? error : "none") << endl;
}


void runPulseStateCommandExecuteTests() {
    int step;

    // should be able to run commands
    {
        PulseStateCommand c;
        Microseconds remainingTime = 100;

        c.type = PulseStateCommand::endProgram;

        step = c.execute(NULL, NULL, 0, 0, &remainingTime);
        assert(remainingTime == 0);
        assert(step == 0);

        remainingTime = 100;
        step = c.execute(NULL, NULL, 0, 100, &remainingTime);
        assert(remainingTime == 0);
        assert(step == 0);
    }
    {
        PulseStateCommand c;

        c.type = PulseStateCommand::noOp;

        step = c.execute(NULL, NULL, 0, 0, NULL);
        assert(step == 1);
    }
    {
        PulseStateCommand c;
        PulseChannel p[numChannels];

        c.type = PulseStateCommand::setChannel;
        c.channel = 1;
        c.onTime = 12;
        c.offTime = 10;

        step = c.execute(p, NULL, 0, 0, NULL);
        assert(p[0].onTime() == 12);
        assert(p[0].offTime() == 10);
        assert(step == 1);
    }
    {
        PulseStateCommand c;
        Microseconds remainingTime = 100;

        c.type = PulseStateCommand::wait;
        c.waitTime = 120;

        step = c.execute(NULL, NULL, 0, 0, &remainingTime);
        assert(remainingTime == 0);
        assert(step == 0);

        remainingTime = 70;
        step = c.execute(NULL, NULL, 0, 100, &remainingTime);
        assert(remainingTime == 50);
        assert(step == 1);
    }
    {
        PulseStateCommand c;
        PulseChannel p[numChannels];
        RepeatStack stack;

        c.type = PulseStateCommand::repeat;
        c.repeatCount = 13;
        stack.pushRepeat(42, 83);

        step = c.execute(p, &stack, 5, 0, NULL);
        assert(step == 1);
        assert(stack.getLoopTarget() == 6);
        stack.pop();
        assert(stack.getLoopTarget() == 42);
        assert(stack.decrementRepeatCount() == 82);
    }
    {
        PulseStateCommand c;
        PulseChannel p[numChannels];
        RepeatStack stack;

        c.type = PulseStateCommand::endRepeat;

        stack.pushRepeat(23, 83);
        step = c.execute(p, &stack, 42, 0, NULL);
        assert(step == 23 - 42);
        assert(stack.decrementRepeatCount() == 81);
        assert(stack.getLoopTarget() == 23);
    }
    {
        PulseStateCommand c;
        PulseChannel p[numChannels];
        RepeatStack stack;

        c.type = PulseStateCommand::endRepeat;

        stack.pushRepeat(42, 83);
        stack.pushRepeat(17, 1);
        step = c.execute(p, &stack, 23, 0, NULL);
        assert(step == 1);
        assert(stack.decrementRepeatCount() == 82);
        assert(stack.getLoopTarget() == 42);
    }
}


int main() {
    cout << "running PulseChannel tests\n";
    runPulseChannelTests();
    cout << "running PulseStateCommand parsing tests\n";
    runPulseStateCommandParsingTests();
    cout << "running PulseStateCommand execute tests\n";
    runPulseStateCommandExecuteTests();
    cout << "**************** Tests passed! ****************\n";
    return 0;
}
