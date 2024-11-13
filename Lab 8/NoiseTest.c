/*
 Lab8_Quiz.c
 Ryan Simoneau
 Last Modified: Mar, 31 2021

 Description: This program is part 1 as fast as I can possibly make it

*/

#include <F28x_Project.h>
#include <stdint.h>
#include <math.h>
#include "fpu.h"
#include <complex.h>
#include "Lab4_Functions.h"
#include "AIC23.h"
#include "CommonDrivers.h"
#include "OneToOneI2CDriver.h"

interrupt void Mcbspb_isr(void);
void PowerFreqLCD(float32 power, float32 freq);

// McBSP variables
int16 Ldata   = 0;
int16 Rdata   = 0;
int16 data    = 0;
int32 Output  = 0;
int16 Loutput = 0;
int16 Routput = 0;

// Data variables
int16  X0 = 0;

// DFT variables
float32 ping[512]  = {0};
float32 pong[512]  = {0};
#pragma DATA_SECTION(ping, "ramgs0")
#pragma DATA_SECTION(pong, "ramgs1")

Uint16  pointer    = 0;
Uint16  DFTFlag    = 0; // 0: ping, 1: pong
Uint16  BufferFlag = 0; // 0: ping, 1: pong

float32 Xkreal      = 0;
float32 Xkimaginary = 0;
float32 Xksquare    = 0;
//#pragma DATA_SECTION(Xkreal, "ramgs2")
//#pragma DATA_SECTION(Xkimaginary, "ramgs3")
//#pragma DATA_SECTION(Xksquare, "ramgs4")

float32 max  = 0;
Uint16  bin  = 0;
float32 freq = 0;
float32 Mag  = 0;
float32 dB   = 0;

const float32 pi      = 3.1415926535;
const float32 ooft    = 0.0019531250;
const float32 FreqDel = 93.75;

// LCD variables
Uint16 LCDcount = 0;

 int main(void){

    InitSysCtrl();      // Set SYSCLK to 200 MHz, disables watchdog timer, turns on peripheral clocks

    EALLOW;
    I2C_O2O_Master_Init(0x27, 200.0, 12);
    LCDINIT();

    EALLOW;
    // enable SPIA and McBSPb along with the AIC23 and GPIO for DIP, PBs, and LEDs
    CODECGPIOINIT();
    SPIINIT();
    InitSPIA();
    InitAIC23();
    InitMcBSPb();

    DINT;               // Disable CPU interrupts on startup
    // Init PIE
    InitPieCtrl();      // Initialize PIE -> disable PIE and clear all PIE registers
    IER = 0x0000;       // Clear CPU interrupt register
    IFR = 0x0000;       // Clear CPU interrupt flag register
    InitPieVectTable(); // Initialize PIE vector table to known state

    EALLOW;
    // configure pin 22 as an output to test the interrupt frequency of the codec
    GpioCtrlRegs.GPAMUX2.bit.GPIO22 = 0;
    GpioCtrlRegs.GPAGMUX2.bit.GPIO22 = 0;
    GpioDataRegs.GPADAT.bit.GPIO22 = 0;
    GpioCtrlRegs.GPADIR.bit.GPIO22 = 1;

    GpioCtrlRegs.GPAMUX2.bit.GPIO29 = 0;
    GpioCtrlRegs.GPAGMUX2.bit.GPIO29 = 0;
    GpioDataRegs.GPADAT.bit.GPIO29 = 0;
    GpioCtrlRegs.GPADIR.bit.GPIO29 = 1;

    // enable interrupts from McBSPb
    McBSPbintEnb();
    PieVectTable.MCBSPB_RX_INT = &Mcbspb_isr;      // Assign ISR to PIE vector table
    IER |= M_INT6;                             // Enable INT6 in CPU
    EnableInterrupts(); // Enable PIE and CPU interrupts

    // Pre-calculate some of the argument of sine and cosine
    float32 arg = -1*2*pi*ooft;

    // can also make Xk--- all single numbers and not arrays


    // DFT for ping buffer
    while(1)
    {
        if(DFTFlag == 0 && BufferFlag == 0)
        {
            GpioDataRegs.GPATOGGLE.bit.GPIO22 = 1;
            for(Uint16 k = 0; k < 256; k++)
            {

                Xkreal = 0;
                Xkimaginary = 0;

                for(Uint16 n = 0; n < 512; n++)
                {

                    Xkreal      = Xkreal + ping[n]*cos(arg*(float32)k*(float32)n);
                    Xkimaginary = Xkimaginary + ping[n]*I*sin(arg*(float32)k*(float32)n);

                }

                Xksquare = Xkreal + Xkimaginary;
                Xksquare = 10*Xksquare*Xksquare;

                if(Xksquare > max) // need something that handles the first k value being the max value
                {

                    max = Xksquare; // output the square root of this number
                    bin = k;

                }

            }

            GpioDataRegs.GPATOGGLE.bit.GPIO22 = 1;
            DFTFlag = 1;
            freq    = FreqDel*(float32)bin;
            dB      = 10*log10(sqrt(max));
            max     = 0;

        }
        if(DFTFlag == 1 && BufferFlag == 1)
        {

            for(Uint16 k = 0; k < 256; k++)
            {

                Xkreal = 0;
                Xkimaginary = 0;

                for(Uint16 n = 0; n < 512; n++)
                {

                    Xkreal      = Xkreal + pong[n]*cos(arg*(float32)k*(float32)n);
                    Xkimaginary = Xkimaginary + pong[n]*I*sin(arg*(float32)k*(float32)n);

                }

                Xksquare = Xkreal + Xkimaginary;
                Xksquare = 10*Xksquare*Xksquare;

                if(Xksquare > max) // need something that handles the first k value being the max value
                {

                    max = Xksquare; // output the square root of this number
                    bin = k;

                }

            }

            DFTFlag = 0;
            freq    = FreqDel*(float32)bin;
            dB      = 10*log10(sqrt(max));
            max     = 0;

        }

        LCDcount++;

        //send to LCD
        if(LCDcount == 4)
        {

        PowerFreqLCD(dB, freq);
        LCDcount = 0;

        }

    }

}

