/* common.h

 * Code and definitions that are common to the lens and body faking code.
 * Steven Bell <sebell@stanford.edu>
 * 26 September 2012
 */

/* Pin numbers refer to the labels on the Arduino Mega 2560 board.  These
 * numbers are used with the Arduino library.
 *
 * PORTX and PINX refer to the registers which correspond to those pins.
 * PORT holds output values, PIN holds input values.
 */

#define SLEEP 45 // Port L 4
#define BODY_ACK 46 // Port L 3
#define BODY_ACK_PIN PINL

#define LENS_ACK 47 // Port L 2
#define LENS_ACK_PORT PORTL
#define LENS_ACK_PIN PINL

#define CLK 52 // Port B 1
#define CLK_PORT PORTB
#define CLK_PIN PINB

// Since the camera has only a single data line, MOSI and MISO are tied
// together externally.  For bit-banging, we'll just use MISO.
#define DATA_MISO 50 // Port B 3
#define DATA_MOSI 51 // Port B 2
#define DATA DATA_MISO

#define DATA_PORT PORTB
#define DATA_PIN PINB // PIN holds inputs
#define DATA_DIR DDRB

#define FOCUS 49 // Port L 0
#define SHUTTER 48 // Port L 1

#define SPI_SS 53 // When low, the slave SPI is enabled

// Useful bitmasks for manipulating the IO pins
// Bitwise OR these with port registers to set pins high
const uint8 CLK_HIGH = 0b00000010; // Port B 1
const uint8 DATA_MOSI_HIGH = 0b00000100; // Port B 2
const uint8 DATA_MISO_HIGH = 0b00001000; // Port B 3
#define DATA_HIGH DATA_MISO_HIGH

// Bitwise AND these with port registers to set pins low
const uint8 CLK_LOW = ~CLK_HIGH;
const uint8 DATA_LOW = ~DATA_HIGH;

// Bitwise AND these with port registers to read the pin
const uint8 LENS_ACK_HIGH = 0b00000100; // Port L 2
const uint8 BODY_ACK_HIGH = 0b00001000; // Port L 3

// Bitwise OR these with port DDR registers to set outputs
const uint8 CLK_WRITE = 0b00000010; // Port B 1
const uint8 DATA_MOSI_WRITE = 0b00000100; // Port B 2
const uint8 DATA_MISO_WRITE = 0b00001000; // Port B 3
#define DATA_WRITE DATA_MISO_WRITE

// Bitwise AND these with port DDR registers to set inputs
const uint8 DATA_READ = ~DATA_WRITE;
