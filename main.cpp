#include "Arduino.h"
#include "typedef.h"

#define SLEEP 53 // Port B 0
#define BODY_ACK 51 // Port B 2
#define LENS_ACK 49 // Port L 0
#define LENS_ACK_PIN PINL
#define CLK 45 // Port L 4
#define CLK_PORT PORTL
#define DATA 47 // Port L 2
#define DATA_PORT PORTL
#define DATA_DIR DDRL

// Useful bitmasks for manipulating the IO pins
// Bitwise OR these with port registers to set pins high
const uint8 CLK_HIGH = 0b00010000;
const uint8 DATA_HIGH = 0b00000100;

// Bitwise AND these with port registers to read the pin
const uint8 LENS_ACK_HIGH = 0b00000001;

const uint8 DATA_WRITE = 0b00010100; // Both CLK and DATA pins

// Bitwise AND these with port registers to set pins low
const uint8 CLK_LOW = ~CLK_HIGH;
const uint8 DATA_LOW = ~DATA_HIGH;

const uint8 DATA_READ = DATA_LOW;

/* Performs one-time pin initialization and other setup */
void setup() {
  pinMode(SLEEP, OUTPUT);
  pinMode(BODY_ACK, OUTPUT);
  pinMode(LENS_ACK, INPUT);
  pinMode(CLK, OUTPUT);
  pinMode(DATA, OUTPUT);

  digitalWrite(SLEEP, LOW);
  digitalWrite(BODY_ACK, LOW);
}

/* Writes a single byte on the SPI bus at about 500 kHz, and waits for the
 * camera acknowledgment.
 * The clock and data pins are set to be outputs
 * The data is written LSB-first. */
void writeByte(uint8 value)
{
  DATA_DIR |= DATA_WRITE;
  // Data is set on the falling edge, and the lens reads it on the rising edge
  for(uint8 i = 0; i < 8; i++){
    CLK_PORT &= CLK_LOW; // Set the clock pin low
    if(value & 0x01){ // Set the data pin to the bit value
      DATA_PORT |= DATA_HIGH;
    }
    else{
      DATA_PORT &= DATA_LOW;
    }
    CLK_PORT |= CLK_HIGH; // Set the clock pin high
    value = value >> 1; // Shift down to the next bit
  }
  // Wait for the ACK
  delayMicroseconds(15);
}

/* Reads a single byte from the SPI bus.
 *
 */
unsigned char readByte()
{
  unsigned char val;
  for(int i = 0; i < 8; i++){
    CLK_PORT &= CLK_LOW;
    CLK_PORT |= CLK_HIGH;
  }
  return(0);
}

// Wait for a falling edge on the lens ACK pin
void waitLensFall()
{
  while(!(LENS_ACK_PIN & LENS_ACK_HIGH)){} // Wait until it's high first
  while(LENS_ACK_PIN & LENS_ACK_HIGH){}
}

// Wait for a rising edge on the lens ACK pin
void waitLensRise()
{
  while(LENS_ACK_PIN & LENS_ACK_HIGH){} // Wait until it's low first
  while(!(LENS_ACK_PIN & LENS_ACK_HIGH)){}
}

void powerup() {
  // Powerup
  digitalWrite(SLEEP, HIGH);
  delay(10);
  digitalWrite(BODY_ACK, HIGH);

  waitLensFall();
  digitalWrite(BODY_ACK, LOW);

  delay(20);

  // Now start some data transfer
  digitalWrite(BODY_ACK, HIGH);
  while(digitalRead(LENS_ACK) == 0){}

  writeByte(0xB0);
  writeByte(0xF2);
  writeByte(0x00);
  writeByte(0x00);

  // Relinquish control of the data pin
  pinMode(DATA, INPUT);

  delayMicroseconds(1000);
  digitalWrite(BODY_ACK, LOW);
  // Probably supposed to do something here?
  digitalWrite(BODY_ACK, HIGH); // Tell the lens we're ready
  readByte(); // This should be 0xA2
  digitalWrite(BODY_ACK, LOW); // Tell the lens we're working
  // TODO: watch lens ACK
  digitalWrite(BODY_ACK, HIGH); // Tell the lens we're done

  waitLensFall();
  digitalWrite(BODY_ACK, LOW);
  digitalWrite(BODY_ACK, HIGH);

  readByte(); // Should be 0x00?
  digitalWrite(BODY_ACK, LOW); // Tell the lens we're working
  delayMicroseconds(1000); // Lens takes a while to respond?
  digitalWrite(BODY_ACK, HIGH); // Tell the lens we're done

  while(digitalRead(LENS_ACK) == 0){}
  writeByte(0xC0);
  writeByte(0xF6);
  writeByte(0x00);
  writeByte(0x00);

  // Handoff of data pin
  pinMode(DATA, INPUT);

  // This should read
  // 0xB6  0x05  0x00  0x0A  0x10  0xC4  0x09  0xE7
  for(int i = 0; i < 8; i++){
    digitalWrite(BODY_ACK, LOW);
    waitLensRise();
    digitalWrite(BODY_ACK, HIGH);
    readByte();
  }

  digitalWrite(BODY_ACK, LOW);
  delay(1);
  digitalWrite(BODY_ACK, HIGH);
  waitLensRise();

  writeByte(0xA0);
  writeByte(0xF5);
  writeByte(0x01);
  writeByte(0x00);

  pinMode(DATA, INPUT);
  digitalWrite(BODY_ACK, LOW);
  digitalWrite(BODY_ACK, HIGH);
  readByte(); // Should be 0x96

  digitalWrite(BODY_ACK, LOW);
  delay(1);
  digitalWrite(BODY_ACK, HIGH);
  waitLensRise();

  writeByte(0xC1);
  writeByte(0xF9);
  writeByte(0x00);
  writeByte(0x00);

  pinMode(DATA, INPUT);
  /*
  digitalWrite(BODY_ACK, LOW);
  digitalWrite(BODY_ACK, HIGH);
  readByte(); // Should be 0xBA
  */

  for(int i = 0; i < 24; i++){
    digitalWrite(BODY_ACK, LOW);
    waitLensRise();
    digitalWrite(BODY_ACK, HIGH);
    readByte();
  }


  delay(100);
  digitalWrite(SLEEP, LOW);

  digitalWrite(BODY_ACK, LOW);

}

int main()
{
  init(); // Arduino library initialization
  setup(); // Pin setup and other init
  while(1){
    powerup();
    delay(1000);
  }


}
