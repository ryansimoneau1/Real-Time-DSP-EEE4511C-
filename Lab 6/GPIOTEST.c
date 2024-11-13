/*
 Lab5_Part2.c
 Ryan Simoneau
 Last Modified: Feb, 22 2021

 Description: This program completes a send and recieve test with the codec board

*/

#include <F28x_Project.h>
#include "Lab4_Functions.h"

// external interrupts for the push buttons
interrupt void PB1_isr(void);
interrupt void PB2_isr(void);
interrupt void PB3_isr(void);

void CODECGPIOINIT(void);
void PBintEnb(void);

Uint16 volatile DIP = 0;
Uint16 volatile PBs = 0;
Uint16 volatile LEDs = 0;
Uint32 volatile PBDIP = 0;

// Push button interrupt test variables
Uint16 volatile PB1test = 0;
Uint16 volatile PB2test = 0;
Uint16 volatile PB3test = 0;

int main(void){

    InitSysCtrl();      // Set SYSCLK to 200 MHz, disables watchdog timer, turns on peripheral clocks

    DINT;               // Disable CPU interrupts on startup

    // Init PIE
    InitPieCtrl();      // Initialize PIE -> disable PIE and clear all PIE registers
    IER = 0x0000;       // Clear CPU interrupt register
    IFR = 0x0000;       // Clear CPU interrupt flag register
    InitPieVectTable(); // Initialize PIE vector table to known state

    EALLOW;
    CODECGPIOINIT();
    PBintEnb();

    while(1){

        DIP = (Uint16)(GpioDataRegs.GPADAT.all & 0xF);
        PBs = (Uint16)((GpioDataRegs.GPADAT.all >> 14) & 0x7);
        LEDs = ((PBs << 4) | DIP);
        PBDIP = (Uint32)(LEDs << 4);
        GpioDataRegs.GPADAT.all = PBDIP;

    }




}

interrupt void PB1_isr(void){

    PB1test++;

    PieCtrlRegs.PIEACK.all = PIEACK_GROUP1;

}

interrupt void PB2_isr(void){

    PB2test++;

    PieCtrlRegs.PIEACK.all = PIEACK_GROUP1;

}

interrupt void PB3_isr(void){

    PB3test++;

    PieCtrlRegs.PIEACK.all = PIEACK_GROUP12;

}




void PBintEnb(void){

    GPIO_SetupXINT1Gpio(14);
    GPIO_SetupXINT2Gpio(15);
    GPIO_SetupXINT3Gpio(16);
    EALLOW;

    PieCtrlRegs.PIEIER1.bit.INTx4 = 1;
    PieCtrlRegs.PIEIER1.bit.INTx5 = 1;
    PieCtrlRegs.PIEIER12.bit.INTx1 = 1;

    // enable all 3
    XintRegs.XINT1CR.bit.POLARITY = 0; // negative edge triggered
    XintRegs.XINT2CR.bit.POLARITY = 0; // negative edge triggered
    XintRegs.XINT3CR.bit.POLARITY = 0; // negative edge triggered

    XintRegs.XINT1CR.bit.ENABLE = 1;
    XintRegs.XINT2CR.bit.ENABLE = 1;
    XintRegs.XINT3CR.bit.ENABLE = 1;


    PieVectTable.XINT1_INT = &PB1_isr;
    PieVectTable.XINT2_INT = &PB2_isr;
    PieVectTable.XINT3_INT = &PB3_isr;

    IER |= M_INT1;
    IER |= M_INT12;
    EnableInterrupts();                         // Enable PIE and CPU interrupts

}

