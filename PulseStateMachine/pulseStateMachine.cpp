#include "pulseStateMachine.h"
#include <stddef.h>


PulseChannel::PulseChannel()
    : m_on(false), m_timeInState(0)
{
    m_stateTime[false] = forever;
    m_stateTime[true] = 0;
}


void PulseChannel::setOnOffTime(Microseconds on, Microseconds off) {
    m_stateTime[true] = on;
    m_stateTime[false] = off;
    m_on = on > 0;
    m_timeInState = 0;
}


void PulseChannel::advanceTime(Microseconds dt) {
    m_timeInState += dt;

    while (m_timeInState >= m_stateTime[m_on]) {
        m_timeInState -= m_stateTime[m_on];
        m_on = !m_on;
    }
}


Microseconds PulseChannel::timeUntilNextStateChange() const {
    return m_stateTime[m_on] - m_timeInState;
}



static bool consumeToken(const char* token, const char* input, int* index) {
    int i = 0;
    while (token[i] != 0) {
        if (token[i] != input[*index + i]) {
            return false; // mismatch
        }
        i++;
    }

    *index += i;
    return true;
}


static void consumeWhitespace(const char* input, int* index) {
    int i = 0;

    while (input[*index + i] == ' ' || input[*index + i] == '\t' ||
            input[*index + i] == '\n' || input[*index + i] == '\r'|| 
            input[*index + i] == '#') {

        if (input[*index + i] == '#') {
            // for comments, read to end of line/string
            while (input[*index + i] != '\n' && input[*index + i] != '\r' &&
                    input[*index + i] != 0) {
                i++;
            }
        } else {
            i++;
        }
    }

    *index += i;
}

static bool consumeUInt32(const char* input, int* index, uint32_t* result) {
    uint32_t val = 0;

    if (!(input[*index] >= '0' && input[*index] <= '9')) {
        return false;
    }

    while (input[*index] >= '0' && input[*index] <= '9') {
        uint32_t val10 = 10 * val;
        uint32_t newVal = val10 + input[*index] - '0';
        (*index)++;

        if (val10 / 10 != val || newVal < val10) {
            // overflow
            return false;
        }
        val = newVal;
    }

    *result = val;
    return true;
}


static bool consumeDouble(const char* input, int* index, double* result) {
    double val = 0;

    if (input[*index] != '.' && !(input[*index] >= '0' && input[*index] <= '9')) {
        return false;
    }

    // read the integer component
    while (input[*index] >= '0' && input[*index] <= '9') {
        val = 10 * val + input[*index] - '0';
        (*index)++;
    }

    // read the decimal component, if present
    if (input[*index] == '.') {
        (*index)++;

        double place = 1.;
        while (input[*index] >= '0' && input[*index] <= '9') {
            place = place / 10.;
            val = val + place * (input[*index] - '0');
            (*index)++;
        }
    }

    *result = val;
    return true;
}


static bool consumeTime(const char* input, int* index, Microseconds* result) {
    const char* expectedToken;
    double val;

    if (!consumeDouble(input, index, &val)) {
        return false;
    }

    consumeWhitespace(input, index);
    switch (input[*index]) {
        case 'u':
            expectedToken = "us";
            break;

        case '\xB5': // Latin-1 micro
            expectedToken = "\xB5s";
            break;

        case '\xC2': // micro, utf-8
            expectedToken = "\xC2\xB5s";
            break;

        case '\xCE': // mu, utf-8
            expectedToken = "\xCE\xBCs";
            break;

        case 'm':
            expectedToken = "ms";
            val *= 1000;
            break;

        case 's':
            expectedToken = "s";
            val *= 1000000;
            break;

        default:
            return false;
    }

    *result = Microseconds(val);

    return consumeToken(expectedToken, input, index);
}


static bool consumeFrequency(const char* input, int* index, Microseconds* result) {
    const char* expectedToken;
    double val;

    if (!consumeDouble(input, index, &val)) {
        return false;
    }

    consumeWhitespace(input, index);
    switch (input[*index]) {
        case 'H':
            expectedToken = "Hz";
            break;

        case 'k':
            expectedToken = "kHz";
            val *= 1000;
            break;

        default:
            return false;
    }

    *result = Microseconds(1000000/val);

    return consumeToken(expectedToken, input, index);
}


