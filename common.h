/* common.h
 * Code and definitions that are common to the lens and body faking code.
 * Steven Bell <sebell@stanford.edu>
 * 26 September 2012
 */

#define SLEEP 53 // Port B 0
#define BODY_ACK 51 // Port B 2
#define BODY_ACK_PIN PINB

#define LENS_ACK 49 // Port L 0
#define LENS_ACK_PORT PORTL
#define LENS_ACK_PIN PINL

#define CLK 45 // Port L 4
#define CLK_PORT PORTL
#define CLK_PIN PINL

#define DATA 47 // Port L 2
#define DATA_PORT PORTL // PORT holds outputs
#define DATA_PIN PINL // PIN holds inputs
#define DATA_DIR DDRL

#define FOCUS 41
#define SHUTTER 43

// Useful bitmasks for manipulating the IO pins
// Bitwise OR these with port registers to set pins high
const uint8 CLK_HIGH = 0b00010000;
const uint8 DATA_HIGH = 0b00000100;

// Bitwise AND these with port registers to read the pin
const uint8 LENS_ACK_HIGH = 0b00000001; // Port L 0
const uint8 BODY_ACK_HIGH = 0b00000100; // Port B 2

const uint8 CLK_WRITE = 0b00010000;
const uint8 DATA_WRITE = 0b00000100;

// Bitwise AND these with port registers to set pins low
const uint8 CLK_LOW = ~CLK_HIGH;
const uint8 DATA_LOW = ~DATA_HIGH;

const uint8 DATA_READ = DATA_LOW;
