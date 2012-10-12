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
  pinMode(FOCUS, INPUT);
  pinMode(SHUTTER, INPUT);

  // Configure the SPI hardware
  // SPE - Enable
  // DORD - Set data order to LSB-first
  // Slave mode (Master bit is not set)
  // CPOL - Set clock polarity to "normally high"
  // CPHA - Set to read on trailing (rising) edge
  SPCR = (1<<SPE) | (1<<DORD) | (1<<CPOL) | (1<<CPHA);

  // Set up the SPI pins
  pinMode(CLK, INPUT);
  pinMode(DATA_MISO, INPUT); // Until we have an explicit write, make both inputs
  pinMode(DATA_MOSI, INPUT);
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
  SPCR = (1<<SPE) | (1<<DORD) | (1<<CPOL) | (1<<CPHA);
  pinMode(DATA_MISO, INPUT); // Just in case it was an output last

  // Clear the SPIF bit from any previously received bytes by reading SPDR
  SPDR = 0x00;

  // Wait until we receive a byte
  while(!(SPSR & (1<<SPIF))) {}

  SPCR = 0x00;
  return(SPDR);
}

/* Writes an 8-bit value on the data bus.  The clock is driven by the body.
 */
void writeByte(uint8 value)
{
  SPCR = (1<<SPE) | (1<<DORD) | (1<<CPOL) | (1<<CPHA);
  // Set the bytes we want to write
  SPDR = value;

  // Set the MISO pin to be an output
  DATA_DIR |= DATA_MISO_WRITE;

  // Wait until transmission is finished
  while(!(SPSR & (1<<SPIF))) {}

  // Clear SPIF
  // BUG: When this was set to 0x00, it didn't do anything.  Perhaps it was optimized away?
  SPDR = 0xFF;
  SPCR = 0x00;
}

/* Reads a number of bytes and then transmits the checksum.
 * nBytes - Number of bytes to read.  Must be greater than zero.
 */
void readBytesChecksum(uint8 nBytes, uint8* values)
{
  uint8 checksum = 0;

  for(uint8 i = 0; i < nBytes - 1; i++){
    values[i] = readByte();
    checksum += values[i];
    digitalWrite(LENS_ACK, 0); // Working
    digitalWrite(LENS_ACK, 1); // Ready
  }

  // Last byte
  values[nBytes - 1] = readByte();
  checksum += values[nBytes - 1];
  digitalWrite(LENS_ACK, 0); // Working
  // Note: No ready here, we're waiting for the body to drop

  // Now we reply with the checksum
  waitBodyFall();
  digitalWrite(LENS_ACK, 1); // Ready
  waitBodyHigh();
  writeByte(checksum);
}

// # bytes, bytes, checksum
void writeBytesChecksum(uint8 nBytes, uint8* values)
{
  uint8 checksum = 0;

  // Write the first byte, which is the number of bytes in the packet
  //waitBodyFall(); // Wait for body to drop
  //digitalWrite(LENS_ACK, 0); // We drop and then rise (ready to send next byte)
  digitalWrite(LENS_ACK, 1);
  writeByte(nBytes);

  // Now write the byte values themselves
  for(uint8 i = 0; i < nBytes; i++){
    waitBodyLow();
    digitalWrite(LENS_ACK, 0);
    digitalWrite(LENS_ACK, 1);
    writeByte(values[i]);
    checksum += values[i]; // Keep a running total
  }

  // Finally, write the checksum
  waitBodyLow();
  digitalWrite(LENS_ACK, 0);
  digitalWrite(LENS_ACK, 1);
  writeByte(checksum);
}

void standbyPacket(void)
{

  uint8 values[31] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
  writeBytesChecksum(31, values);
}

