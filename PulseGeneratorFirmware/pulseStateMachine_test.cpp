#include <iostream>
#include <assert.h>
#include "pulseStateMachine.h"

using std::cout;
using std::endl;

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
}


void runPulseStateCommandTests() {
    // verify that parsing works
    {
        PulseStateCommand c;
        const char* error;
        c.parseFromString("end", &error);
        assert(error == NULL);
        assert(c.type == PulseStateCommand::end);
    }
    {
        PulseStateCommand c;
        const char* error;
        // us and Latin-1 micro
        c.parseFromString("change channel 3 to repeat 213 us on 182 \xB5s off", &error);
        assert(error == NULL);
        assert(c.type == PulseStateCommand::setChannel);
        assert(c.channel == 3);
        assert(c.onTime == 213);
        assert(c.offTime == 182);
    }
    {
        PulseStateCommand c;
        const char* error;
        c.parseFromString("change channel 1 to repeat 12 ms on 18 s off", &error);
        assert(error == NULL);
        assert(c.type == PulseStateCommand::setChannel);
        assert(c.channel == 1);
        assert(c.onTime == 12000);
        assert(c.offTime == 18000000UL);
    }
    {
        PulseStateCommand c;
        const char* error;
        // utf-8 micro and utf-8 greek lowercase mu
        c.parseFromString("change channel 4 to repeat 13 \u00B5s on 23 \u03BCs off", &error);
        assert(error == NULL);
        assert(c.type == PulseStateCommand::setChannel);
        assert(c.channel == 4);
        assert(c.onTime == 13);
        assert(c.offTime == 23);
    }
    {
        PulseStateCommand c;
        const char* error;
        c.parseFromString("turn off channel 5", &error);
        assert(error == NULL);
        assert(c.type == PulseStateCommand::setChannel);
        assert(c.channel == 5);
        assert(c.onTime == 0);
        assert(c.offTime == forever);
    }
    {
        PulseStateCommand c;
        const char* error;
        c.parseFromString("turn on channel 2", &error);
        assert(error == NULL);
        assert(c.type == PulseStateCommand::setChannel);
        assert(c.channel == 2);
        assert(c.onTime == forever);
        assert(c.offTime == 0);
    }
    {
        PulseStateCommand c;
        const char* error;
        c.parseFromString("wait 273 ms", &error);
        assert(error == NULL);
        assert(c.type == PulseStateCommand::wait);
        assert(c.waitTime == 273000);
    }
    // cout << "Error: " << (error ? error : "none") << endl;
}


int main() {
    cout << "running PulseChannel tests\n";
    runPulseChannelTests();
    cout << "running PulseStateCommand tests\n";
    runPulseStateCommandTests();
    cout << "**************** Tests passed! ****************\n";
    return 0;
}
