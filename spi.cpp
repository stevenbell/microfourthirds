/* spi.cpp
 * Code to use the SPI hardware on the Atmega 2560 for communication with
 * the camera body or lens, rather than bit-banging.
 * Steven Bell <sebell@stanford.edu>
 * 3 October 2012
 */

#include "Arduino.h"
#include "typedef.h"

// Pins:
#define PIN_MISO 50 // Master out, slave in
#define PIN_MOSI 51 // Master in, slave out
#define PIN_CLK  52 // Clock, driven by master
#define PIN_SS   53 // When low, the slave SPI is enabled

int main()
{
  init();


  // Configuration:
  pinMode(PIN_CLK, OUTPUT); // Set CLK as an output
  pinMode(PIN_SS, OUTPUT); // Don't use SS, so force it as an output

  // Set the SPI hardware to master mode
  // SPE - Enable
  // DORD - Set data order to LSB-first
  // MSTR - Master mode
  // CPOL - Set clock polarity to "normally high"
  // CPHA - Set to read on trailing (rising) edge
  // SPR1 - Set clock frequency to 1/64 Fosc (but doubled below)

  SPCR = (1<<SPE) | (1<<DORD) | (1<<MSTR) | (1<<CPOL) | (1<<CPHA) | (1<<SPR1);

  // Set clock doubling to get 1/32 Fosc = 500kHz
  SPSR |= (1 << SPI2X);

  // MISO is automatically an input

  // Sending a byte:
  // Set MOSI as an output

  // Receiving a byte
  // Set MOSI as an input (or high-z)


  while(1){
  // Send one byte
    pinMode(PIN_MOSI, OUTPUT);
    SPDR = 0x88;
    delay(1);
  }

}

