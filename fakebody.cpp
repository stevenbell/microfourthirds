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
  digitalWrite(CLK, HIGH);
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
  while(!(LENS_ACK_PIN & LENS_ACK_HIGH)){}
}

// Wait until the lens ACK pin is low
inline void waitLensLow()
{
  while((LENS_ACK_PIN & LENS_ACK_HIGH)){}
}

/* Sends a 4-byte command and waits for the checksum
 * Returns true if the checksum matches, false otherwise.
 * The BODY_ACK pin should be low when this method enters.
 * The BODY_ACK pin is low when this method exits. */
bool sendCommand(uint8* bytes)
{
  uint8 checksum = 0; // Our running checksum
  uint8 checkbyte; // Checksum from the lens

  digitalWrite(BODY_ACK, HIGH); // Get the lens' attention
  waitLensHigh(); // Wait for it to be ready

  // Send the four bytes
  for(uint8 i = 0; i < 4; i++){
    writeByte(bytes[i]);
    checksum += bytes[i];
  }

  pinMode(DATA, INPUT); // Relinquish control of the data pin

  waitLensLow();
  digitalWrite(BODY_ACK, LOW);
  waitLensHigh();
  digitalWrite(BODY_ACK, HIGH); // Tell the lens we're ready

  checkbyte = readByte();

  digitalWrite(BODY_ACK, LOW);

  return(checksum == checkbyte);
}

/* Reads a series of bytes in response to a command
 * bytes - Pointer to store the byte values in.
 * maxBytes - Maximum number of bytes to read, not including header.
 * Returns true if expected number of bytes is read, false otherwise. */
uint16 readBytes(uint8* bytes, uint16 maxBytes)
{
  // Read the packet length
  uint16 nBytes;

  waitLensHigh();
  digitalWrite(BODY_ACK, HIGH);
  nBytes = readByte(); // Low 8 bits
  BODY_ACK_PORT &= BODY_ACK_LOW;

  waitLensRise();
  digitalWrite(BODY_ACK, HIGH);
  nBytes += (uint16)readByte() << 8; // High 8 bits
  digitalWrite(BODY_ACK, LOW);

  waitLensLow(); // Just to be safe

  // If the lens is trying to give us more bytes than we have storage for, fail.
  if(nBytes > maxBytes){
    return(0);
  }

  for(uint16 i = 0; i < nBytes; i++){
    waitLensHigh();
    digitalWrite(BODY_ACK, HIGH);
    bytes[i] = readByte();
    //BODY_ACK_PORT &= BODY_ACK_LOW; // Set low; digitalWrite is too slow here.
    digitalWrite(BODY_ACK, LOW);
    // BUG: something isn't working with waitLensLow.
    //waitLensLow();
    delayMicroseconds(10);
  }

  return(nBytes);
}

