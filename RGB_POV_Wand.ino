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

// select chip type (uncomment the corresponding #define)
// ATMEGA328: Arduino Uno/Pro/Pro Mini/other 328-based boards
// ATMEGA32U4: Arduino Micro/Pro Micro/other 32u4-based boards

#define ATMEGA328
// #define ATMEGA32U4

// By default, images are stored in flash memory (hence PROGMEM). They can be stored in EEPROM
// but you will need to use avrdude to program the image data onto the chip.
// To use EEPROM, comment the line below

#define USE_FLASH


#ifdef USE_FLASH

// Replace with your own image data either manually or with the provided converters
// Image can be up to 65536x8 pixels big, but for best results the image size should be 128x8 or less
// (The included image converters require that the image size be 127x8 or less)
// image[0] and image[1] contain the width of the image (big endian)
// Every subsequent byte represents one pixel, going from top to bottom then left to right in RRRGGGBB format.
// This has no effect if EEPROM is being used - you must use avrdude to program an image. The format is the same
// but you are limited to an image 127x8 pixels or less (just under 1024 bytes).

PROGMEM prog_uchar image[] = {0,1,255,255,255,255,255,255,255,255};

#endif

// Set frame delay. This is used in conjunction with the divisor to set how often the chip should update LED state.
// Default setting is 5, but you may find other settings more suitable.
#define DELAY 5

// Set power on self test (POST) delay.
// Default setting is 30
#define POST_DELAY 30

// chip-specific defines, do not modify
#ifdef ATMEGA32U4

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

#elif defined(ATMEGA328)

#define RED_BIT 0
#define GREEN_BIT 1
#define BLUE_BIT 2

#define RED A0
#define GREEN A1
#define BLUE A2
#define LED1 4
#define LED2 5
#define LED3 6
#define LED4 7
#define LED5 8
#define LED6 9
#define LED7 10
#define LED8 11

#endif

// convenience/temporary variables
uint8_t led_cathodes[] = {RED, GREEN, BLUE};
uint8_t led_anodes[] = {LED1, LED2, LED3, LED4, LED5, LED6, LED7, LED8};

volatile int ani_length;
volatile int ani_idx;
volatile uint8_t colour_idx = 0; // 0 = R, 1 = G, 2 = B, repeating
volatile uint8_t shade_idx = 0;
volatile byte frame_buffer[8];

void setup() {           
  
  // set up LED pins
  
  for(uint8_t i = 0; i < 8; ++i) {
    pinMode(led_anodes[i], OUTPUT);
    digitalWrite(led_anodes[i], LOW);
  }
  
  for(uint8_t i = 0; i < 3; ++i) {
    pinMode(led_cathodes[i], OUTPUT);
    digitalWrite(led_cathodes[i], LOW);
  }
  
  // LED power on self test
  
  for (uint8_t c = 0; c < 3; ++c) {
    // turn on one color at a time
    digitalWrite(led_cathodes[c], HIGH);
    
    for (uint8_t a = 0; a < 8; ++a) {
      // turn on one LED at a time
      digitalWrite(led_anodes[a], HIGH);
      delay(POST_DELAY);
    }
    
    // turn it off
    digitalWrite(led_cathodes[c], LOW);
    for (uint8_t a = 0; a < 8; a++) {
      digitalWrite(led_anodes[a], LOW);
    }
  }
  
  // check the animation length
  #ifdef USE_FLASH
  ani_length = pgm_read_word_near(&image);
  #else
  ani_length = (eeprom_read_byte((uint8_t*)0) << 8) + eeprom_read_byte((uint8_t*)1);
  #endif
  
  // start timer
  TCCR1A = 0;
  TCCR1B = _BV(WGM12) | 0x04;
  OCR1A = 5;
  TIMSK1 |= _BV(OCIE1A);   // Output Compare Interrupt Enable (timer 1, OCR1A)
}

// does nothing, everything is handled by interrupts
void loop() {}

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
  
  #ifdef ATMEGA32U4

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
  
  #elif defined(ATMEGA328)

  // PORTD: B11110000 (pin 7,6,5,4)
  // PORTB: B00001111 (pin 11,10,9,8)
  // PORTC: B00000111 (pin A2,A1,A0)
  
  PORTD &= B00001111;
  PORTB &= B11110000;
  PORTC &= B11111000;
  
  PORTD |= (ledport & B00001111) << 4;
  PORTB |= (ledport & B11110000) >> 4;
  PORTC |= cathodeport;
  

  #endif

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
      // load next frame
      #ifdef USE_FLASH
      memcpy_P((void*) frame_buffer, (const void*) (&image + 2 + (ani_idx * 8)), 8);
      #else
      eeprom_read_block((void*)frame_buffer, (const void*)(2 + (ani_idx * 8)), 8);
      #endif
    }
  }
}
