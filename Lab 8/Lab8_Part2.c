/*
 Lab8_Part2.c
 Ryan Simoneau
 Last Modified: Mar, 16 2021

 Description: This program implements a 512 point DFT utilizing pingpong registers and DMA

*/

#include <F28x_Project.h>
#include "fpu_rfft.h"
#include <stdint.h>
#include <math.h>
#include <complex.h>
#include "Lab4_Functions.h"
#include "AIC23.h"
#include "CommonDrivers.h"
#include "OneToOneI2CDriver.h"

void PowerFreqLCD(float32 power, float32 freq);

// DFT variables
volatile int16 ping[512]  = {0};
volatile int16 pong[512]  = {0};
#pragma DATA_SECTION(ping, "ramgs0")
#pragma DATA_SECTION(pong, "ramgs1")


Uint16  pointer    = 0;
Uint16  DFTFlag    = 0; // 0: ping, 1: pong
Uint16  BufferFlag = 0; // 0: ping, 1: pong

float32 Xkreal[512]      = {0};
float32 Xkimaginary[512] = {0};
float32 Xksquare[512]    = {0};
#pragma DATA_SECTION(Xkreal, "ramgs2")
#pragma DATA_SECTION(Xkimaginary, "ramgs3")
#pragma DATA_SECTION(Xksquare, "ramgs4")

float32 max  = 0;
Uint16  bin  = 0;
float32 freq = 0;
float32 Mag  = 0;
float32 dB   = 0;

const float32 N       = 128;
const float32 pi      = 3.1415926535;
const float32 FreqDel = 93.75;

// LCD variables
Uint16 LCDcount = 0;

//BURST is number of burst - 1 in an xfer
//TRANSFER is number of bursts - 1 in an xfer
#define BURST 1
#define TRANSFER 511


interrupt void local_D_INTCH6_ISR(void);

