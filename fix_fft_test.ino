/*****

	Whistle-to-Serial MIDI converter

	Copyright (C) 2008-2012 Bill Roy

	Permission is hereby granted, free of charge, to any person
	obtaining a copy of this software and associated documentation
	files (the "Software"), to deal in the Software without
	restriction, including without limitation the rights to use,
	copy, modify, merge, publish, distribute, sublicense, and/or sell
	copies of the Software, and to permit persons to whom the
	Software is furnished to do so, subject to the following
	conditions:
	
	The above copyright notice and this permission notice shall be
	included in all copies or substantial portions of the Software.
	
	THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
	EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
	OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
	NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
	HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
	WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
	FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
	OTHER DEALINGS IN THE SOFTWARE.
*****/

// Note: requires fix_fft library from http://www.arduino.cc/cgi-bin/yabb2/YaBB.pl?num=1286718155
#include "fix_fft.h"
#define pin_adc 0

#define NUMBINS 128
char im[NUMBINS];
char data[NUMBINS];

#define SAMPLE_THRESHOLD 10
int max_amplitude;
int max_sample;

int sample_value(int i) {
	return data[i] * data[i] + im[i] * im[i];
}

// bin 16 maps nicely to MIDI note 72 (C5)
#define MIDI_OFFSET (72-16)

#define MIDI_VELOCITY 126
byte playing;
int current_note;


void sendnoteon(byte note, byte velocity) {
	Serial.print(0x90);
	Serial.print(note);
	Serial.print(velocity);
}

void sendnoteoff(int note) {
	sendnoteon(note, 0);
}


void playnote(void) {
int note = max_sample + MIDI_OFFSET;

	if (playing) {
		if (max_amplitude < SAMPLE_THRESHOLD) {
			sendnoteoff(current_note);
			playing = 0;
		}
		else if (note != current_note) {
			sendnoteoff(current_note);
			current_note = note;
			sendnoteon(current_note, MIDI_VELOCITY);
		}
		
	} else if (max_amplitude >= SAMPLE_THRESHOLD) {
		current_note = note;
		sendnoteon(current_note, MIDI_VELOCITY);
		playing = 1;
	}
}


void showdata() {
int i;
	if (max_amplitude < SAMPLE_THRESHOLD) return;

	Serial.print(millis());
	Serial.print(": ");
	Serial.print(max_sample);
	Serial.print(" ");

	for (i=0; i<(NUMBINS/2); i++) {
		if (sample_value(i) >= (SAMPLE_THRESHOLD/2)) {
			Serial.print(i);
			Serial.print(":");
			Serial.print(sample_value(i));
			Serial.print(" ");
		}
	}
	Serial.println("");
}

void setup() {
	pinMode(13,1);
	Serial.begin(57600);
}


#define SAMPLE_INTERVAL 2000UL
unsigned long next_sample_time;
byte sample;
int val;

void loop() {
  
	if (millis() > next_sample_time) {

		// Sample the buffer full at the ADC maximum rate
		for (sample=0; sample < NUMBINS; sample++) {
			val = analogRead(pin_adc);	// read one to throw away
			val = analogRead(pin_adc);
			data[sample] = val / 4 - 128;
			im[sample] = 0;
		}

		//this could be done with the fix_fftr function without the im array.
		digitalWrite(13,1);
		fix_fft(data, im, 7, 0);
		digitalWrite(13,0);

		// scan spectrum for max absolute magnitude power bin
		max_amplitude = 0;
		for (sample=0; sample < (NUMBINS/2); sample++) {
			val = sample_value(sample);
			if (val > max_amplitude) {
				max_sample = sample;
				max_amplitude = val;
			}
		}
		
		//showdata();
		playnote();

	    next_sample_time = 0;	// millis() + SAMPLE_INTERVAL;
	}
}


