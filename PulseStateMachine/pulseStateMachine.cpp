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
            input[*index + i] == '\n' || input[*index + i] == '\r') {
        i++;
    }

    *index += i;
}


static bool consumeUInt32(const char* input, int* index, uint32_t* result) {
    uint32_t val = 0;

    if (!(input[*index] >= '0' && input[*index] <= '9')) {
        return false;
    }

    while (input[*index] >= '0' && input[*index] <= '9') {
        val = 10 * val + input[*index] - '0';
        (*index)++;
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


void PulseStateCommand::parseFromString(const char* input, const char** error) {
    int index = 0;
    uint32_t val;

    consumeWhitespace(input, &index);
    if (input[index] == 'e') {
        // e.g. "end"
        if (!consumeToken("end", input, &index)) {
            *error = "unrecognized command";
            return;
        }
        type = end;


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
            *error = "Channel number must be between 1 and 4";
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
            *error = "Channel number must be between 1 and 4";
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

    *error = NULL;
}


bool PulseStateCommand::execute(PulseChannel* channels,
        Microseconds timeInState, Microseconds* timeAvailable) {
    switch (type) {
        default:
        case end:
            *timeAvailable = 0;
            return false;

        case setChannel:
            channels[channel - 1].setOnOffTime(onTime, offTime);
            return true;

        case wait:
            if (waitTime > *timeAvailable + timeInState) {
                *timeAvailable = 0;
                return false;
            } else {
                *timeAvailable -= waitTime - timeInState;
                return true;
            }
    }
};