void CODECGPIOINIT(void){

    //Initialize the DIP switches, Buttons, and LEDs
    // DIP: GPIO[3:0] (4 wide)
    // PBs: GPIO[16:14] (3 wide)
    // LEDs: GPIO[11:4] (8 wide)

    EALLOW;
    // DIP switches
    GpioCtrlRegs.GPAMUX1.bit.GPIO3 = 0; // sets pins as GPIO
    GpioCtrlRegs.GPAMUX1.bit.GPIO2 = 0;
    GpioCtrlRegs.GPAMUX1.bit.GPIO1 = 0;
    GpioCtrlRegs.GPAMUX1.bit.GPIO0 = 0;
    GpioCtrlRegs.GPAGMUX1.bit.GPIO3 = 0;
    GpioCtrlRegs.GPAGMUX1.bit.GPIO2 = 0;
    GpioCtrlRegs.GPAGMUX1.bit.GPIO1 = 0;
    GpioCtrlRegs.GPAGMUX1.bit.GPIO0 = 0;

    GpioCtrlRegs.GPAPUD.bit.GPIO3 = 0; // enables pull up resistors
    GpioCtrlRegs.GPAPUD.bit.GPIO2 = 0;
    GpioCtrlRegs.GPAPUD.bit.GPIO1 = 0;
    GpioCtrlRegs.GPAPUD.bit.GPIO0 = 0;

    GpioCtrlRegs.GPADIR.bit.GPIO3 = 0; // set as input to the uProcessor
    GpioCtrlRegs.GPADIR.bit.GPIO2 = 0;
    GpioCtrlRegs.GPADIR.bit.GPIO1 = 0;
    GpioCtrlRegs.GPADIR.bit.GPIO0 = 0;


    // Push Buttons
    GpioCtrlRegs.GPAMUX2.bit.GPIO16 = 0; // sets pins as GPIO
    GpioCtrlRegs.GPAMUX1.bit.GPIO15 = 0;
    GpioCtrlRegs.GPAMUX1.bit.GPIO14 = 0;
    GpioCtrlRegs.GPAGMUX2.bit.GPIO16 = 0;
    GpioCtrlRegs.GPAGMUX1.bit.GPIO15 = 0;
    GpioCtrlRegs.GPAGMUX1.bit.GPIO14 = 0;

    GpioCtrlRegs.GPAPUD.bit.GPIO16 = 0; // enables pull up resistors
    GpioCtrlRegs.GPAPUD.bit.GPIO15 = 0;
    GpioCtrlRegs.GPAPUD.bit.GPIO14 = 0;

    GpioCtrlRegs.GPADIR.bit.GPIO16 = 0; // set as input to the uProcessor
    GpioCtrlRegs.GPADIR.bit.GPIO15 = 0;
    GpioCtrlRegs.GPADIR.bit.GPIO14 = 0;

    // LEDs
    GpioCtrlRegs.GPAMUX1.bit.GPIO11 = 0; // set pins as GPIO
    GpioCtrlRegs.GPAMUX1.bit.GPIO10 = 0;
    GpioCtrlRegs.GPAMUX1.bit.GPIO9 = 0;
    GpioCtrlRegs.GPAMUX1.bit.GPIO8 = 0;
    GpioCtrlRegs.GPAMUX1.bit.GPIO7 = 0;
    GpioCtrlRegs.GPAMUX1.bit.GPIO6 = 0;
    GpioCtrlRegs.GPAMUX1.bit.GPIO5 = 0;
    GpioCtrlRegs.GPAMUX1.bit.GPIO4 = 0;
    GpioCtrlRegs.GPAGMUX1.bit.GPIO11 = 0;
    GpioCtrlRegs.GPAGMUX1.bit.GPIO10 = 0;
    GpioCtrlRegs.GPAGMUX1.bit.GPIO9 = 0;
    GpioCtrlRegs.GPAGMUX1.bit.GPIO8 = 0;
    GpioCtrlRegs.GPAGMUX1.bit.GPIO7 = 0;
    GpioCtrlRegs.GPAGMUX1.bit.GPIO6 = 0;
    GpioCtrlRegs.GPAGMUX1.bit.GPIO5 = 0;
    GpioCtrlRegs.GPAGMUX1.bit.GPIO4 = 0;

    GpioDataRegs.GPASET.bit.GPIO11 = 1; // set all LED outputs as 1 to start since LEDs are active low
    GpioDataRegs.GPASET.bit.GPIO10 = 1;
    GpioDataRegs.GPASET.bit.GPIO9 = 1;
    GpioDataRegs.GPASET.bit.GPIO8 = 1;
    GpioDataRegs.GPASET.bit.GPIO7 = 1;
    GpioDataRegs.GPASET.bit.GPIO6 = 1;
    GpioDataRegs.GPASET.bit.GPIO5 = 1;
    GpioDataRegs.GPASET.bit.GPIO4 = 1;

    GpioCtrlRegs.GPADIR.bit.GPIO11 = 1; // set all LED outputs as 1 to start since LEDs are active low
    GpioCtrlRegs.GPADIR.bit.GPIO10 = 1;
    GpioCtrlRegs.GPADIR.bit.GPIO9 = 1;
    GpioCtrlRegs.GPADIR.bit.GPIO8 = 1;
    GpioCtrlRegs.GPADIR.bit.GPIO7 = 1;
    GpioCtrlRegs.GPADIR.bit.GPIO6 = 1;
    GpioCtrlRegs.GPADIR.bit.GPIO5 = 1;
    GpioCtrlRegs.GPADIR.bit.GPIO4 = 1;


}