interrupt void Mcbspb_isr(void) {

    GpioDataRegs.GPATOGGLE.bit.GPIO29 = 1;

    // recieve data
    Ldata = McbspbRegs.DRR2.all;
    Rdata = McbspbRegs.DRR1.all;
    X0 = Ldata;

    if(BufferFlag == 0) // DFT is being computed on ping
    {

       X0 = McbspbRegs.DRR2.all;
        pong[pointer] = (float32)X0;
        pointer++;

        if(pointer == 512)
        {

            pointer = 0;
            BufferFlag = 1;

        }

    }else
    {

        X0 = McbspbRegs.DRR2.all;
        ping[pointer] = (float32)X0;
        pointer++;

        if(pointer == 512)
        {

            pointer = 0;
            BufferFlag = 0;

        }

    }

    // send data
    //McbspbRegs.DXR2.all = 0; // do I still need to send data for this to work?
    //McbspbRegs.DXR1.all = 0;

    PieCtrlRegs.PIEACK.all = PIEACK_GROUP6;

}


 void PowerFreqLCD(float32 power, float32 freq){

    Uint16 Phund = 0;
    Uint16 Pten  = 0;
    Uint16 Pone  = 0;
    Uint16 PD1   = 0;

    Uint16 FTtho = 0;
    Uint16 Ftho  = 0;
    Uint16 Fhund = 0;
    Uint16 Ften  = 0;
    Uint16 Fone  = 0;
    Uint16 FD1   = 0;
    Uint16 FD2   = 0;


    FTtho = (Uint16)(freq*0.0001);
    Ftho  = (Uint16)(freq*0.001);
    Fhund = (Uint16)(freq*0.01);
    Ften  = (Uint16)(freq*0.1);


    // isolate the numbers of the power / frequency
    Phund = (Uint16)(power*0.01);
    Pten  = (Uint16)(power*0.1 - (10*(float32)Phund));
    Pone  = (Uint16)(power     - (100*(float32)Phund  + 10*(float32)Pten));
    PD1   = (Uint16)(10*power  - (1000*(float32)Phund + 100*(float32)Pten + 10*(float32)Pone));

    FTtho = (Uint16)(freq*0.0001);
    Ftho  = (Uint16)(freq*0.001 - (10*(float32)FTtho));
    Fhund = (Uint16)(freq*0.01  - (100*(float32)FTtho     + 10*(float32)Ftho));
    Ften  = (Uint16)(freq*0.1   - (1000*(float32)FTtho    + 100*(float32)Ftho    + 10*(float32)Fhund));
    Fone  = (Uint16)(freq       - (10000*(float32)FTtho   + 1000*(float32)Ftho   + 100*(float32)Fhund   + 10*(float32)Ften));
    FD1   = (Uint16)(freq*10    - (100000*(float32)FTtho  + 10000*(float32)Ftho  + 1000*(float32)Fhund  + 100*(float32)Ften  + 10*(float32)Fone));
    FD2   = (Uint16)(freq*100   - (1000000*(float32)FTtho + 100000*(float32)Ftho + 10000*(float32)Fhund + 1000*(float32)Ften + 100*(float32)Fone + 10*(float32)FD1));



    // convert each to ascii by adding 0x30 to each
    Phund = Phund + 0x30;
    Pten  = Pten  + 0x30;
    Pone  = Pone  + 0x30;
    PD1   = PD1   + 0x30;

    FTtho = FTtho + 0x30;
    Ftho  = Ftho  + 0x30;
    Fhund = Fhund + 0x30;
    Ften  = Ften  + 0x30;
    Fone  = Fone  + 0x30;
    FD1   = FD1   + 0x30;
    FD2   = FD2   + 0x30;

    // output the power and frequency to the LCD
    char LCDpower[14] = {'P', 'o', 'w', 'e', 'r', ':', Phund, Pten, Pone, '.', PD1, ' ', 'd', 'B'};
    char LCDfreq[16]  = {'F', 'r', 'e', 'q', ':', FTtho, Ftho, Fhund, Ften, Fone, '.', FD1, FD2, ' ', 'H', 'z'};
    // reset the LCD screen output (return Home)
    LCDCTRL(0x02);

    // output to the LCD
    LCDSTRING(LCDpower, 14);

    // go to the next line
    LCDCTRL(0xC0);

    LCDSTRING(LCDfreq,16);

}

 //Xkreal      = Xkreal + ping[n]*cos(arg*(float32)k*(float32)n);
 //Xkimaginary = Xkimaginary + ping[n]*I*sin(arg*(float32)k*(float32)n);

 //Xkreal      = Xkreal + pong[n]*cos(arg*(float32)k*(float32)n);
 //Xkimaginary = Xkimaginary + pong[n]*I*sin(arg*(float32)k*(float32)n);
