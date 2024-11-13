/*
 Lab7_Part1.c
 Ryan Simoneau
 Last Modified: Mar, 16 2021

 Description: This program implements a Infinite Impulse Response (IIR) Filter (Band Pass)

*/

#include <F28x_Project.h>
#include <stdint.h>
#include "Lab4_Functions.h"
#include "AIC23.h"
#include "CommonDrivers.h"
interrupt void Mcbspb_isr(void);

// GPIO variables
Uint16 volatile PB1test = 0;

// McBSP variables
int16 Ldata = 0;
int16 Rdata = 0;
int16 data  = 0;
int32 Output = 0;
int16 Loutput = 0;
int16 Routput = 0;

// Data variables
Uint16 volatile Recieveflag = 0;


Uint16 test  = 0;
Uint16 testloop = 0;

//IIR coefficient variables
Uint16 Buffsize = 3;

float32 B0n[6] = {0.0249, 0.0315, 0.0199, 0.0227, 0.0378, 0.0351};
float32 B2n[6] = {-0.0249, -0.0315, -0.0199, -0.0227, -0.0378, -0.0351};
float32 B1  = 0;
float32 A0  = 1;//   A11      A12      A13      A14      A15      A16
float32 A1n[6] = {-1.9410, -1.9345, -1.9878, -1.9692, -1.9680, -1.9478}; // stage 1 gets A1n[1], stage 2 gets A1n[2] and so on
float32 A2n[6] = {0.9503, 0.9411, 0.9905, 0.9804, 0.9711, 0.9521}; // works the same as A1n

// IIR input/output variables
int32 volatile X0 = 0;

// stage 1
float32 X1[3] = {0};
float32 Y1[3] = {0};

// stage 2
float32 X2[3] = {0};
float32 Y2[3] = {0};

// stage 3
float32 X3[3] = {0};
float32 Y3[3] = {0};

// stage 4
float32 X4[3] = {0};
float32 Y4[3] = {0};

// stage 5
float32 X5[3] = {0};
float32 Y5[3] = {0};

// stage 6
float32 X6[3] = {0};
float32 Y6[3] = {0};



int main(void){

    InitSysCtrl();      // Set SYSCLK to 200 MHz, disables watchdog timer, turns on peripheral clocks

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

    // enable interrupts from McBSPb
    McBSPbintEnb();
    PieVectTable.MCBSPB_RX_INT = &Mcbspb_isr;      // Assign ISR to PIE vector table
    IER |= M_INT6;                             // Enable INT6 in CPU
    EnableInterrupts(); // Enable PIE and CPU interrupts


    while(1)
    {

        // wait for an input from the codec
        while(Recieveflag == 0);
        Recieveflag = 0;

        // Stage 1 //
        X1[0] = (float32)X0;
        Y1[0] = B0n[0]*X1[0] + B2n[0]*X1[2] - A1n[0]*Y1[1] - A2n[0]*Y1[2];

        // Stage 2 //
        X2[0] = Y1[0];
        Y2[0] = B0n[1]*X2[0] + B2n[1]*X2[2] - A1n[1]*Y2[1] - A2n[1]*Y2[2];

        // Stage 3 //
        X3[0] = Y2[0];
        Y3[0] = B0n[2]*X3[0] + B2n[2]*X3[2] - A1n[2]*Y3[1] - A2n[2]*Y3[2];

        // Stage 4 //
        X4[0] = Y3[0];
        Y4[0] = B0n[3]*X4[0] + B2n[3]*X4[2] - A1n[3]*Y4[1] - A2n[3]*Y4[2];

        // Stage 5 //
        X5[0] = Y4[0];
        Y5[0] = B0n[4]*X5[0] + B2n[4]*X5[2] - A1n[4]*Y5[1] - A2n[4]*Y5[2];

        // Stage 6 //
        X6[0] = Y5[0];
        Y6[0] = B0n[5]*X6[0] + B2n[5]*X6[2] - A1n[5]*Y6[1] - A2n[5]*Y6[2];

        // send Y6[0] to the output
        Output = (int32)Y6[0];

        // for loop to shift all the buffer values after Y6[0] is output
        for(Uint16 i = Buffsize - 1; i > 0; i--)
        {

            // Stage 1 //
            X1[i] = X1[i - 1];
            Y1[i] = Y1[i - 1];

            // Stage 2 //
            X2[i] = X2[i - 1];
            Y2[i] = Y2[i - 1];

            // Stage 3 //
            X3[i] = X3[i - 1];
            Y3[i] = Y3[i - 1];

            // Stage 4 //
            X4[i] = X4[i - 1];
            Y4[i] = Y4[i - 1];

            // Stage 5 //
            X5[i] = X5[i - 1];
            Y5[i] = Y5[i - 1];

            // Stage 6 //
            X6[i] = X6[i - 1];
            Y6[i] = Y6[i - 1];

        }

    }

}

interrupt void Mcbspb_isr(void) {


    test++;
    Recieveflag = 1;
    GpioDataRegs.GPATOGGLE.bit.GPIO22 = 1;

    // recieve data
    Ldata = McbspbRegs.DRR2.all;
    Rdata = McbspbRegs.DRR1.all;
    X0 = Ldata;

    Loutput = (Output);
    Routput = (Output);

    // send data
    McbspbRegs.DXR2.all = Loutput;
    McbspbRegs.DXR1.all = Routput;

    PieCtrlRegs.PIEACK.all = PIEACK_GROUP6;

}
