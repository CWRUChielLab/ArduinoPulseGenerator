#define NUM_CHANNELS 4
#define TIMER1_PIN 12

// the activity of a single channel during a pulse sequence
struct ChannelActivity {
    int16_t pulsePeriod;  // if this is zero, the channel will not be modified
    int16_t pulseDuration;    
};

// A block of time where all of the channels are generating pulses with 
// a fixed width and fixed period.
struct PulseBlock {
    ChannelActivity channelActivity[NUM_CHANNELS];
    uint32_t numPulses;     // number of pulses before switching
    int16_t nextPulseBlock; // index of which block to trigger next
    int8_t timingChannel;   // channel to use for counting the pulses
};



void setup() {
  pinMode(TIMER1_PIN, OUTPUT);
  
  // set the period to 48 clocks (~= 3 ms)
  OCR1A = 48;
  
  // Turn off the pulse after 16 clocks (~= 1 ms)
  OCR1B = 16;
  
  // COM1B = 2: Turn on OC1B pulse at the begining of the period, off when timer reaches
  // WGM1 = 15: Count up to ORCA1 then reset
  // CS1 = 5: Increment timer every 1024 system clocks (e.g. 16 MHz / 1024)
  TCCR1A = _BV(COM1B1) | _BV(WGM11) | _BV(WGM10); 
  TCCR1B = _BV(WGM13) | _BV(WGM12) | _BV(CS12) | _BV(CS10);  
}

void loop() {
  // just let it run for now
}
