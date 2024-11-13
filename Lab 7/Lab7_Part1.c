/*
 Lab7_Part1.c
 Ryan Simoneau
 Last Modified: Mar, 13 2021

 Description: This program implements a Finite Impulse Response (FIR) Filter (Low Pass)

*/

#include <F28x_Project.h>
#include <stdint.h>
#include "Lab4_Functions.h"
#include "AIC23.h"
#include "CommonDrivers.h"
interrupt void Mcbspb_isr(void);

// external interrupts for the push buttons
interrupt void PB1_isr(void);
//interrupt void PB2_isr(void);
//interrupt void PB3_isr(void);
interrupt void DIP1_isr(void);
interrupt void DIP2_isr(void);


// GPIO variables
Uint16 volatile PB1test = 0;
//Uint16 volatile PB2test = 0;
//Uint16 volatile PB3test = 0;

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

//FIR variables
Uint16 Taps = 42;
int32 volatile X0 = 0;
Uint16 Reset = 0;

// define arrays for both the coefficients
int16 X[42] = {0};
float32 C[42] = {-0.0068, -0.0044, -0.0053, -0.0058, -0.0058, -0.0050,
                   -0.0032, -0.0003, 0.0037, 0.0091, 0.0156, 0.0231,
                   0.0315, 0.0404, 0.0495, 0.0584, 0.0666, 0.0737,
                   0.0795, 0.0834, 0.0855, 0.0855, 0.0834, 0.0795,
                   0.0737, 0.0666, 0.0584, 0.0495, 0.0404, 0.0315,
                   0.0231, 0.0156, 0.0091, 0.0037, -0.0003, -0.0032,
                   -0.0050, -0.0058, -0.0058, -0.0053, -0.0044, -0.0068};


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

    // enable interrups for the push buttons and DIP switches
    GPIOintEnb();
    PieVectTable.XINT1_INT = &PB1_isr;
    //PieVectTable.XINT2_INT = &PB2_isr;
    //PieVectTable.XINT5_INT = &PB3_isr;
    IER |= M_INT1;
    IER |= M_INT12;

    EnableInterrupts(); // Enable PIE and CPU interrupts


    while(1)
    {

        while(Recieveflag == 0);
        Recieveflag = 0;
        // loop that will fill the buffer before doing any filtering
        for(Uint16 i = Taps - 1; i > 0; i--)
        {
            while(Recieveflag == 0);
            X[0] = X0;
            Recieveflag = 0;
            for(Uint16 j = Taps - 1; j > 0; j--)
            {

                X[j] = X[j - 1];
                testloop++;

            }

        }


        while(1)
        {

            while(Recieveflag == 0);
            X[0] = X0;
            Recieveflag = 0;
            for(Uint16 L = Taps - 1; L > 0; L--) // shift the entire buffer
            {

                X[L] = X[L - 1];

            }

            Output = (int32)(C[0]*(float32)X[0] + C[1]*(float32)X[1] + C[2]*(float32)X[2] + C[3]*(float32)X[3] + C[4]*(float32)X[4]  + C[5]*(float32)X[5]   +
                    C[6]*(float32)X[6]   + C[7]*(float32)X[7]   + C[8]*(float32)X[8]   + C[9]*(float32)X[9]   + C[10]*(float32)X[10] + C[11]*(float32)X[11] +
                    C[12]*(float32)X[12] + C[13]*(float32)X[13] + C[14]*(float32)X[14] + C[15]*(float32)X[15] + C[16]*(float32)X[16] + C[17]*(float32)X[17] +
                    C[18]*(float32)X[18] + C[19]*(float32)X[19] + C[20]*(float32)X[20] + C[21]*(float32)X[21] + C[22]*(float32)X[22] + C[23]*(float32)X[23] +
                    C[24]*(float32)X[24] + C[25]*(float32)X[25] + C[26]*(float32)X[26] + C[27]*(float32)X[27] + C[28]*(float32)X[28] + C[29]*(float32)X[29] +
                    C[30]*(float32)X[30] + C[31]*(float32)X[31] + C[32]*(float32)X[32] + C[33]*(float32)X[33] + C[34]*(float32)X[34] + C[35]*(float32)X[35] +
                    C[36]*(float32)X[36] + C[37]*(float32)X[37] + C[38]*(float32)X[38] + C[39]*(float32)X[39] + C[40]*(float32)X[40] + C[41]*(float32)X[41]);

            if(Reset == 1)
            {

                Reset = 0; // reset Reset
                break; // if the reset button (1) is pressed, break from the loop so that the buffer can be reloaded

            }

        }


    }

}



interrupt void PB1_isr(void){ // Button 1

    PB1test++;

    XintRegs.XINT1CR.bit.ENABLE = 0; // disable interrupts to avoid bouncing
    DELAY_US(5E3);
    XintRegs.XINT1CR.bit.ENABLE = 1; // re-enable after bouncing is gone

    Reset = 1;

    PieCtrlRegs.PIEACK.all = PIEACK_GROUP1;

}

interrupt void Mcbspb_isr(void) {


    test++;
    Recieveflag = 1;
    GpioDataRegs.GPATOGGLE.bit.GPIO22 = 1;

    // recieve data
    Ldata = McbspbRegs.DRR2.all;
    Rdata = McbspbRegs.DRR1.all;
    //data = (Uint16)(((float32)Ldata + (float32)Rdata) * 0.5); // average between the two values

    X0 = (Ldata);

    Loutput = (Output);
    Routput = (Output);

    // send data
    McbspbRegs.DXR2.all = Loutput;
    McbspbRegs.DXR1.all = Routput;


    PieCtrlRegs.PIEACK.all = PIEACK_GROUP6;

}
