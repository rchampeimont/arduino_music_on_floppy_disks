// Copyright (c) 2021 Raphael Champeimont

// Code designed for Arduino Uno R3

//const unsigned long SAMPLING_PERIOD = 91; // period = 91 us <=> freq = 11025 Hz
const unsigned long SAMPLING_PERIOD = 125; // period = 125 us <=> freq = 8000 Hz

// Uses pins OUTPUT_FIRST_PIN to OUTPUT_FIRST_PIN + 7.
// OUTPUT_FIRST_PIN is the least significant bit
const int OUTPUT_FIRST_PIN = 6;

const int MISSING_DATA_LED_PIN = A0;
byte lastMissingDataLEDState = HIGH;

const int TOO_MUCH_DATA_LED_PIN = A1;
byte lastTooMuchDataLEDState = LOW;

const int ASK_FOR_DATA_PIN = A2;
byte askForDataState = HIGH;

const int OK_LED_PIN = A3;
unsigned long lastIssueTime = micros();

const int LOST_SOUND_SYNC_LED = A4;
unsigned long lastLostSyncTime = micros();

const int BUFFER_SIZE = 1792;

// How many bytes we reach per batch. Must be a divider of BUFFER_SIZE.
// This must be too high, otherwise we will lock the loop for too long and will be late to send sample to DAC.
const int SERIAL_BATCH_SIZE = 8;

// Tell sender to send more data when there is this amount of bytes free in buffer.
const int RELOAD_BUFFER_ABOVE_AVAILABLE_SPACE = 128;

// Switch on LED to say "everything is OK" if everything has been fine for this time
const unsigned long DELAY_FOR_LED_RESET = 1000000; // in microseconds
const unsigned int ACCPETABLE_PERIOD_SHIFT_PERCENT = 200; // % of error (100 = no error tolerance at all)

byte buffer[BUFFER_SIZE];
int readPointer = 0;
int writePointer = 0;

unsigned long lastPlayedValueTime = micros();
unsigned long lastIdealSampleTime = micros();

// true if next sample to play is the 4 LSBs / false if next sample to play is the 4 MSBs
bool nextSampleIsLowerBits = false;

// cache for micros()
unsigned long now = micros();

void setup() {
  // Speed at which we receive WAV sound data
  Serial.begin(1E6, SERIAL_8E1);

  for (int pin = OUTPUT_FIRST_PIN; pin < OUTPUT_FIRST_PIN + 8; pin++) {
    pinMode(pin, OUTPUT);
  }

  pinMode(MISSING_DATA_LED_PIN, OUTPUT);
  pinMode(TOO_MUCH_DATA_LED_PIN, OUTPUT);
  pinMode(ASK_FOR_DATA_PIN, OUTPUT);
  pinMode(OK_LED_PIN, OUTPUT);
  pinMode(LOST_SOUND_SYNC_LED, OUTPUT);
}


void loop() {
  // Compute available buffer space
  int availableSpace;
  if (writePointer < readPointer) {
    availableSpace = readPointer - writePointer - 1;
  } else {
    availableSpace = readPointer + BUFFER_SIZE - writePointer - 1;
  }

  // Tell sender if we want to ask sender Arduino for more data
  if (availableSpace > RELOAD_BUFFER_ABOVE_AVAILABLE_SPACE) {
    askForDataState = HIGH;
  } else {
    askForDataState = LOW;
  }

  // Set all LEDs and send "ask for data" state to sender Arduino
  PORTC = (lastMissingDataLEDState                            ? B00000001 : 0) // missing data LED
          | (lastTooMuchDataLEDState                          ? B00000010 : 0) // too much data LED
          | (askForDataState                                  ? B00000100 : 0) // ask more data LED+wire
          | ((now > lastIssueTime + DELAY_FOR_LED_RESET)      ? B00001000 : 0) // OK LED
          | ((now < lastLostSyncTime + DELAY_FOR_LED_RESET)   ? B00010000 : 0); // lost sync LED

  // Receive data
  byte availableBytes = Serial.available();
  if (availableBytes >= SERIAL_BATCH_SIZE) {
    if (availableSpace >= SERIAL_BATCH_SIZE) {
      // Receive data through serial port
      Serial.readBytes(buffer + writePointer, SERIAL_BATCH_SIZE);
      // Increment write pointer
      writePointer += SERIAL_BATCH_SIZE;
      if (writePointer >= BUFFER_SIZE) {
        writePointer = 0;
      }
      // Report that we don't have too much data
      lastTooMuchDataLEDState = LOW;
    } else {
      // We cannot read new data because there is not enough space in buffer to receive one batch
      lastIssueTime = now;
      lastTooMuchDataLEDState = HIGH;
    }
  }

  // Send data to DAC to play
  now = micros();
  if (now > lastIdealSampleTime + SAMPLING_PERIOD) {
    if (readPointer == writePointer) {
      // We need to wait to receive some more data.
      // Report issue by switching on red LED
      lastIssueTime = now;
      lastMissingDataLEDState = HIGH;
    } else {
      // We have data to read, so play it.
      lastMissingDataLEDState = LOW;

      byte valueToPlay;
      // For each byte we receive, composed of bits "abcdefgh",
      // we play first "abcd" as a 4-bit sample value,
      // then "efgh" as a second 4-bit sample value.
      if (nextSampleIsLowerBits) {
        // Read the 4 least significant bits transmitted and play them as a 4-bit sample
        valueToPlay = buffer[readPointer] << 4;
      } else {
        // Read the 4 most significant bits transmitted and play them as a 4-bit sample
        valueToPlay = buffer[readPointer] & 0xf0;
      }

      // incomment to visualize buffer usage on oscilloscope plugged on DAC output
      //valueToPlay = (BUFFER_SIZE - availableSpace)*255.0/BUFFER_SIZE;

      // Send new value to DAC
      PORTB = valueToPlay >> 2; // sets pins 8 to 13 to the 6 most significant bits
      // with this optimized version, least significant bit changes 0.4 to 0.7 microsecond after PORTB
      byte shiftedByte = valueToPlay << 6;
      PORTD |= shiftedByte & B11000000; // set pins 6 and 7 to 1 if needed
      PORTD &= shiftedByte | B00111111; // set pins 6 and 7 to 0 if needed

      if (now > lastIdealSampleTime + 10*SAMPLING_PERIOD) {
        // We are very late, so let's reset the ideal value time to now
        lastIdealSampleTime = now;
        lastLostSyncTime = now;
        lastIssueTime = now;
      } else {
        // We are in acceptable sync.
        // This is more precise that setting to "now" because it avoids accumulated shift which slows down the music.
        lastIdealSampleTime += SAMPLING_PERIOD;

        if (now > lastPlayedValueTime + SAMPLING_PERIOD*ACCPETABLE_PERIOD_SHIFT_PERCENT/100) {
          // We are a bit out of sync, so report that not everything is OK on LED
          lastIssueTime = now;
        }
      }

      lastPlayedValueTime = now;

      nextSampleIsLowerBits = ! nextSampleIsLowerBits;
      if (! nextSampleIsLowerBits) {
        readPointer++;
        if (readPointer >= BUFFER_SIZE) {
          readPointer = 0;
        }
      }
    }
  }
}
