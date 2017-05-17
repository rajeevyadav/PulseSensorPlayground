/*
   A central Playground object to manage the PulseSensor.
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
#include <PulseSensorPlayground.h>

PulseSensorPlayground::PulseSensorPlayground(int numberOfSensors) {
  // Save a static pointer to our playground so the ISR can read it.
  OurThis = this;

  // By default, we attempt to use interrupts to sample.
  UsingInterrupts = true;

  // Dynamically create the array to minimize ram usage.
  SensorCount = (byte) numberOfSensors;
  Sensors = new PulseSensor[SensorCount];
}

boolean PulseSensorPlayground::PulseSensorPlayground::begin() {

  // Note the time, for non-interrupt sampling and for timing statistics.
  NextSampleMicros = micros() + MICROS_PER_READ;
  
  SawNewSample = false;

  // Lastly, set up and turn on the interrupts.
  
  if (UsingInterrupts) {
    setupInterrupt();
  }


  return true;
}

void PulseSensorPlayground::analogInput(int inputPin, int sensorIndex) {
  if (sensorIndex != constrain(sensorIndex, 0, SensorCount)) {
    return; // out of range.
  }
  Sensors[sensorIndex].analogInput(inputPin);
}

void PulseSensorPlayground::blinkOnPulse(int blinkPin, int sensorIndex) {
  if (sensorIndex != constrain(sensorIndex, 0, SensorCount)) {
    return; // out of range.
  }
  Sensors[sensorIndex].blinkOnPulse(blinkPin);
}

void PulseSensorPlayground::fadeOnPulse(int fadePin, int sensorIndex) {
  if (sensorIndex != constrain(sensorIndex, 0, SensorCount)) {
    return; // out of range.
  }
  Sensors[sensorIndex].fadeOnPulse(fadePin);
}

boolean PulseSensorPlayground::sawNewSample() { 
  /*
     If using interrupts, this function reads and clears the
     'saw a sample' flag that is set by the ISR.

     When not using interrupts, this function sees whether it's time
     to sample and, if so, reads the sample and processes it.
  */
  
  if (UsingInterrupts) {
    // Disable interrupts to avoid a race with the ISR.
    cli();
    boolean sawOne = SawNewSample;
    SawNewSample = false;
    sei();
    
    return sawOne;
  }

  // Not using interrupts
  
  unsigned long nowMicros = micros();
  if ((long) (NextSampleMicros - nowMicros) > 0L) {
    return false;  // not time yet.
  }
  NextSampleMicros = nowMicros + MICROS_PER_READ;

  // Read signals from all pulse sensors.
  // To minimize jitter, we read all samples first, then process them.
  for (int i = 0; i < SensorCount; ++i) {
    Sensors[i].readNextSample();
  }
  for (int i = 0; i < SensorCount; ++i) {
    Sensors[i].processLatestSample();
  }

  return true;
}

int PulseSensorPlayground::getLatestSample(int sensorIndex) {
  if (sensorIndex != constrain(sensorIndex, 0, SensorCount)) {
    return -1; // out of range.
  }
  return Sensors[sensorIndex].getLatestSample();
}

int PulseSensorPlayground::getBeatsPerMinute(int sensorIndex) {
  if (sensorIndex != constrain(sensorIndex, 0, SensorCount)) {
    return -1; // out of range.
  }
  return Sensors[sensorIndex].getBeatsPerMinute(); 
}

int PulseSensorPlayground::getInterBeatIntervalMs(int sensorIndex) {
  if (sensorIndex != constrain(sensorIndex, 0, SensorCount)) {
    return -1; // out of range.
  }
  return Sensors[sensorIndex].getInterBeatIntervalMs(); 
}

boolean PulseSensorPlayground::sawStartOfBeat(int sensorIndex) {
  if (sensorIndex != constrain(sensorIndex, 0, SensorCount)) {
    return false; // out of range.
  }
  return Sensors[sensorIndex].sawStartOfBeat(); 
}

boolean PulseSensorPlayground::isInsideBeat(int sensorIndex) {
  if (sensorIndex != constrain(sensorIndex, 0, SensorCount)) {
    return false; // out of range.
  }
  return Sensors[sensorIndex].isInsideBeat();
}

void PulseSensorPlayground::setOutputType(byte outputType) {
  SerialOutput.setOutputType(outputType);
}

void PulseSensorPlayground::outputSample() {
  SerialOutput.outputSample(Sensors, SensorCount);
}

void PulseSensorPlayground::outputBeat() {
  SerialOutput.outputBeat(Sensors, SensorCount);
}

void PulseSensorPlayground::setupInterrupt() {
  //TODO: attachInterrupt(digitalPinToInterrupt(pin), ISR, mode);	?
  // Initializes Timer2 to throw an interrupt every 2mS.
  TCCR2A = 0x02;     // DISABLE PWM ON DIGITAL PINS 3 AND 11, AND GO INTO CTC MODE
  TCCR2B = 0x06;     // DON'T FORCE COMPARE, 256 PRESCALER
  OCR2A = 0X7C;      // SET THE TOP OF THE COUNT TO 124 FOR 500Hz SAMPLE RATE
  TIMSK2 = 0x02;     // ENABLE INTERRUPT ON MATCH BETWEEN TIMER2 AND OCR2A
  sei();             // MAKE SURE GLOBAL INTERRUPTS ARE ENABLED
}

// THIS IS THE TIMER 2 INTERRUPT SERVICE ROUTINE.
// Timer 2 makes sure that we take a reading every 2 miliseconds
ISR(TIMER2_COMPA_vect) {                   // triggered when Timer2 counts to 124
  cli();                                   // disable interrupts while we do this

  /*
     Read the voltage from each PulseSensor.
     We do this separately from processing the voltages
     to minimize jitter in acquiring the signal.
  */
  for (int i = 0; i < PulseSensorPlayground::OurThis->SensorCount; ++i) {
    PulseSensorPlayground::OurThis->Sensors[i].readNextSample();
  }

  // Process those voltages.
  for (int i = 0; i < PulseSensorPlayground::OurThis->SensorCount; ++i) {
    PulseSensorPlayground::OurThis->Sensors[i].processLatestSample();
    PulseSensorPlayground::OurThis->Sensors[i].updateLEDs();
  }

  // Set the flag that says we've read a sample since the Sketch checked.
  PulseSensorPlayground::OurThis->SawNewSample = true;
  
  sei();                                   // enable interrupts when youre done

}// end isr
