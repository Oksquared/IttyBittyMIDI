/**
This code receives a MIDI note and then plays the MIDI file that corresponds with the note it received.
example: if noteON is received for note 2 over the MIDI jack, the Teensy reads the MIDI file called "2.MID" and drives the outputs on for X milliseconds when it reads a noteOn in the MIDI file.
**/
#include "MD_MIDIFile.h"
#include <MIDI.h>
#include <Audio.h>
#include <Wire.h>
//#include <SPI.h>
#include <SD.h>
//#include <SerialFlash.h>
MIDI_CREATE_INSTANCE(HardwareSerial, Serial1, MIDI);
int lowEndNote = 49;
const uint32_t pinOnTimeMs = 20;
const int numIOPins = 16;
const int ioPins[] = { 2, 3, 4, 5, 6, 7, 8, 9, 33, 36, 37, 14, 15, 18, 19, 22 };
bool ioPinStates[numIOPins];
uint32_t ioPinTimes[numIOPins];
const int highEndNote = lowEndNote + numIOPins;
int offsetNote, note;

#define DBG_SERIAL Serial

#define USE_MIDI 0  // set to 1 to enable MIDI output, otherwise debug output

#if USE_MIDI  // set up for direct MIDI serial output

#define DEBUG(x)
#define DEBUGX(x)
#define DEBUGS(s)
#define SERIAL_RATE 31250

#else  // don't use MIDI to allow printing debug statements

#define DEBUG(x) Serial.print(x)
#define DEBUGX(x) Serial.print(x, HEX)
#define DEBUGS(s) Serial.print(F(s))
#define SERIAL_RATE 57600

#endif  // USE_MIDI

const int ledPin = 13;
#define ARRAY_SIZE(a) (sizeof(a) / sizeof(a[0]))

MD_MIDIFile SMF;

const char *tuneList[] = {
  "1.MID"  // simplest and shortest file
  "2.MID"
  "3.MID"
  "4.MID"
};

const uint16_t WAIT_DELAY = 2000;  // ms

void printDirectory(File dir, File prevDir);
void printFiles(File dir);

void setup() {



  for (int i = 0; i < numIOPins; i++) {
    pinMode(ioPins[i], OUTPUT);
  }
  MIDI.begin(MIDI_CHANNEL_OMNI);
  Serial.begin(57600);
  Serial.println("MIDI Input Test");

  SMF.begin(&(SdFat &)SD);

  DBG_SERIAL.begin(115200);
  unsigned long ms = millis();
  while (!DBG_SERIAL) {
    if ((millis() - ms) > 10000) break;
  }
  Serial.println("starting stuff...");
  //-------------------------------
  AudioMemory(50);  // no idea what we'll need, so allow plenty
  //-------------------------------
  /* if (CrashReport && DBG_SERIAL)
    {
        DBG_SERIAL.println(CrashReport);
        CrashReport.clear();
    }*/

  while (!(SD.begin(BUILTIN_SDCARD))) {
    Serial.println("Unable to access the SD card");
    delay(500);
  }
  File root = SD.open("/");

  Serial.println("----------------\nFiles:");
  printFiles(root);
  Serial.println("----------------");
  root.close();

  SMF.setMidiHandler(midiCallback);
  SMF.setSysexHandler(sysexCallback);

  //Serial.printf("midi file load: %d", SMF.load("furelise.mid"));
}
void printFiles(File dir) {
  while (true) {
    File entry = dir.openNextFile();
    if (!entry) {
      // no more files
      break;
    }
    Serial.print(entry.name());
    if (entry.isDirectory()) {
      Serial.println("/");
    } else {
      // files have sizes, directories do not
      Serial.print("\t\t");
      Serial.println(entry.size(), DEC);
    }
    entry.close();
  }
}

void printDirectory(File dir, int numTabs) {
  while (true) {
    File entry = dir.openNextFile();
    if (!entry) {
      // no more files
      break;
    }
    for (uint8_t i = 0; i < numTabs; i++) {
      Serial.print('\t');
    }
    Serial.print(entry.name());
    if (entry.isDirectory()) {
      Serial.println("/");
      printDirectory(entry, numTabs + 1);
    } else {
      // files have sizes, directories do not
      Serial.print("\t\t");
      Serial.println(entry.size(), DEC);
    }
    entry.close();
  }
}

