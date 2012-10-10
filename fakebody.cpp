/* fakebody.cpp
 * Code that pretends to the the camera body and drives the lens.
 * Steven Bell <sebell@stanford.edu>
 * August 2012
 */

#include "Arduino.h"
#include "typedef.h"
#include "common.h"

/* Performs one-time pin initialization and other setup */
void setup() {
  Serial.begin(115200);
  pinMode(SLEEP, OUTPUT);
  pinMode(BODY_ACK, OUTPUT);
  pinMode(LENS_ACK, INPUT);
  pinMode(CLK, OUTPUT);
  pinMode(DATA_MISO, OUTPUT);
  pinMode(DATA_MOSI, INPUT); // We're only using MISO, but they're tied together.
  pinMode(FOCUS, OUTPUT);
  pinMode(SHUTTER, OUTPUT);

  digitalWrite(SLEEP, LOW);
  digitalWrite(BODY_ACK, LOW);
}

/* Writes a single byte on the SPI bus at about 500 kHz, and waits for the
 * camera acknowledgment.
 * The clock and data pins are set to be outputs
 * The data is written LSB-first. */
void writeByte(uint8 value)
{
  DATA_DIR |= DATA_WRITE; // Just in case...
  DATA_DIR |= CLK_WRITE;
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
 * Data is read LSB-first
 */
unsigned char readByte()
{
  unsigned char value = 0;
  for(int i = 0; i < 8; i++){
    CLK_PORT &= CLK_LOW;
    value = value >> 1;
    CLK_PORT |= CLK_HIGH;
    if(DATA_PIN & DATA_HIGH){
      value |= 0x80;
    }
  }
  return(value);
}

// Wait for a falling edge on the lens ACK pin
inline void waitLensFall()
{
  while(!(LENS_ACK_PIN & LENS_ACK_HIGH)){} // Wait until it's high first
  while(LENS_ACK_PIN & LENS_ACK_HIGH){}
}

// Wait for a rising edge on the lens ACK pin
inline void waitLensRise()
{
  while(LENS_ACK_PIN & LENS_ACK_HIGH){} // Wait until it's low first
  while(!(LENS_ACK_PIN & LENS_ACK_HIGH)){}
}

// Wait until the lens ACK pin is high
inline void waitLensHigh()
{
  delayMicroseconds(2);
  while(!(LENS_ACK_PIN & LENS_ACK_HIGH)){}
}

// Wait until the lens ACK pin is low
inline void waitLensLow()
{
  delayMicroseconds(2);
  while((LENS_ACK_PIN & LENS_ACK_HIGH)){}
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
  readByte(); // This should be 0xA2 (checksum)
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
  // 0xB6 (checksum) 0x05 (# bytes)  [0x00  0x0A  0x10  0xC4  0x09] (data) 0xE7 (checksum)
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
  readByte(); // Should be 0x96 (checksum)

  digitalWrite(BODY_ACK, LOW);
  delay(1);
  digitalWrite(BODY_ACK, HIGH);
  waitLensRise();

  writeByte(0xC1);
  writeByte(0xF9);
  writeByte(0x00);
  writeByte(0x00);

  pinMode(DATA, INPUT);
  for(int i = 0; i < 24; i++){
    digitalWrite(BODY_ACK, LOW);
    waitLensRise();
    digitalWrite(BODY_ACK, HIGH);
    readByte();
  }

  digitalWrite(BODY_ACK, LOW);
  delay(1);
  digitalWrite(BODY_ACK, HIGH);
  waitLensRise();

  writeByte(0x60);
  writeByte(0xF0);
  writeByte(0x00);
  writeByte(0x00);

  // Read one byte
  pinMode(DATA, INPUT);
  digitalWrite(BODY_ACK, LOW);
  waitLensRise();
  digitalWrite(BODY_ACK, HIGH);
  readByte(); // Should be 0x50 (checksum)

  digitalWrite(BODY_ACK, LOW);
  delayMicroseconds(250);
  digitalWrite(BODY_ACK, HIGH);
  waitLensRise();

  writeByte(0x05);
  writeByte(0x0C);
  writeByte(0x06);
  writeByte(0xD1);
  writeByte(0x00);
  writeByte(0x06);

  // Read one byte
  pinMode(DATA, INPUT);
  digitalWrite(BODY_ACK, LOW);
  waitLensRise();
  digitalWrite(BODY_ACK, HIGH);
  readByte(); // Should be 0xD3

  digitalWrite(BODY_ACK, LOW); // Clean up after ourselves
  delay(1);
}

void standbyPacket()
{
  const uint8 RESPONSE_BYTES = 34; // Number of bytes in the response

  // Doing a waitLensRise() can be too slow here, so first check that the
  // pin is low, then wait for it to be high after we set our pin.
  waitLensLow();
  digitalWrite(BODY_ACK, HIGH);
  waitLensHigh();

  writeByte(0xC1);
  writeByte(0x80);
  writeByte(0x01);
  writeByte(0x02);

  uint8 response[RESPONSE_BYTES];
  pinMode(DATA, INPUT);
  for(uint8 i = 0; i < RESPONSE_BYTES; i++){
    digitalWrite(BODY_ACK, LOW);
    waitLensHigh();
    digitalWrite(BODY_ACK, HIGH);
    response[i] = readByte();
  }

  digitalWrite(BODY_ACK, LOW);

  for(uint8 i = 0; i < RESPONSE_BYTES; i++){
    Serial.print(response[i]);
    Serial.print(" ");
  }
  Serial.print('\n');

}

void extendedPacket(uint8 data[17])
{
  digitalWrite(BODY_ACK, HIGH);
  waitLensRise();

  // Write 4 bytes
  writeByte(data[0]);
  writeByte(data[1]);
  writeByte(data[2]);
  writeByte(data[3]);
  delayMicroseconds(100);

  // Read one byte
  pinMode(DATA, INPUT);
  digitalWrite(BODY_ACK, LOW);
  waitLensRise();
  digitalWrite(BODY_ACK, HIGH);
  data[4] = readByte();

  digitalWrite(BODY_ACK, LOW);
  delayMicroseconds(250);
  digitalWrite(BODY_ACK, HIGH);
  waitLensRise();

  // Write 11 bytes
  for(uint8 i = 5; i < 16; i++){
    writeByte(data[i]);
  }
  delayMicroseconds(100);

  // Read one byte
  pinMode(DATA, INPUT);
  digitalWrite(BODY_ACK, LOW);
  waitLensRise();
  digitalWrite(BODY_ACK, HIGH);
  data[16] = readByte();

  delayMicroseconds(100);  // Wait a little while (to match the camera, not sure if necessary)
  digitalWrite(BODY_ACK, LOW); // Clean up after ourselves
}

inline void pulseShutter(void)
{
  digitalWrite(SHUTTER, HIGH);
  delayMicroseconds(500);
  digitalWrite(SHUTTER, LOW);
}

int main()
{
  init(); // Arduino library initialization
  setup(); // Pin setup and other init
  powerup();
  while(1){
    // Do some standby packets
    for(int i = 0; i < 50; i++){
      delay(13);
      pulseShutter();
      delayMicroseconds(1500);

      digitalWrite(FOCUS, !digitalRead(FOCUS)); // Flip the focus pin
      delay(14);

      pulseShutter();
      delay(1);
      standbyPacket();
    }

    // Now send an extended packet
    delay(1);
    uint8 one[] = {0x60, 0x80, 0xfe, 0x02, 0x00, 0x0a, 0x00, 0x01, 0xca, 0x04, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
    extendedPacket(one);

    // Back to standby packets
    for(int i = 0; i < 50; i++){
      delay(13);
      pulseShutter();
      delayMicroseconds(1500);

      digitalWrite(FOCUS, !digitalRead(FOCUS)); // Flip the focus pin
      delay(14);

      pulseShutter();
      delay(1);
      standbyPacket();
    }
/*
    // Back to standby packets
    for(int i = 0; i < 50; i++){
      digitalWrite(FOCUS, HIGH);
      delay(3);

      // Pulse the shutter pin
      digitalWrite(SHUTTER, HIGH);
      delay(1);
      digitalWrite(SHUTTER, LOW);
      delay(1);

      standbyPacket();
      delay(8);

      digitalWrite(FOCUS, LOW);
      delay(3);

      // Pulse the shutter pin
      digitalWrite(SHUTTER, HIGH);
      delay(1);
      digitalWrite(SHUTTER, LOW);
      delay(1);

      standbyPacket();
      delay(8);
    }
*/
    // Another extended packet
    delay(1);
    uint8 two[] = {0x60, 0x80, 0xfe, 0x02, 0x00, 0x0a, 0x00, 0x01, 0xb9, 0x04, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
    extendedPacket(two);

  }


}