void powerup() {
  uint8 bytedump[50]; // Array for dumping bytes read from the lens

  // Powerup
  digitalWrite(SLEEP, HIGH);
  delay(10);
  digitalWrite(BODY_ACK, HIGH);

  waitLensFall();
  digitalWrite(BODY_ACK, LOW);

  delay(20);

  // Now start some data transfer
  uint8 c1[4] = {0xB0, 0xF2, 0x00, 0x00};
  sendCommand(c1);

  // There seems to be an extra low-high here, not sure why
  waitLensLow();
  digitalWrite(BODY_ACK, HIGH);
  waitLensFall(); // Wait for rise and fall
  digitalWrite(BODY_ACK, LOW);
  digitalWrite(BODY_ACK, HIGH);

  readByte(); // Should be 0x00?
  digitalWrite(BODY_ACK, LOW); // Tell the lens we're working

  delay(1);

  uint8 c2[4] = {0xC0, 0xF6, 0x00, 0x00};
  sendCommand(c2);
  readBytes(bytedump, 5);


  delay(1);

  uint8 c3[4] = {0xA0, 0xF5, 0x01, 0x00};
  sendCommand(c3);

  // This is where the camera does a clock reset.  Is that important?
  delay(1);

  uint8 c4[4] = {0xC1, 0xF9, 0x00, 0x00};
  sendCommand(c4);

  readBytes(bytedump, 0x15); // Read 21 bytes

  delay(1);

  uint8 c5[4] = {0x60, 0xF0, 0x00, 0x00};
  sendCommand(c5);

  digitalWrite(BODY_ACK, HIGH);
  waitLensHigh();
  // Old lens had different commands; not sure what these are.
  writeByte(0x05);
  writeByte(0x00);
  writeByte(0x00);
  writeByte(0x00);
  writeByte(0x00);
  writeByte(0x00);

  // Read one byte
  pinMode(DATA, INPUT);
  digitalWrite(BODY_ACK, LOW);
  waitLensRise();
  digitalWrite(BODY_ACK, HIGH);
  readByte();
  digitalWrite(BODY_ACK, LOW);

  // Standby packet
  uint8 c6[4] = {0xC1, 0x80, 0x01, 0x06};
  sendCommand(c6);
  readBytes(bytedump, 0x1F); // Read 31 bytes

  delay(1);

  // Manual focus
  uint8 c7[4] = {0xA0, 0xB0, 0xFE, 0x00}; // Ring forward
  //uint8 c7[4] = {0xA0, 0xB0, 0xFE, 0x01}; // Ring reverse
  sendCommand(c7);

  delay(1);

  uint8 c8[4] = {0xB1, 0x88, 0x03, 0x02};
  sendCommand(c8);

  digitalWrite(BODY_ACK, LOW); // Clean up after ourselves
  delay(1);
}

void standbyPacket()
{
  const uint8 RESPONSE_BYTES = 31;
  uint8 resultBytes[RESPONSE_BYTES];
  uint8 standbyRequest[] = {0xC1, 0x80, 0x01, 0x06};
  sendCommand(standbyRequest);
  waitLensLow(); // BUG: Hangs here if the function return doesn't happen fast enough
  readBytes(resultBytes, RESPONSE_BYTES);

  // Print all of the bytes, skipping the checksum at the end
  /*
  for(uint8 i = 0; i < RESPONSE_BYTES -1; i++){
    Serial.print(resultBytes[i]);
    Serial.print(" ");
  }
  Serial.print('\n');
  */

}

void extendedPacket(uint8 data[17])
{
  digitalWrite(BODY_ACK, HIGH);
  waitLensHigh();

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
  // We really should wait for the lens to be low here
  delayMicroseconds(250);
  digitalWrite(BODY_ACK, HIGH);
  waitLensHigh();

  // Write 11 bytes
  for(uint8 i = 5; i < 16; i++){
    writeByte(data[i]);
  }
  delayMicroseconds(100);

  // Read one byte
  pinMode(DATA, INPUT);
  digitalWrite(BODY_ACK, LOW);
  waitLensHigh();
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
    //for(int i = 0; i < 1; i++){
    while(1){
      for(uint8 i = 0; i < 4; i++){
      delay(9);
      pulseShutter();
      delayMicroseconds(1500);
      standbyPacket();

      delay(4);
      digitalWrite(FOCUS, !digitalRead(FOCUS)); // Flip the focus pin
      }

      delay(9);
      pulseShutter();
      delayMicroseconds(700);
      standbyPacket();

    // Now send an extended packet with an aperture command
    uint8 one[] = {0x60, 0x80, 0xfe, 0x02, 0x00,
                   0x0a, 0x00,
                   0x01, 0xbd, 0x04, 0x00, 0x00,
                   0x00, 0x00, 0x00, 0x00, 0x00};
    extendedPacket(one);

      delay(4);
      digitalWrite(FOCUS, !digitalRead(FOCUS)); // Flip the focus pin
    }
    /*
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

    // Another extended packet
    delay(1);
    uint8 two[] = {0x60, 0x80, 0x04, 0xfe, 0x00,
                   0x0a, 0x00,
                   0x01, 0x00, 0x00, 0x00, 0x00,
                   0xff, 0x7f, 0x05, 0x00, 0x00};
    extendedPacket(one);
*/
  }


}