void tickMetronome(void)
// flash a LED to the beat
{
  static uint32_t lastBeatTime = 0;
  static boolean inBeat = false;
  uint16_t beatTime;

  beatTime = 60000 / SMF.getTempo();  // msec/beat = ((60sec/min)*(1000 ms/sec))/(beats/min)
  if (!inBeat) {
    if ((millis() - lastBeatTime) >= beatTime) {
      lastBeatTime = millis();
      digitalWrite(ledPin, HIGH);
      inBeat = true;
    }
  } else {
    if ((millis() - lastBeatTime) >= 100)  // keep the flash on for 100ms only
    {
      digitalWrite(ledPin, LOW);
      inBeat = false;
    }
  }
}

void blinkLedTask(void) {
  static int ledState = LOW;                // ledState used to set the LED
  static unsigned long previousMillis = 0;  // will store last time LED was updated
  static unsigned long currentMillis = 0;
  static unsigned long currentInterval = 0;
  static unsigned long ledBlinkOnInterval = 100;
  static unsigned long ledBlinkOffInterval = 2000;

  currentMillis = millis();
  currentInterval = currentMillis - previousMillis;

  if (ledState == LOW) {
    if (currentInterval > ledBlinkOffInterval) {
      previousMillis = currentMillis;
      ledState = HIGH;
      digitalWrite(ledPin, HIGH);
    }
  } else {
    if (currentInterval > ledBlinkOnInterval) {
      previousMillis = currentMillis;
      ledState = LOW;
      digitalWrite(ledPin, LOW);
    }
  }
}

void midiCallback(midi_event *pev)
// Called by the MIDIFile library when a file event needs to be processed
// thru the midi communications interface.
// This callback is set up in the setup() function.
{
#if USE_MIDI
  if ((pev->data[0] >= 0x80) && (pev->data[0] <= 0xe0)) {
    //Serial.write(pev->data[0] | pev->channel);
    //Serial.write(&pev->data[1], pev->size-1);

    usbMIDI.send(pev->data[0], pev->data[1], pev->data[2], pev->channel, 0);
  }
  //else
  //Serial.write(pev->data, pev->size);
#else
  // DEBUG("\n");
  // DEBUG(millis());
  // DEBUG("\tM T");
  // DEBUG(pev->track);
  // DEBUG(":  Ch ");
  // DEBUG(pev->channel + 1);
  // DEBUG(" Data ");
  for (uint8_t i = 0; i < pev->size; i++) {
    // DEBUGX(pev->data[i]);
    //DEBUG(' ');
  }
  //Serial.println();
  //Serial.println((pev->data[0]) >> 4);
  switch (pev->data[0] >> 4) {
    case 9:  //noteON
      note = pev->data[1];
      // Serial.println("On");
      // Serial.println(pev->data[1]);
      offsetNote = note - lowEndNote;
      ioPinTimes[note - lowEndNote] = millis();  // capture time note was pressed
      digitalWrite(ioPins[offsetNote], HIGH);
      // Serial.println(offsetNote);
      break;
    case 8:  //noteOff
      //Serial.println("Off");
      break;
    default:
      // statements
      break;
  }
#endif
}

void sysexCallback(sysex_event *pev)
// Called by the MIDIFile library when a system Exclusive (sysex) file event needs
// to be processed through the midi communications interface. Most sysex events cannot
// really be processed, so we just ignore it here.
// This callback is set up in the setup() function.
{
  DEBUG("\nS T");
  DEBUG(pev->track);
  DEBUG(": Data ");
  for (uint8_t i = 0; i < pev->size; i++) {
    DEBUGX(pev->data[i]);
    DEBUG(' ');
  }
}

