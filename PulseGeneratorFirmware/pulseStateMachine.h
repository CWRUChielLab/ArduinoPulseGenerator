#ifndef PULSESTATEMACHINE_H
#define PULSESTATEMACHINE_H
#include <stdint.h>

typedef uint32_t Microseconds;
const Microseconds forever = 0xFFFFFFFF;


class PulseChannel {
    private:
        enum { numStates=2 };
        bool m_on;
        Microseconds m_stateTime[numStates];
        Microseconds m_timeInState;

    public:
        PulseChannel();
        bool on() const { return m_on; }
        Microseconds onTime() const { return m_stateTime[true]; }
        Microseconds offTime() const { return m_stateTime[false]; }
        Microseconds setOnOffTime(Microseconds on, Microseconds off);
        bool advanceTime(Microseconds dt);
};


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
        PulseStateCommand() : type(end) {};
        void parseFromString(const char* input, const char** error);
};

#endif /* PULSESTATEMACHINE_H */
