/*
 * CommonDrivers.h
 *
 *  Created on: Mar 13, 2021
 *      Author: Ryan's PC
 */

#include <F28x_Project.h>
#include "Lab4_Functions.h"
#include "AIC23.h"

#ifndef COMMONDRIVERS_H_
#define COMMONDRIVERS_H_


#define CodecSPI_CLK_PULS {EALLOW; GpioDataRegs.GPASET.bit.GPIO18 = 1; GpioDataRegs.GPACLEAR.bit.GPIO18 = 1;}
#define CodecSPI_CS_LOW {EALLOW; GpioDataRegs.GPACLEAR.bit.GPIO19 = 1;}
#define CodecSPI_CS_HIGH {EALLOW; GpioDataRegs.GPASET.bit.GPIO19 = 1;}

// enable interrupts for Codec Push buttons and switches
void PBintEnb(void);

// enable interrupts for Codec
void McBSPbintEnb(void);

// a few initializations for codec including I2S or DSP mode
void InitAIC23(void);

// initialize the Codec McBSP
void InitMcBSPb(void);

// initializes SPIA
void InitSPIA(void);

// allows for a transfer on SPIA
void SpiTransmit(uint16_t data);

// initialize all of the push buttons, DIP switches, and LEDs on the Codec
void CODECGPIOINIT(void);


#endif /* COMMONDRIVERS_H_ */