void midiSilence(void)
// Turn everything off on every channel.
// Some midi files are badly behaved and leave notes hanging, so between songs turn
// off all the notes and sound
{
  midi_event ev;

  // All sound off
  // When All Sound Off is received all oscillators will turn off, and their volume
  // envelopes are set to zero as soon as possible.
  ev.size = 0;
  ev.data[ev.size++] = 0xb0;
  ev.data[ev.size++] = 120;
  ev.data[ev.size++] = 0;

  for (ev.channel = 0; ev.channel < 16; ev.channel++)
    midiCallback(&ev);
}

// work with SLIP-protocol serial port:
unsigned long t = 0;
int playingNote;
int freq;
static int16_t currTune = ARRAY_SIZE(tuneList);
void loop() {
  int type, MIDInote, velocity, channel, d1, d2;
  //blinkLedTask();
  if (MIDI.read()) {  // Is there a MIDI message incoming ?
    byte type = MIDI.getType();
    switch (type) {
      case midi::NoteOn:
        note = MIDI.getData1();
        velocity = MIDI.getData2();
        channel = MIDI.getChannel();
        if (velocity > 0) {
          Serial.println(String("Note On:  ch=") + channel + ", MIDInote=" + MIDInote + ", velocity=" + velocity);
          currTune = MIDInote;
          break;
          case midi::NoteOff:
            note = MIDI.getData1();
            velocity = MIDI.getData2();
            channel = MIDI.getChannel();
            Serial.println(String("Note Off: ch=") + channel + ", MIDInote=" + MIDInote + ", velocity=" + velocity);
            break;
          default:
            d1 = MIDI.getData1();
            d2 = MIDI.getData2();
            Serial.println(String("Message, type=") + type + ", data = " + d1 + " " + d2);
        }
        t = millis();
    }
    if (millis() - t > 10000) {
      t += 10000;
      Serial.println("(inactivity)");
    }
  }
  for (int i = 0; i < numIOPins; i++) {
    if ((ioPinTimes[i] + pinOnTimeMs) < millis()) {  // if pin has been energized for pinOnTimeMs, turn off
      digitalWrite(ioPins[i], LOW);
    }
  }


  static enum { S_HALT,
                S_IDLE,
                S_PLAYING,
                S_END,
                S_WAIT_BETWEEN } state = S_IDLE;

  static uint32_t timeStart;

  switch (state) {
    case S_HALT:
      {
        Serial.println();
        Serial.println(currTune);
        if (currTune != -1) {
          state = S_IDLE;
        }
      }
      break;
    case S_IDLE:  // now idle, set up the next tune
      {
        int err;

        DEBUGS("\nS_IDLE");

        //digitalWrite(READY_LED, LOW);
        //digitalWrite(SMF_ERROR_LED, LOW);


        if (currTune >= ARRAY_SIZE(tuneList))
          currTune = 0;

        // use the next file name and play it
        DEBUG("\nFile: ");
        DEBUG(tuneList[currTune]);
        err = SMF.load(tuneList[currTune]);
        if (err != MD_MIDIFile::E_OK) {
          DEBUG(" - SMF load Error ");
          DEBUG(err);
          //digitalWrite(SMF_ERROR_LED, HIGH);
          timeStart = millis();
          state = S_WAIT_BETWEEN;
          DEBUGS("\nWAIT_BETWEEN");
        } else {
          DEBUGS("\nS_PLAYING");
          state = S_PLAYING;
        }
      }
      break;

    case S_PLAYING:  // play the file

      if (!SMF.isEOF()) {
        if (SMF.getNextEvent()) {
          //DEBUGS("\nS_PLAYING");
          tickMetronome();
        }
      } else
        state = S_END;
      break;

    case S_END:  // done with this one
      DEBUGS("\nS_END");
      SMF.close();
      midiSilence();
      timeStart = millis();
      state = S_HALT;
      DEBUGS("\nS_HALT");
      currTune = -1;
      break;

    case S_WAIT_BETWEEN:  // signal finished with a dignified pause
      //digitalWrite(READY_LED, HIGH);
      if (millis() - timeStart >= WAIT_DELAY)
        state = S_HALT;
      break;

    default:
      state = S_IDLE;
      break;
  }
}