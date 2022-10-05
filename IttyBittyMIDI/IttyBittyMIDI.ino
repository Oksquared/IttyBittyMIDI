#include <MIDI.h>

/**
This Sketch turns a pin on for "pinOnTimeMs" when a NoteOn signal is received
 after that it turns the pin off

**/

int lowEndNote = 48;
const uint32_t pinOnTimeMs = 20;





MIDI_CREATE_INSTANCE(HardwareSerial, Serial1, MIDI);
const int numIOPins = 16;
const int ioPins[] = { 2, 3, 4, 5, 6, 7, 8, 9, 33, 36, 37, 14, 15, 18, 19, 22 };
bool ioPinStates[numIOPins];
uint32_t ioPinTimes[numIOPins];
const int highEndNote = lowEndNote + numIOPins;


void setup() {
  for (int i = 0; i < numIOPins; i++) {
    pinMode(ioPins[i], OUTPUT);
  }
  MIDI.begin(MIDI_CHANNEL_OMNI);
  Serial.begin(57600);
  Serial.println("MIDI Input Test");
}

unsigned long t = 0;
int playingNote;
int freq;
void loop() {
  int type, note, velocity, channel, d1, d2, offsetNote;


  for (int i = 0; i < numIOPins; i++) {
    if ((ioPinTimes[i] + pinOnTimeMs) < millis()) {  // if pin has been energized for pinOnTimeMs, turn off
      digitalWrite(ioPins[i], LOW);
    }
  }

  if (MIDI.read()) {  // Is there a MIDI message incoming ?
    byte type = MIDI.getType();
    switch (type) {
      case midi::NoteOn:
        note = MIDI.getData1();
        offsetNote = note - lowEndNote;
        velocity = MIDI.getData2();
        channel = MIDI.getChannel();
        if (velocity > 0) {
          Serial.println(String("Note On:  ch=") + channel + ", note=" + note + ", velocity=" + velocity);
          if (note <= highEndNote && note >= lowEndNote) {  // check if note in range so we dont break shit
            ioPinTimes[note - lowEndNote] = millis();       // capture time note was pressed
            digitalWrite(ioPins[offsetNote], HIGH);         // drive pin aligned to note, HIGH

            Serial.print("output: ");
            Serial.println(offsetNote);
            
          } else {
            Serial.println(String("Note Off: ch=") + channel + ", note=" + note);
          }
          break;
          case midi::NoteOff:
            note = MIDI.getData1();
            velocity = MIDI.getData2();
            channel = MIDI.getChannel();
            Serial.println(String("Note Off: ch=") + channel + ", note=" + note + ", velocity=" + velocity);
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
}
void ioStates() {
  for (int i = 0; i < numIOPins; i++) {
    if (ioPinStates[i] == 1) {                       // if pin is on
      if (ioPinTimes[i] > millis() + pinOnTimeMs) {  // should turn pin off
        digitalWrite(ioPins[i], LOW);
      }
    }
  }
}