int main(void){

    InitSysCtrl();      // Set SYSCLK to 200 MHz, disables watchdog timer, turns on peripheral clocks

    EALLOW;
    DINT;               // Disable CPU interrupts on startup
    // Init PIE
    InitPieCtrl();      // Initialize PIE -> disable PIE and clear all PIE registers
    IER = 0x0000;       // Clear CPU interrupt register
    IFR = 0x0000;       // Clear CPU interrupt flag register
    InitPieVectTable(); // Initialize PIE vector table to known state

    EALLOW;
    // enable SPIA and McBSPb along with the AIC23 and GPIO for DIP, PBs, and LEDs

    InitSPIA();
    InitAIC23();
    InitMcBSPb();

    EALLOW;
    // Set DMA CH6 transfer complete interrupts to ISR
    PieVectTable.DMA_CH6_INT= &local_D_INTCH6_ISR;

    // Performs a hard reset on the DMA
    DMAInitialize();

    // source and destination pointers
    volatile Uint16 *DMA_CH6_Source = (volatile Uint16 *)&McbspbRegs.DRR2.all;
    volatile Uint16 *DMA_CH6_Dest = (volatile Uint16 *)&pong; // is the destination going to have to be ping and pong or their addresses

    // Initialize CH6 source and destination addresses
    DMACH6AddrConfig(DMA_CH6_Dest,DMA_CH6_Source); // may need this in the ISR //

    // Configures the burst size and source/destination step size
    // Burst size is 2 16-bit words (1 + 1)
    // Source address increments by 0 after word is transmitted (DRR2 -> DRR1)
    // Destination address increments by 1 after word is transmitted (buff[1] -> buff[2])
    DMACH6BurstConfig(BURST,1,1); // does not increment between words //

    // Configures the transfer size and source/destination step size
    // Transfer size is 1 burst (0 + 1)
    DMACH6TransferConfig(TRANSFER,-1,0); // increments between bursts //

    // Configures source and destination wrapping
    // Source wrapping doesn't matter -> set to 0xFFFF so it's ignored
    // Destination wrapping doesn't matter -> set to 0xFFFF so it's ignored
    DMACH6WrapConfig(0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF); // don't need this, but maybe could be used for ping ponging

    // CH6 mode configuration:
    // Burst triggers after McBSPb RX, oneshot disabled, continuous mode enable
    // 16-bit data, interrupt enabled, interrupt triggers at end of transfer
    DMACH6ModeConfig(74,PERINT_ENABLE,ONESHOT_DISABLE,CONT_ENABLE,
                     SYNC_DISABLE,SYNC_SRC,OVRFLOW_DISABLE,SIXTEEN_BIT,
                     CHINT_END,CHINT_ENABLE);

    // Dual ported bridge connected to DMA
    EALLOW;
    CpuSysRegs.SECMSEL.bit.PF2SEL = 1;
    EDIS;

    // Interrupt enabling
    // PIE group 7, interrupt 6 -> DMA CH6
    PieCtrlRegs.PIEIER7.bit.INTx6 = 1;
    IER |= M_INT7;

    CODECGPIOINIT();
    // configure pin 22 as an output to test the interrupt frequency of the codec
    GpioCtrlRegs.GPAMUX2.bit.GPIO22 = 0;
    GpioCtrlRegs.GPAGMUX2.bit.GPIO22 = 0;
    GpioDataRegs.GPADAT.bit.GPIO22 = 0;
    GpioCtrlRegs.GPADIR.bit.GPIO22 = 1;

    GpioCtrlRegs.GPAMUX2.bit.GPIO29 = 0;
    GpioCtrlRegs.GPAGMUX2.bit.GPIO29 = 0;
    GpioDataRegs.GPADAT.bit.GPIO29 = 0;
    GpioCtrlRegs.GPADIR.bit.GPIO29 = 1;

    // Enables the PIE
    // Clears PIEACK register
    // Enables global interrupts
    EnableInterrupts();

    // Start DMA CH6
    StartDMACH6();

    // DFT
    while(1)
    {
        if(DFTFlag == 0 && BufferFlag == 0)
        {
            GpioDataRegs.GPATOGGLE.bit.GPIO22 = 1;
            for(Uint16 k = 0; k < N/2; k++)
            {

                Xkreal[k] = 0;
                Xkimaginary[k] = 0;

                for(Uint16 n = 0; n < 512; n++)
                {

                    Xkreal[k]      = Xkreal[k] + (float32)ping[n]*(cos((-1*2*pi*(float32)k*(float32)n)/N));
                    Xkimaginary[k] = Xkimaginary[k] + (float32)ping[n]*I*(sin((-1*2*pi*(float32)k*(float32)n)/N));

                }

                Xksquare[k] = pow(Xkreal[k] + Xkimaginary[k], 2);

                if(Xksquare[k] > max) // need something that handles the first k value being the max value
                {

                    max = Xksquare[k]; // output the square root of this number
                    bin = k;

                }

            }

            GpioDataRegs.GPATOGGLE.bit.GPIO22 = 1;
            DFTFlag = 1;
            freq    = (48000/N)*(float32)bin;
            Mag     = sqrt(max);
            dB      = 10*log(Mag);
            max     = 0;

        }
        if(DFTFlag == 1 && BufferFlag == 1)
        {

            for(Uint16 k = 0; k < N/2; k++)
            {

                Xkreal[k] = 0;
                Xkimaginary[k] = 0;

                for(Uint16 n = 0; n < 512; n++)
                {

                    Xkreal[k]      = Xkreal[k] + (float32)pong[n]*(cos((-1*2*pi*(float32)k*(float32)n)/N));
                    Xkimaginary[k] = Xkimaginary[k] + (float32)pong[n]*I*(sin((-1*2*pi*(float32)k*(float32)n)/N));

                }

                Xksquare[k] = pow(Xkreal[k] + Xkimaginary[k], 2);

                if(Xksquare[k] > max) // need something that handles the first k value being the max value
                {

                    max = Xksquare[k]; // output the square root of this number
                    bin = k;

                }

            }

            DFTFlag = 0;
            freq    = (48000/N)*(float32)bin;
            Mag     = sqrt(max);
            dB      = 10*log10(Mag);
            max     = 0;

        }

    }

}

interrupt void local_D_INTCH6_ISR(void)
{
    EALLOW;

    GpioDataRegs.GPATOGGLE.bit.GPIO29 = 1; // each time a buffer has been filled, the interrupt triggers

    if(BufferFlag == 0)
    {

        BufferFlag = 1;
        volatile Uint16 *DMA_CH6_Source = (volatile Uint16 *)&McbspbRegs.DRR2.all;
        volatile Uint16 *DMA_CH6_Dest = (volatile Uint16 *)&ping;
        DMACH6AddrConfig(DMA_CH6_Dest,DMA_CH6_Source);

    }else
    {

        BufferFlag = 0;
        volatile Uint16 *DMA_CH6_Source = (volatile Uint16 *)&McbspbRegs.DRR2.all;
        volatile Uint16 *DMA_CH6_Dest = (volatile Uint16 *)&pong;
        DMACH6AddrConfig(DMA_CH6_Dest,DMA_CH6_Source);

    }

    // ACK to receive more interrupts
    PieCtrlRegs.PIEACK.all |= PIEACK_GROUP7;

    EDIS;
}
