/* fakelens.cpp
 * Code that pretends to be a lens and talks to the body.
 * Steven Bell <sebell@stanford.edu>
 * 27 September 2012
 */

#include "Arduino.h"
#include "typedef.h"
#include "common.h"

/* Performs one-time pin initialization and other setup.  The pin directions
 * here are the opposite of fakebody, since we're playing the other side. */
void setup() {
  Serial.begin(115200);
  pinMode(SLEEP, INPUT);
  pinMode(BODY_ACK, INPUT);
  pinMode(LENS_ACK, OUTPUT);
  pinMode(CLK, INPUT);
  pinMode(DATA, INPUT);
  pinMode(FOCUS, INPUT);
  pinMode(SHUTTER, INPUT);
}
// Wait for a falling edge on the body ACK pin
inline void waitBodyFall()
{
  while(!(BODY_ACK_PIN & BODY_ACK_HIGH)){} // Wait until it's high first
  while(BODY_ACK_PIN & BODY_ACK_HIGH){}
}

// Wait for a rising edge on the body ACK pin
inline void waitBodyRise()
{
  while(BODY_ACK_PIN & BODY_ACK_HIGH){} // Wait until it's low first
  while(!(BODY_ACK_PIN & BODY_ACK_HIGH)){}
}

// Wait until the body ACK pin is low
inline void waitBodyLow()
{
  delayMicroseconds(2);
  while(BODY_ACK_PIN & BODY_ACK_HIGH){}
}

// Wait until the body ACK pin is high
inline void waitBodyHigh()
{
  delayMicroseconds(2);
  while(!(BODY_ACK_PIN & BODY_ACK_HIGH)){}
}

/* Reads a single byte from the SPI bus.
 * Data is read LSB-first
 */
uint8 readByte()
{
  unsigned char value = 0;
  for(int i = 0; i < 8; i++){
    while(CLK_PIN & CLK_HIGH){} // Wait for the clock pin to fall
    while(!(CLK_PIN & CLK_HIGH)){} // Wait for the clock pin to rise
    // Save the bit
    value = value >> 1;
    if(DATA_PIN & DATA_HIGH){
      value |= 0x80;
    }
  }
  return(value);
}

/* Writes an 8-bit value on the data bus.  The clock is driven by the body.
 * This does some ugly tricks to get the data out fast enough.  It would be
 * faster (and probably more robust) to use the SPI hardware instead.
 */
void writeByte(uint8 value)
{
  DATA_DIR |= DATA_WRITE; // Set the data pin to be an output
  uint8 portvals[8];

  // Precalculate all of the port values we are going to set, so that setting
  // the pin is a single instruction.  If we have to do any work during the
  // loop, we won't be able to keep up and respond within 1us.
  for(uint8 i = 0; i < 8; i++){
    portvals[i] = DATA_PORT;
    if(value & 0x01){ // Set the data pin to the bit value
      portvals[i] |= DATA_HIGH;
    }
    else{
      portvals[i] &= DATA_LOW;
    }
    value = value >> 1; // Shift down to the next bit
  }

  // Data is set on the falling edge, and the body reads it on the rising edge
  for(uint8 i = 0; i < 8; i++){
    while(CLK_PIN & CLK_HIGH){} // Wait for the clock pin to fall
    DATA_PORT = portvals[i];
    while(!(CLK_PIN & CLK_HIGH)){} // Wait for the clock pin to rise
  }
}

int main()
{
  init(); // Arduino library init
  setup(); // Pin setup
  uint8 checksum = 0;

  // Sit and wait for the sleep pin to go high (camera is turned on)
  while(digitalRead(SLEEP) == 0){}

  // Check that the body ACK pin is high
  waitBodyHigh();

  // Pulse our ACK pin to let the body know we're awake
  digitalWrite(LENS_ACK, 1);
  delay(10);
  digitalWrite(LENS_ACK, 0);

  // Wait until the body ACK goes high
  waitBodyRise();

  // Ready
  digitalWrite(LENS_ACK, 1);

  // Read four bytes
  checksum += readByte(); // 0xB0
  digitalWrite(LENS_ACK, 0); // Working
  digitalWrite(LENS_ACK, 1); // Ready

  checksum += readByte(); // 0xF2
  digitalWrite(LENS_ACK, 0); // Working
  digitalWrite(LENS_ACK, 1); // Ready

  checksum += readByte(); // 0x00
  digitalWrite(LENS_ACK, 0); // Working
  digitalWrite(LENS_ACK, 1); // Ready

  checksum += readByte(); // 0x00
  digitalWrite(LENS_ACK, 0); // Working
  // Note: No ready here, we're waiting for the body to drop

  pinMode(DATA, OUTPUT);
  digitalWrite(DATA, HIGH);
  waitBodyFall();
  digitalWrite(LENS_ACK, 1); // Ready
  waitBodyRise();

  // Now we reply with the checksum
  writeByte(checksum);

  waitBodyFall();
  digitalWrite(LENS_ACK, 0);
  waitBodyRise();
  digitalWrite(LENS_ACK, 1);

  delay(500);

  digitalWrite(LENS_ACK, 0);
  waitBodyLow(); // Falling edge happens very fast
  digitalWrite(LENS_ACK, 1);
  waitBodyRise();

  writeByte(0x00);

  pinMode(DATA, INPUT);

  waitBodyLow(); // Falling edge happens very fast
  digitalWrite(LENS_ACK, 0);
  waitBodyRise();
  digitalWrite(LENS_ACK, 1);

  checksum = 0;
    // Read four bytes
  checksum += readByte(); // 0xB0
  digitalWrite(LENS_ACK, 0); // Working
  digitalWrite(LENS_ACK, 1); // Ready

  checksum += readByte(); // 0xF2
  digitalWrite(LENS_ACK, 0); // Working
  digitalWrite(LENS_ACK, 1); // Ready

  checksum += readByte(); // 0x00
  digitalWrite(LENS_ACK, 0); // Working
  digitalWrite(LENS_ACK, 1); // Ready

  checksum += readByte(); // 0x00
  digitalWrite(LENS_ACK, 0); // Working

  waitBodyLow();
  digitalWrite(LENS_ACK, 1);
  writeByte(checksum);

  // # bytes, bytes, checksum
  uint8 sendBytes[] = {0x05, 0x00, 0x0a, 0x10, 0xc4, 0x09, 0xe7};

  for(int i = 0; i < 7; i++){
    waitBodyFall(); // Wait for body to drop
    digitalWrite(LENS_ACK, 0); // We drop and then rise (ready to send next byte)
    digitalWrite(LENS_ACK, 1);
    writeByte(sendBytes[i]);
  }

  // ACK
  while(1); // Don't ever return; who knows where we'd end up.
  return(0);
}