void PulseStateCommand::parseFromString(const char* input, const char** error,
        unsigned* repeatDepth) {
    int index = 0;
    uint32_t val;

    consumeWhitespace(input, &index);
    if (input[index] == 0) {
        // empty line, comment, etc.
        type = noOp;
    } else if (input[index] == 'e') {
        // e.g. "end program" or "end repeat"
        if (!consumeToken("end", input, &index)) {
            *error = "unrecognized command";
            return;
        }
        consumeWhitespace(input, &index);

        if (input[index] == 'p') {
            if (!consumeToken("program", input, &index)) {
                *error = "unrecognized command";
                return;
            }
            if (*repeatDepth != 0) {
                *error = "found \"end program\" while still expecting an \"end repeat\"";
                return;
            }
            type = endProgram;
        } else if (input[index] == 'r') {
            if (!consumeToken("repeat", input, &index)) {
                *error = "unrecognized command";
                return;
            }
            if (*repeatDepth == 0) {
                *error = "found \"end repeat\" without matching \"repeat\"";
                return;
            }
            type = endRepeat;
            *repeatDepth -= 1;
        } else {
            *error = "expected \"repeat\" or \"program\"";
            return;
        }

    } else if (input[index] == 'r') {
        // e.g. "repeat 12 times:"
        if (!consumeToken("repeat", input, &index)) {
            *error = "unrecognized command";
            return;
        }
        type = repeat;

        consumeWhitespace(input, &index);
        if (!consumeUInt32(input, &index, &repeatCount)) {
            *error = "expected repeat count";
            return;
        }

        consumeWhitespace(input, &index);
        if (!consumeToken("times", input, &index)) {
            *error = "expected \"times\"";
            return;
        }

        consumeWhitespace(input, &index);
        if (!consumeToken(":", input, &index)) {
            *error = "expected \":\"";
            return;
        }
        if (*repeatDepth == maxRepeatNesting) {
            *error = "repeats nested too deeply";
            return;
        }
        *repeatDepth += 1;

    } else if (input[index] == 's') {
        // e.g. "set channel 3 to 213 us pulses at 15.1 Hz"
        if (!consumeToken("set", input, &index)) {
            *error = "unrecognized command";
            return;
        }
        type = setChannel;

        consumeWhitespace(input, &index);
        if (!consumeToken("channel", input, &index)) {
            *error = "expected \"channel\"";
            return;
        }

        consumeWhitespace(input, &index);
        if (!consumeUInt32(input, &index, &val)) {
            *error = "expected channel number";
            return;
        }
        if (val > numChannels || val == 0) {
            *error = "channel number must be between 1 and 8";
            return;
        }
        channel = val;

        consumeWhitespace(input, &index);
        if (!consumeToken("to", input, &index)) {
            *error = "expected \"to\"";
            return;
        }

        consumeWhitespace(input, &index);
        if (!consumeTime(input, &index, &onTime)) {
            *error = "expected time, e.g. \"2 s\", "
                "\"13 ms\", \"12 us\", or \"15 \u00B5s\"";
            return;
        }

        consumeWhitespace(input, &index);
        if (!consumeToken("pulses", input, &index)) {
            *error = "expected \"pulses\"";
            return;
        }

        consumeWhitespace(input, &index);
        if (!consumeToken("at", input, &index)) {
            *error = "expected \"at\"";
            return;
        }

        consumeWhitespace(input, &index);
        Microseconds period;
        if (!consumeFrequency(input, &index, &period)) {
            *error = "expected frequency, e.g. \"2.3 Hz\" or \"15 kHz\"";
            return;
        }

        offTime = period - onTime;

    } else if (input[index] == 't') {
        // e.g. "turn off channel 4" or "turn on channel 1"
        if (!consumeToken("turn", input, &index)) {
            *error = "unrecognized command";
            return;
        }
        type = setChannel;

        consumeWhitespace(input, &index);
        const char* expectedToken;
        if (input[index] == 'o' && input[index + 1] == 'n') {
            expectedToken = "on";
            onTime = forever;
            offTime = 0;
        } else {
            expectedToken = "off";
            onTime = 0;
            offTime = forever;
        }
        if (!consumeToken(expectedToken, input, &index)) {
            *error = "expected \"on\" or \"off\"";
            return;
        }

        consumeWhitespace(input, &index);
        if (!consumeToken("channel", input, &index)) {
            *error = "expected \"channel\"";
            return;
        }

        consumeWhitespace(input, &index);
        if (!consumeUInt32(input, &index, &val)) {
            *error = "expected channel number";
            return;
        }
        if (val > numChannels || val == 0) {
            *error = "channel number must be between 1 and 8";
            return;
        }
        channel = val;

    } else if (input[index] == 'w') {
        // e.g. "wait 182 us"
        if (!consumeToken("wait", input, &index)) {
            *error = "unrecognized command";
            return;
        }
        type = wait;

        consumeWhitespace(input, &index);
        if (!consumeTime(input, &index, &waitTime)) {
            *error = "expected time, e.g. \"2 s\", "
                "\"13 ms\", \"12 us\", or \"15 \u00B5s\"";
            return;
        }
    } else {
        *error = "unrecognized command";
        return;
    }

    consumeWhitespace(input, &index);
    if (input[index] != 0) {
        *error = "unexpected text found after end of command";
        return;
    }

    *error = NULL;
}


int PulseStateCommand::execute(PulseChannel* channels, RepeatStack* stack,
                int commandId, Microseconds timeInState,
                Microseconds* timeAvailable) {
    switch (type) {
        default:
        case noOp:
            return 1;

        case endProgram:
            *timeAvailable = 0;
            return 0;

        case setChannel:
            channels[channel - 1].setOnOffTime(onTime, offTime);
            return 1;

        case wait:
            if (waitTime > *timeAvailable + timeInState) {
                *timeAvailable = 0;
                return 0;
            } else {
                *timeAvailable -= waitTime - timeInState;
                return 1;
            }

        case endRepeat:
            if (stack->decrementRepeatCount() > 0) {
                // repeat
                return stack->getLoopTarget() - commandId;
            } else {
                // loop completed
                stack->pop();
                return 1;
            }

        case repeat:
            stack->pushRepeat(commandId + 1, repeatCount);
            return 1;
    }
};

