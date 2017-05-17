/*
   PulseSensor measurement manager.
   See https://www.pulsesensor.com to get started.

   Copyright World Famous Electronics LLC - see LICENSE
   Contributors:
     Joel Murphy, https://pulsesensor.com
     Yury Gitman, https://pulsesensor.com
     Bradford Needham, @bneedhamia, https://bluepapertech.com

   Licensed under the MIT License, a copy of which
   should have been included with this software.

   This software is not intended for medical use.
*/
#ifndef PULSE_SENSOR_H
#define PULSE_SENSOR_H

#include <Arduino.h>

class PulseSensor {
  public:
    // Constructs a PulseSensor manager using a default configuration.
    PulseSensor();

    // Sets the analog input pin this PulseSensor is connected to.
    void analogInput(int inputPin);

    // Configures to blink the given pin while inside a pulse.
    void blinkOnPulse(int blinkPin);

    // Configures to fade the given, starting on start of pulse.
    void fadeOnPulse(int fadePin);

    // Returns the sample most recently-read from this PulseSensor.
    int getLatestSample();

    // Returns the latest beats-per-minute measurement on this PulseSensor.
    int getBeatsPerMinute();

    // Returns the latest inter-beat interval (milliseconds) on this PulseSensor.
    int getInterBeatIntervalMs();

    // Reads and clears the 'saw start of beat' flag, "QS".
    boolean sawStartOfBeat();

    // Returns true if this PulseSensor signal is inside a beat vs. outside.
    boolean isInsideBeat();

    // (internal to the library) Read a sample from this PulseSensor.
    void readNextSample();

    // (internal to the library) Process the latest sample.
    void processLatestSample();
    
    // (internal to the library) Update the Blink and Fade LED states.
    void updateLEDs();

  private:
    // Configuration
    volatile int InputPin;           // Analog input pin for PulseSensor.
    volatile int BlinkPin;           // pin to blink in beat, or -1.
    volatile int FadePin;            // pin to fade on beat, or -1.
    
    // Pulse detection output variables.
    // Volatile because our pulse detection code could be called from an Interrupt
    volatile int BPM;                // int that holds raw Analog in 0. updated every call to readSensor()
    volatile int Signal;             // holds the latest incoming raw data (0..1023)
    volatile int IBI;                // int that holds the time interval (ms) between beats! Must be seeded!
    volatile boolean Pulse;          // "True" when User's live heartbeat is detected. "False" when not a "live beat".
    volatile boolean QS;             // The start of beat has been detected and not read by the Sketch.
    
    // Set in the ISR
    volatile int FadePWM;            // (0..255) current brightness of FadePin.

    // Variables internal to the pulse detection algorithm.
    // Not volatile because we use them only in the pulse detection function.
    unsigned long sampleIntervalMs;  // expected time between calls to readSensor(), in milliseconds.
    int rate[10];                    // array to hold last ten IBI values (ms)
    unsigned long sampleCounter;     // used to determine pulse timing. Milliseconds since we started.
    unsigned long lastBeatTime;      // used to find IBI. Time (sampleCounter) of the previous detected beat start.
    int P;                           // used to find peak in pulse wave, seeded (sample value)
    int T;                           // used to find trough in pulse wave, seeded (sample value)
    int thresh;                      // used to find instant moment of heart beat, seeded (sample value)
    int amp;                         // used to hold amplitude of pulse waveform, seeded (sample value)
    boolean firstBeat;               // used to seed rate array so we startup with reasonable BPM
    boolean secondBeat;              // used to seed rate array so we startup with reasonable BPM
};
#endif // PULSE_SENSOR_H
