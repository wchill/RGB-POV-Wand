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

#define ATMEGA328

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

#define MINDELAY 5
#define MAXDELAY 50

uint8_t led_cathodes[] = {RED, GREEN, BLUE};
uint8_t led_anodes[] = {LED1, LED2, LED3, LED4, LED5, LED6, LED7, LED8};

volatile int ani_length;
volatile int ani_idx;
volatile uint8_t colour_idx = 0; // 0 = R, 1 = G, 2 = B, repeating
volatile uint8_t shade_idx = 0;
volatile byte frame_buffer[8];

PROGMEM prog_uchar image[] = {0,127,
//uint8_t image[] = {0, 127,
96,128,192,228,233,237,242,242,64,128,192,229,232,238,241,246,64,128,164,228,233,237,242,246,64,132,160,228,232,241,241,246,
96,128,164,228,233,237,242,246,32,36,96,132,100,136,105,109,255,255,255,255,255,255,255,255,142,255,255,255,255,255,255,255,
68,145,255,255,214,105,109,109,64,100,145,255,255,214,173,246,68,100,104,178,255,255,218,177,68,72,145,255,255,218,141,249,
68,145,255,255,214,136,109,109,141,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,36,68,68,104,104,140,108,113,
255,255,72,255,255,255,255,255,255,255,68,255,255,255,255,255,36,36,76,72,108,108,141,141,72,108,68,255,255,255,255,255,
104,108,72,255,255,255,255,255,72,108,40,255,72,108,108,145,104,108,72,255,145,108,144,109,72,108,72,219,255,255,255,255,
76,108,108,76,218,255,255,255,36,40,72,76,104,112,108,145,255,255,72,255,255,255,255,255,255,255,68,255,255,255,255,255,
36,36,40,72,76,108,81,109,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,40,40,76,255,80,77,113,
255,77,40,109,255,76,189,222,182,255,255,255,186,84,158,222,8,145,255,149,48,88,189,190,8,40,8,44,44,81,81,186,
8,186,255,255,255,255,218,114,182,255,255,255,255,255,255,222,255,76,40,12,44,45,117,255,255,77,8,44,12,49,113,255,
187,255,255,255,255,255,255,218,4,187,255,255,255,255,187,118,4,4,8,12,16,44,85,153,255,255,114,12,20,93,125,190,
255,255,255,255,150,45,117,158,4,8,145,255,255,255,150,113,8,12,12,12,150,255,255,255,4,8,113,255,255,255,150,81,
255,255,255,255,150,45,117,158,255,255,146,13,16,93,126,191,4,5,9,12,13,45,117,190,4,255,255,255,255,255,45,190,
5,255,255,255,255,255,49,159,4,8,8,13,12,255,77,191,4,5,9,12,13,255,81,110,255,255,255,255,255,255,255,255,
255,255,255,255,255,255,255,255,4,8,8,13,13,255,46,114,9,13,18,22,21,45,86,190,9,13,17,22,26,63,126,191,
13,13,18,22,27,63,127,159,9,13,17,26,26,63,127,191,9,13,18,22,27,63,127,191,9,13,9,13,13,50,45,82,
9,100,164,224,224,224,224,224,96,224,255,255,255,224,224,114,160,224,255,0,255,224,224,224,192,224,224,224,224,224,224,82,
160,224,255,255,255,224,224,224,64,224,255,0,255,224,224,114,5,68,164,224,224,224,224,224,5,9,9,10,9,78,46,110,
9,9,14,19,55,119,155,155,5,10,14,19,51,119,151,187,5,6,10,10,42,78,119,187,5,77,144,220,216,152,109,151,
73,184,252,252,252,252,216,113,144,252,252,252,252,252,252,185,216,252,252,252,252,252,252,248,216,252,252,10,37,252,252,220,
176,252,6,38,74,73,252,181,68,5,6,38,37,78,73,145,1,2,5,255,255,41,151,147,1,7,1,255,255,74,147,183,
1,3,6,6,70,106,147,151,1,3,35,75,107,147,147,183,1,3,6,37,69,74,147,151,1,35,2,255,255,74,147,183,
1,3,33,255,255,42,179,179,1,34,34,38,38,110,147,183,33,34,71,103,139,143,179,183,1,66,34,38,37,110,147,183,
33,34,34,255,255,73,179,211,1,66,33,255,255,74,179,215,33,66,66,37,70,106,179,215,33,66,99,135,139,179,179,215,
33,66,131,135,171,175,211,211,33,98,99,167,167,207,211,215,65,66,131,130,101,101,106,174,33,98,97,70,28,28,28,109,
33,65,65,28,28,102,106,174,28,65,28,28,28,28,28,142,33,28,28,98,28,28,137,28,33,65,28,28,28,28,105,28,
65,64,28,28,28,28,137,106,32,65,28,28,28,28,105,28,33,28,28,97,28,28,138,28,28,65,28,28,28,28,28,109,
33,65,97,28,28,133,137,174,65,129,98,130,28,28,28,142,65,97,162,162,101,133,137,174,65,129,161,231,230,239,242,247,
96,97,162,226,234,238,243,243,64,129,129,133,97,170,170,142,65,65,97,124,124,101,137,124,32,65,124,156,124,137,124,110,
33,124,156,97,156,124,137,124,124,156,124,156,124,137,124,141,124,124,156,124,124,101,156,137,32,156,124,101,124,156,105,156,
64,64,156,124,156,133,156,137,64,96,96,156,124,137,105,156,64,128,129,128,101,169,174,137,64,129,192,229,233,238,242,246,
96,128,192,228,233,237,242,242,64,128,192,229,229,238,242,246,64,128,192,228,233,246
};


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
  ani_length = (image[0] << 8) + image[1];
  /*
  for(int i = 2; i < ani_length + 2; ++i) {
    memcpy_P((void*) frame_buffer, (const void*) (&image + 2 + (i * 8)), 8);
    eeprom_write_block((void*)frame_buffer, (void*) (i * 8 + 2), 8);
  }
  */
  //ani_length = (eeprom_read_byte((uint8_t*)0) << 8) + eeprom_read_byte((uint8_t*)1);
  
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
  /*
  if ( abs(speed - analogRead(A1)) > 2) {
    speed = analogRead(A1); 
    OCR1A = 5;
    //OCR1A = map(speed, 0, 1024, MINDELAY, MAXDELAY);
  }  
  */
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
      //memcpy((void*) frame_buffer, (const void*) (&image + 2 + (ani_idx * 8)), 8);
      memcpy_P((void*) frame_buffer, (const void*) (&image + 2 + (ani_idx * 8)), 8);
      //eeprom_read_block((void*)frame_buffer, (const void*)(2 + (ani_idx * 8)), 8); // load next frame
    }
  }
}
