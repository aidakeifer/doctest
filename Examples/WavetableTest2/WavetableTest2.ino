#include "SF2_Decoded_Samples.h" // http://www.freesound.org/people/kiddpark/sounds/201159/
#include <Bounce.h>
#include <AudioSynthWavetable.h>
#include <Audio.h>
#include <Wire.h>
#include <SPI.h>
#include <SD.h>
#include <SerialFlash.h>


const int TOTAL_VOICES = 8;
const int TOTAL_MIXERS = 3;
struct voice_t {
	byte channel;
	byte note;
};
voice_t voices[TOTAL_VOICES];

int allocateVoice(voice_t voice);
int freeVoice(voice_t voice);

AudioControlSGTL5000 sgtl5000_1;
AudioSynthWavetable wavetable[TOTAL_VOICES];
AudioMixer4 mixer[TOTAL_MIXERS];
AudioOutputI2S i2s1;
AudioConnection patchCord[] = {
	{wavetable[0], 0, mixer[0], 0},
	{wavetable[1], 0, mixer[0], 1},
	{wavetable[2], 0, mixer[0], 2},
	{wavetable[3], 0, mixer[0], 3},
	{wavetable[4], 0, mixer[1], 0},
	{wavetable[5], 0, mixer[1], 1},
	{wavetable[6], 0, mixer[1], 2},
	{wavetable[7], 0, mixer[1], 3},
	//{wavetable[7], 0, mixer[2], 0},
	//{wavetable[8], 0, mixer[2], 1},
	//{wavetable[9], 0, mixer[2], 2},
	////{wavetable[11], 0, mixer[2], 3},
	{mixer[0], 0, mixer[2], 0},
	{mixer[1], 0, mixer[2], 1},
	//{mixer[2], 0, mixer[3], 2},
	{mixer[2], 0, i2s1, 0},
	{mixer[2], 0, i2s1, 1},
};

int evict_voice = 0;

void setup() {
	Serial.begin(38400);

	AudioMemory(40);

	sgtl5000_1.enable();
	sgtl5000_1.volume(0.8);

	for (int i = 0; i < TOTAL_VOICES; ++i) {
		mixer[i / 4].gain(i % 4, 1.0);
		wavetable[i].setSamples(samples);
		wavetable[i].amplitude(1.0);
		voices[i].channel = voices[i].note = 0xFF;
	}

	usbMIDI.setHandleNoteOn(OnNoteOn);
	usbMIDI.setHandleNoteOff(OnNoteOff);
}

void printVoices() {
	for (int i = 0; i < TOTAL_VOICES; ++i)
		Serial.printf("%2i c   n ", i);
	Serial.print('\n');
	for (int i = 0; i < TOTAL_VOICES; ++i)
		Serial.printf("%4hhu%4hhu ", voices[i].channel, voices[i].note);
	Serial.print('\n');
}

void OnNoteOn(byte channel, byte note, byte velocity) {
	//Serial.printf("NoteOn: channel==%hhu,note==%hhu\n", channel, note);
	//printVoices();
	int voice_id = allocateVoice(voice_t{ channel, note });
	voices[voice_id].channel = channel;
	voices[voice_id].note = note;
	wavetable[voice_id].playNote(note);
	printVoices();
}

void OnNoteOff(byte channel, byte note, byte velocity) {
	//Serial.printf("NoteOff: channel==%hhu,note==%hhu\n", channel, note);
	//printVoices();

	//for (int i = 0; i < TOTAL_VOICES; ++i)
	//	wavetable[i].stop();
	int voice_id = freeVoice(voice_t{ channel, note });
	if (voice_id == TOTAL_VOICES) return;

	voices[voice_id].note = 0xFF;
	wavetable[voice_id].stop();
	printVoices();
}

void loop() {
	usbMIDI.read();
}

int allocateVoice(voice_t voice) {
	int i;

	//find matching channel/note
	for (i = 0; i < TOTAL_VOICES; ++i)
		if (voices[i].channel == voice.channel && voices[i].note == voice.note)
			break;

	//find matching channel, non-playing note (i.e. prevent resetting sample)
	if (i == TOTAL_VOICES) for (i = 0; i < TOTAL_VOICES; ++i)
		if (voices[i].channel == voice.channel && voices[i].note == 0xFF)
			break;

	//find non-playing note
	if (i == TOTAL_VOICES) for (i = 0; i < TOTAL_VOICES; ++i)
		if (voices[i].note == 0xFF)
			break;

	//else choose evict idx
	i = i == TOTAL_VOICES ? evict_voice : i;

	//possibly iterate evict so it doesn't match what we're now assigning
	evict_voice = i == evict_voice ? evict_voice + 1 : evict_voice;

	//loop evict idx
	evict_voice = evict_voice == TOTAL_VOICES ? 0 : evict_voice;

	return i;
}

int freeVoice(voice_t voice) {
	int i;

	//find matching channel/note
	for (i = 0; i < TOTAL_VOICES; ++i)
		if (voices[i].channel == voice.channel && voices[i].note == voice.note)
			break;

	return i;
}