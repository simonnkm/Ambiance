/**
 * @file    Ascii.h
 *
 * @author  Autonomous Systems Lab
 * @author  HARE Lab
 * @author  jLab
 *
 * @date    4 Jan 2025
 */
#ifndef ASCII_H
#define ASCII_H

#include <stdint.h>

/*  MODULE-LEVEL DEFINITIONS, MACROS    */
// Specify the height and width of the characters defined in this library.
#define ASCII_FONT_HEIGHT 8
#define ASCII_FONT_WIDTH  6

// Special codes.
#define UP_ARROW_OFF        '\x01'
#define UP_ARROW_ON         '\x02'
#define DOWN_ARROW_OFF      '\x03'
#define DOWN_ARROW_ON       '\x04'
#define LEFT_ARROW_OFF      '\x05'
#define LEFT_ARROW_ON       '\x06'
#define RIGHT_ARROW_OFF     '\x07'
#define RIGHT_ARROW_ON      '\x08'


/*  PROTOTYPES  */
/**
 * Pack a font for most of the standard ASCII characters into a byte array. Each character is stored
 * as ASCII_FONT_WIDTH number of bytes which each byte corresponding to a vertical line of 8 pixels
 * on the display.
 *
 * Some extended ASCII characters are supported:
 *  0xF8: Degree symbol
 *
 * Some custom characters have also been implemented in lieu of some non-printing characters:
 *  0x01: Top of oven, on
 *  0x02: Top of oven, off
 *  0x03: Bottom of oven, on
 *  0x04: Bottom of oven, off
 */
extern const uint8_t ascii[256][ASCII_FONT_WIDTH];


#endif /*   ASCII_H */
