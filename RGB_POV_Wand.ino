/*
  RGB POV core firmware for Arduino Pro Micro
  Written by Eric Ahn (ericahn3@illinois.edu)
  
  Adapted from MiniPOV4 Core firmware, Arduino-esque edition
  Written by Frank Zhao for Adafruit Industries
  https://github.com/adafruit/Adafruit-MiniPOV4-Kit/blob/master/Firmware/MiniPOV4Core.ino

  Licensed under GPL v2
*/


#include <avr/io.h>
#include <avr/eeprom.h>
#include <avr/pgmspace.h>
#include <avr/interrupt.h>
#include <avr/wdt.h>
#include <util/delay.h>

#define RED_BIT 2
#define GREEN_BIT 6
#define BLUE_BIT 3

#define RED 16
#define GREEN 10
#define BLUE 14
#define LED1 2
#define LED2 3
#define LED3 4
#define LED4 5
#define LED5 6
#define LED6 7
#define LED7 8
#define LED8 9

#define MINDELAY 5
#define MAXDELAY 50

uint8_t led_cathodes[] = {RED, GREEN, BLUE};
uint8_t led_anodes[] = {LED1, LED2, LED3, LED4, LED5, LED6, LED7, LED8};

volatile int ani_length;
volatile int ani_idx;
volatile uint8_t colour_idx = 0; // 0 = R, 1 = G, 2 = B, repeating
volatile uint8_t shade_idx = 0;
volatile byte frame_buffer[8];

int speed = 0;

void setup() {               
  
  // 10k trimpot
  pinMode(A0, OUTPUT);
  pinMode(A2, OUTPUT);
  pinMode(A1, INPUT);
  digitalWrite(A0, HIGH);
  digitalWrite(A2, LOW);
  
  for(int i = 0; i < 8; ++i) {
    pinMode(led_anodes[i], OUTPUT);
    digitalWrite(led_anodes[i], LOW);
  }
  
  for(int i = 0; i < 3; ++i) {
    pinMode(led_cathodes[i], OUTPUT);
    digitalWrite(led_cathodes[i], LOW);
  }
  
  for (uint8_t c=0; c<3; c++) {
    // turn on one color at a time
    digitalWrite(led_cathodes[c], HIGH);
    for (uint8_t a=0; a<8; a++) {
      // turn on one LED at a time
      digitalWrite(led_anodes[a], HIGH);
      delay(30);
    }
    // turn it off
    digitalWrite(led_cathodes[c], LOW);
    for (uint8_t a=0; a<8; a++) {
      digitalWrite(led_anodes[a], LOW);
    }
  }
  
  // check the animation length
  ani_length = (eeprom_read_byte((uint8_t*)0) << 8) + eeprom_read_byte((uint8_t*)1);
  
  speed = analogRead(A1);
  
  // start timer
  TCCR1A = 0;
  TCCR1B = _BV(WGM12) | 0x04;
  //OCR1A = map(speed, 0, 1024, MINDELAY, MAXDELAY); // this should not be lower than 500
  OCR1A = 5;
  TIMSK1 |= _BV(OCIE1A);   // Output Compare Interrupt Enable (timer 1, OCR1A)
}

// the loop routine runs over and over again forever:
void loop() {
  if ( abs(speed - analogRead(A1)) > 2) {
    speed = analogRead(A1); 
    OCR1A = 5;
    //OCR1A = map(speed, 0, 1024, MINDELAY, MAXDELAY);
  }
}

ISR(TIMER1_COMPA_vect)
{
  // show the next frame
  // data format is
  // 0bRRRGGGBB, eight of them
  uint8_t i, b, cathodeport = 0, ledport = 0;
   
  // We use direct port access is used instead of Arduino-style
  // because this needs to be fast
     
  if (colour_idx == 0) {
    cathodeport = _BV(RED_BIT);

    for (i = 0; i < 8; i++) {
      b = (frame_buffer[i] & 0xE0) >> 5;
      if (b > shade_idx)
        ledport |= _BV(i); 
    }
  }
  else if (colour_idx == 1)
  {
    cathodeport = _BV(GREEN_BIT);
    
    for (i = 0; i < 8; i++) {
      b = (frame_buffer[i] & 0x1C) >> 2;
      if (b > shade_idx)
        ledport |= _BV(i);
    }
  }
  else if (colour_idx == 2)
  {
    cathodeport = _BV(BLUE_BIT);
  
    uint8_t s = shade_idx >> 1;

    for (i = 0; i < 8; i++) {
      b = frame_buffer[i] & 0x03;
      if (b > s)
        ledport |= _BV(i); 
    }
  }
  
  // Port mapping on the Arduino Pro Micro is complicated
  
  // (X denotes a pin that is either unused or does not map to
  // an Arduino style digital/analog pin on the Pro Micro)
  
  // PORTB bit: 7  6  5  4  3  2  1  0
  //       Pin: X  10 9  8  14 16 15 X
  
  // PORTC bit: 7  6  5  4  3  2  1  0
  //       Pin: X  5  X  X  X  X  X  X
  
  // PORTD bit: 7  6  5  4  3  2  1  0
  //       Pin: 6  X  X  4  TX RX 2  3
  
  // PORTE bit: 7  6  5  4  3  2  1  0
  //       Pin: X  7  X  X  X  X  X  X
  
  // PORTF bit: 7  6  5  4  3  2  1  0
  //       Pin: A0 A1 A2 A3 X  X  X  X
  
  // We are using pins 2-9 on the Pro Micro for the anodes. This maps to
  // PORTB: B00110000 (pin 9, 8)
  // PORTC: B01000000 (pin 5)
  // PORTD: B10010011 (pin 6, 4, 3, 2)
  // PORTE: B01000000 (pin 7)
  
  // Pins 10, 14 and 16 on the Pro Micro are for the cathodes:
  // PORTB: B01001100 (pin 10, 14, 16)
  
  PORTB &= ~(B00110000) & ~_BV(RED_BIT) & ~_BV(GREEN_BIT) & ~_BV(BLUE_BIT);
  PORTC &= ~(B01000000);
  PORTD &= ~(B10010011);
  PORTE &= ~(B01000000);
  
  PORTB |= (ledport & B11000000) >> 2;
  PORTC |= (ledport & B00001000) << 3;
  PORTD |= ((ledport & B00000001) << 1) | ((ledport & B00000010) >> 1) | ((ledport & B00000100) << 2) | ((ledport & B00010000) << 3);
  PORTE |= (ledport & B00100000) << 1;
  
  PORTB |= cathodeport;
  
  /*
  LED_3_1_PORTx &= ~(0x07);
  LED_8_4_PORTx &= ~(0xF8);
  LED_CATHODES_PORTx &= ~_BV(LED_RED_PINNUM) & ~_BV(LED_GREEN_PINNUM) & ~_BV(LED_BLUE_PINNUM);
  LED_3_1_PORTx |= ledport & 0x7;
  LED_8_4_PORTx |= ledport & 0xF8;
  LED_CATHODES_PORTx |= cathodeport;
  */

  // next color
  colour_idx++;
  if (colour_idx >= 3) {
    colour_idx = 0;
    shade_idx++;
    if (shade_idx >= 7) {
      shade_idx = 0;
      // simple next frame with roll over
      ani_idx++;
      if (ani_idx >= ani_length) {
        ani_idx = 0;
      }
      eeprom_read_block((void*)frame_buffer, (const void*)(2 + (ani_idx * 8)), 8); // load next frame
    }
  }
}
