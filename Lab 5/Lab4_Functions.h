/*
 * Lab4_Functions.h
 *
 *  Created on: Feb 15, 2021
 *      Author: Ryan's PC
 */

#ifndef LAB4_FUNCTIONS_H_
#define LAB4_FUNCTIONS_H_

// send single commands to the LCD
void LCDCTRL(Uint16 command);

// Function to send single characters to the LCD
void LCDDATA(Uint16 data);

// function with hardcodded values meant to initialize the LCD
void LCDINIT(void);

// function to send string to the LCD
void LCDSTRING(char * const string, Uint16 length);

// initializes the SPI system
void SPIINIT(void);

// send data to the SRAMs via SPIA
Uint16 SPITRANSMIT(Uint16 data);

// write a 16 bit word to an SRAM module
void SRAMWRITE(Uint16 data, Uint32 address);

// read a 16 bit word from an SRAM module
Uint16 SRAMREAD(Uint32 address);

#endif /* LAB4_FUNCTIONS_H_ */