int main()
{
  init(); // Arduino library init
  setup(); // Pin setup

  // Sit and wait for the sleep pin to go high (camera is turned on)
  while(digitalRead(SLEEP) == 0){}

  // Check that the body ACK pin is high
  waitBodyHigh();

  // Pulse our ACK pin to let the body know we're awake
  digitalWrite(LENS_ACK, 1);
  delay(10);
  digitalWrite(LENS_ACK, 0);

  while(1){
    // Wait until the body ACK goes high
    waitBodyRise();

    // Ready
    digitalWrite(LENS_ACK, 1);

    // Read four bytes
    uint32 commandBytes;
    readBytesChecksum(4, (uint8*)&commandBytes);

    waitBodyFall();
    digitalWrite(LENS_ACK, 0);

    switch(commandBytes){
    // Note that all of the case values have the bytes in reverse order from
    // the way they are transmitted.
    case 0x0000f2b0:
      // There's a extra fall-rise sequence for some reason
      waitBodyRise();
      digitalWrite(LENS_ACK, 1);
      delay(500);

      digitalWrite(LENS_ACK, 0);
      waitBodyLow(); // Falling edge happens very fast
      digitalWrite(LENS_ACK, 1);
      waitBodyRise();

      writeByte(0x00);
      break;

    case 0x0000f6c0:
      {
      uint8 sendBytes[5] = {0x00, 0x0a, 0x10, 0xc4, 0x09};
      writeBytesChecksum(5, sendBytes);
      }
      break;

    case 0x0001f5a0:
      // A0 F5 01 00 is followed by dropping the clock pin for a ms.  This
      // ruins the SPI hardware synchronization, so reset it here.
      // There is no response for this command.
      SPCR = 0;
      while(!(CLK_PIN & CLK_HIGH)){} // Wait for clock to be high again
      //delayMicroseconds(10); // TODO: There must be a better way.
      Serial.write("reset!");
      SPCR = 0;
      SPCR = (1<<SPE) | (1<<DORD) | (1<<CPOL) | (1<<CPHA);
      break;

    case 0x0000f9c1:
      {
          // Information contained in here:
  // Aperture limits, focus limits, zoom?
  // Firmware version
  // Vendor
  // # bytes, bytes, checksum
  uint8 sendBytes2[21] = {0x00, 0x00, 0x00, 0x01, 0x10, 0x00, 0x00, 0x41,
                          0x41, 0x41, 0x32, 0x34, 0x33, 0x38, 0x34, 0x31,
                          0x00, 0x00, 0x00, 0x01, 0x11};
  writeBytesChecksum(21, sendBytes2);

      }
      break;

    case 0x0000f0c3:
      // Appears to be some kind of firmware dump
        waitBodyLow();
        digitalWrite(LENS_ACK, 0);
        digitalWrite(LENS_ACK, 1);
        writeByte(0xBF);

        waitBodyLow();
        digitalWrite(LENS_ACK, 0);
        digitalWrite(LENS_ACK, 1);
        writeByte(0x08);

      for(uint16 i = 0; i < 0x08BF; i++){
        waitBodyLow();
        digitalWrite(LENS_ACK, 0);
        digitalWrite(LENS_ACK, 1);
        writeByte(0x00);
      }

    case 0x0000f3c2:

      /*
    //case 0x0000f3cf:
      {
      uint8 sendBytes[4] = {0x00, 0x00, 0x00, 0x00};
      writeBytesChecksum(4, sendBytes);
      }
      break;
      */
    default:
      Serial.print("Unknown: ");
      Serial.println(commandBytes, HEX);
    }
    waitBodyLow(); // We may have missed the edge, just wait for low
    digitalWrite(LENS_ACK, 0);

    // Set the data pin low and let go of it
    digitalWrite(DATA_MISO, 0);
    pinMode(DATA_MISO, INPUT);
  }
/*



  readBytesChecksum(4);

  waitBodyLow();
  digitalWrite(LENS_ACK, 0);
  waitBodyHigh();
  digitalWrite(LENS_ACK, 1);

  // The body drops the clock for some unknown reason, ruining the
  // SPI line synchronization.  Reset the hardware to fix it.

  readBytesChecksum(4);


  waitBodyLow();
  digitalWrite(LENS_ACK, 0);
  waitBodyHigh();
  digitalWrite(LENS_ACK, 1);

  readBytesChecksum(4);

  // The body expects some bytes here...
  writeBytesChecksum(2, sendBytes2);

  waitBodyLow();
  digitalWrite(LENS_ACK, 0);
  waitBodyHigh();
  digitalWrite(LENS_ACK, 1);

  readBytesChecksum(4);

  waitBodyLow();
  digitalWrite(LENS_ACK, 0);
  delay(10);
  digitalWrite(LENS_ACK, 1);

  while(1){
    //standbyPacket();
  }
  */
  return(0);
}
