/*
 Lab5_Part1.c
 Ryan Simoneau
 Last Modified: Feb, 20 2021

 Description: This outputs a voltage reading from the DSP's ADC onto the LCD

*/

#include <F28x_Project.h>
#include "OneToOneI2CDriver.h"
#include "Lab4_Functions.h"
interrupt void Timer1_isr(void);

void InitTimer1(void);
void InitAdca(void);
void VoltageLCD(float voltage);

Uint16 test    = 0;
Uint16 test2   = 0;
Uint16 AdcData = 0;
Uint16 key     = 0;
volatile float  analog = 0;
float  delta = (3.0/4096);

int main(void){

    InitSysCtrl();      // Set SYSCLK to 200 MHz, disables watchdog timer, turns on peripheral clocks

    I2C_O2O_Master_Init(0x27, 200.0, 12);


    LCDINIT();

    DINT;               // Disable CPU interrupts on startup

    // Init PIE
    InitPieCtrl();      // Initialize PIE -> disable PIE and clear all PIE registers
    IER = 0x0000;       // Clear CPU interrupt register
    IFR = 0x0000;       // Clear CPU interrupt flag register
    InitPieVectTable(); // Initialize PIE vector table to known state

    EALLOW;             // EALLOW for rest of program (unless later function disables it)

    InitTimer1();       // Initialize CPU timer 1

    InitAdca();         // Initialize ADC A channel 0

    GpioCtrlRegs.GPADIR.bit.GPIO31 = 1;     // Blue LED for timer


    while(1){
        if(key == 1){

            // reset the key
            key = 0;
            test2++;
            // convert digital voltage into analog representation
            analog = (delta)*((float)AdcData);
            VoltageLCD(analog);

        }

    }

}


interrupt void Timer1_isr(void) {

    key = 1;
    test++;                                 // Increment variable. Can view in real time in the "Expressions" window if breakpoint here set to refresh all windows (see breakpoint properties)
    AdcaRegs.ADCSOCFRC1.all = 0x1;          // Force conversion on channel 0
    GpioDataRegs.GPATOGGLE.bit.GPIO31 = 1;  // Toggle blue LED
    AdcData = AdcaResultRegs.ADCRESULT0;    // Read ADC result into global variable
}









void InitAdca(void) {
    AdcaRegs.ADCCTL2.bit.PRESCALE = 6;                                 // Set ADCCLK to SYSCLK/4
    AdcSetMode(ADC_ADCA, ADC_RESOLUTION_12BIT, ADC_SIGNALMODE_SINGLE); // Initializes ADCA to 12-bit and single-ended mode. Performs internal calibration
    AdcaRegs.ADCCTL1.bit.ADCPWDNZ = 1;                                 // Powers up ADC
    DELAY_US(1000);                                                    // Delay to allow ADC to power up
    AdcaRegs.ADCSOC0CTL.bit.CHSEL = 0;                                 // Sets SOC0 to channel 0 -> pin ADCINA0
    AdcaRegs.ADCSOC0CTL.bit.ACQPS = 14;                                // Sets sample and hold window -> must be at least 1 ADC clock long
}

void InitTimer1(void) {
    InitCpuTimers();                            // Initialize all timers to known state
    ConfigCpuTimer(&CpuTimer1, 200, 1000000);    // Configure CPU timer 1. 200 -> SYSCLK in MHz, 1000000 -> period in usec. NOTE: Does NOT start timer
    PieVectTable.TIMER1_INT = &Timer1_isr;      // Assign timer 1 ISR to PIE vector table
    IER |= M_INT13;                             // Enable INT13 in CPU
    EnableInterrupts();                         // Enable PIE and CPU interrupts
    CpuTimer1.RegsAddr->TCR.bit.TSS = 0;        // Start timer 1
}

void VoltageLCD(float voltage){

    Uint16 Vone  = 0;
    Uint16 Vten  = 0;
    Uint16 Vhund = 0;
    Uint16 Vtho  = 0;

    // isolate the ones, tenths, hundredths, and thousandths place of the voltage
    Vone  = (Uint16)voltage;
    Vten  = (Uint16)(10*(voltage - (float)Vone));
    Vhund = (Uint16)(10*(10*voltage - ((float)Vten + 10*(float)Vone)));
    Vtho  = (Uint16)(10*(100*voltage - ((float)Vhund + 10*(float)Vten + 100*(float)Vone)));

    // convert each to ascii by adding 0x30 to each
    Vone  = Vone  + 0x30;
    Vten  = Vten  + 0x30;
    Vhund = Vhund + 0x30;
    Vtho  = Vtho  + 0x30;

    // store into an array along with " " and "mV"
    //char LCDout[7] = {Vone, Vten, Vhund, Vtho, 0x20, 0x6D, 0x56};
    char LCDout[7] = {Vone, Vten, Vhund, Vtho, ' ', 'm', 'V'};
    // reset the LCD screen output (return Home)
    LCDCTRL(0x02);

    // output to the LCD
    LCDSTRING(LCDout, 7);

}








