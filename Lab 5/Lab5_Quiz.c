/*
 Lab5_Part2.c
 Ryan Simoneau
 Last Modified: Feb, 22 2021

 Description: This program completes a send and recieve test with the codec board

*/

#include <F28x_Project.h>
#include "AIC23.h"
interrupt void Mcbspb_isr(void);
interrupt void Timer1_isr(void);




#define CodecSPI_CLK_PULS {EALLOW; GpioDataRegs.GPASET.bit.GPIO18 = 1; GpioDataRegs.GPACLEAR.bit.GPIO18 = 1;}
#define CodecSPI_CS_LOW {EALLOW; GpioDataRegs.GPACLEAR.bit.GPIO19 = 1;}
#define CodecSPI_CS_HIGH {EALLOW; GpioDataRegs.GPASET.bit.GPIO19 = 1;}

void InitTimer1(void);
void InitAdca(void);
void VoltageLCD(float voltage);

void McBSPbintEnb(void);
void InitAIC23(void);
void InitMcBSPb(void);
void InitSPIA(void);
void SpiTransmit(uint16_t data);


volatile Uint16 LeftoRight = 0;
Uint16 LeftH  = 0;
Uint16 LeftL  = 0;
Uint16 RightH = 0;
Uint16 RightL = 0;

Uint16 test  = 0;
Uint16 test2 = 0;
Uint16 testL = 0;
Uint16 testR = 0;
Uint16 AdcData = 0;
Uint16 key     = 0;
volatile Uint16  dBval = 0;
float  scale = (56.0/2048);

int main(void){

    InitSysCtrl();      // Set SYSCLK to 200 MHz, disables watchdog timer, turns on peripheral clocks

    // enable SPIA and McBSPb along with the AIC23
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

    InitTimer1();       // Initialize CPU timer 1

    InitAdca();         // Initialize ADC A channel 0

    GpioCtrlRegs.GPADIR.bit.GPIO31 = 1;     // Blue LED for timer

    // enable interrupts from McBSPb
    McBSPbintEnb();

    while(1){
        if(key == 1){

            // reset the key
            key = 0;
            test2++;
            // convert digital voltage into dB values for Codec
            dBval = (Uint16)((scale)*((float)AdcData));
            Uint16 command;
            if(dBval >=0x38){
                command = softpowerdown();       // Power down everything except device and clocks
                SpiTransmit(command);
                SmallDelay();
                command = lhp_volctl(0x70 - dBval);       // Left headphone volume control
                SpiTransmit(command);
                SmallDelay();
                command = rhp_volctl(dBval);       // Right headphone volume control
                SpiTransmit(command);
                SmallDelay();
                command = nomicaaudpath();      // Turn on DAC, mute mic
                SpiTransmit(command);
                SmallDelay();

            }else{

                command = softpowerdown();       // Power down everything except device and clocks
                SpiTransmit(command);
                SmallDelay();
                command = lhp_volctl(dBval);       // Left headphone volume control
                SpiTransmit(command);
                SmallDelay();
                command = rhp_volctl(0x70 - dBval);       // Right headphone volume control
                SpiTransmit(command);
                SmallDelay();
                command = nomicaaudpath();      // Turn on DAC, mute mic
                SpiTransmit(command);
                SmallDelay();

            }

        }

    }

}

interrupt void Timer1_isr(void) {

    key = 1;                               // Increment variable. Can view in real time in the "Expressions" window if breakpoint here set to refresh all windows (see breakpoint properties)
    AdcaRegs.ADCSOCFRC1.all = 0x1;          // Force conversion on channel 0
    GpioDataRegs.GPATOGGLE.bit.GPIO31 = 1;  // Toggle blue LED
    AdcData = AdcaResultRegs.ADCRESULT0;    // Read ADC result into global variable
}


interrupt void Mcbspb_isr(void) {

    test++;

    if(0 == LeftoRight){

    LeftoRight = 1;

    testL++;

    // recieve data
    LeftH = McbspbRegs.DRR2.all;
    LeftL = McbspbRegs.DRR1.all;

    // send data to output
    McbspbRegs.DXR2.all = LeftH;
    McbspbRegs.DXR1.all = LeftL;

    }else{


    LeftoRight = 0;

    testR++;

    // recieve data
    RightH = McbspbRegs.DRR2.all;
    RightL = McbspbRegs.DRR1.all;

    McbspbRegs.DXR2.all = RightH;
    McbspbRegs.DXR1.all = RightL;

    }

    PieCtrlRegs.PIEACK.all = PIEACK_GROUP6;

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


void McBSPbintEnb(void){

    PieCtrlRegs.PIEIER6.bit.INTx7 = 1; // enable interrupts to McBSPB (Interrupt 6.7)

    PieVectTable.MCBSPB_RX_INT = &Mcbspb_isr;      // Assign ISR to PIE vector table
    IER |= M_INT6;                             // Enable INT6 in CPU
    EnableInterrupts();                         // Enable PIE and CPU interrupts

}

void InitAIC23(void) {
    SmallDelay();
    uint16_t command;
    command = reset();
    SpiTransmit(command);
    SmallDelay();
    command = softpowerdown();       // Power down everything except device and clocks
    SpiTransmit(command);
    SmallDelay();
    command = linput_volctl(LIV);    // Unmute left line input and maintain default volume
    SpiTransmit(command);
    SmallDelay();
    command = rinput_volctl(RIV);    // Unmute right line input and maintain default volume
    SpiTransmit(command);
    SmallDelay();
    command = lhp_volctl(LHV);       // Left headphone volume control
    SpiTransmit(command);
    SmallDelay();
    command = rhp_volctl(RHV);       // Right headphone volume control
    SpiTransmit(command);
    SmallDelay();
    command = nomicaaudpath();      // Turn on DAC, mute mic
    SpiTransmit(command);
    SmallDelay();
    command = digaudiopath();       // Disable DAC mute, add de-emph
    SpiTransmit(command);
    SmallDelay();

    // I2S
    command = I2Sdigaudinterface(); // AIC23 master mode, I2S mode,32-bit data, LRP=1 to match with XDATADLY=1
    SpiTransmit(command);
    SmallDelay();
    command = CLKsampleratecontrol (SR48);
    SpiTransmit(command);
    SmallDelay();

    command = digact();             // Activate digital interface
    SpiTransmit(command);
    SmallDelay();
    command = nomicpowerup();      // Turn everything on except Mic.
    SpiTransmit(command);
}

void InitMcBSPb(void)
{
    /* Init McBSPb GPIO Pins */

    //modify the GPxMUX, GPxGMUX, GPxQSEL
    //all pins should be set to asynch qualification

    /*
     * MDXB -> GPIO24
     * MDRB -> GPIO25
     * MCLKRB -> GPIO60
     * MCLKXB -> GPIO26
     * MFSRB -> GPIO61
     * MFSXB -> GPIO27
     */
    EALLOW;

    // MDXB -> GPIO24 (GPIOA)

    GpioCtrlRegs.GPAGMUX2.bit.GPIO24 = 0;
    GpioCtrlRegs.GPAMUX2.bit.GPIO24 = 3;
    GpioCtrlRegs.GPAQSEL2.bit.GPIO24 = 3;

    // MDRB -> GPIO25 (GPIOA)

    GpioCtrlRegs.GPAGMUX2.bit.GPIO25 = 0;
    GpioCtrlRegs.GPAMUX2.bit.GPIO25 = 3;
    GpioCtrlRegs.GPAQSEL2.bit.GPIO25 = 3;

    // MFSRB -> GPIO61 (GPIOB)

    GpioCtrlRegs.GPBGMUX2.bit.GPIO61 = 0;
    GpioCtrlRegs.GPBMUX2.bit.GPIO61 = 1;
    GpioCtrlRegs.GPBQSEL2.bit.GPIO61 = 3;

    // MFSXB -> GPIO27 (GPIOA)

    GpioCtrlRegs.GPAGMUX2.bit.GPIO27 = 0;
    GpioCtrlRegs.GPAMUX2.bit.GPIO27 = 3;
    GpioCtrlRegs.GPAQSEL2.bit.GPIO27 = 3;

    // MCLKRB -> GPIO60 (GPIOB)

    GpioCtrlRegs.GPBGMUX2.bit.GPIO60 = 0;
    GpioCtrlRegs.GPBMUX2.bit.GPIO60 = 1;
    GpioCtrlRegs.GPBQSEL2.bit.GPIO60 = 3;

    // MCLKXB -> GPIO26 (GPIOA)

    GpioCtrlRegs.GPAGMUX2.bit.GPIO26 = 0;
    GpioCtrlRegs.GPAMUX2.bit.GPIO26 = 3;
    GpioCtrlRegs.GPAQSEL2.bit.GPIO26 = 3;
    EDIS;

    /* Init McBSPb for I2S mode */
    EALLOW;
    McbspbRegs.SPCR2.all = 0; // Reset FS generator, sample rate generator & transmitter
    McbspbRegs.SPCR1.all = 0; // Reset Receiver, Right justify word
    McbspbRegs.SPCR1.bit.RJUST = 2; // left-justify word in DRR and zero-fill LSBs
    McbspbRegs.MFFINT.all=0x0; // Disable all interrupts
    McbspbRegs.SPCR1.bit.RINTM = 0; // McBSP interrupt flag - RRDY
    McbspbRegs.SPCR2.bit.XINTM = 0; // McBSP interrupt flag - XRDY
    // Clear Receive Control Registers
    McbspbRegs.RCR2.all = 0x0;
    McbspbRegs.RCR1.all = 0x0;
    // Clear Transmit Control Registers
    McbspbRegs.XCR2.all = 0x0;
    McbspbRegs.XCR1.all = 0x0;
    // Set Receive/Transmit to 32-bit operation
    McbspbRegs.RCR2.bit.RWDLEN2 = 5;
    McbspbRegs.RCR1.bit.RWDLEN1 = 5;
    McbspbRegs.XCR2.bit.XWDLEN2 = 5;
    McbspbRegs.XCR1.bit.XWDLEN1 = 5;
    McbspbRegs.RCR2.bit.RPHASE = 1; // Dual-phase frame for receive
    McbspbRegs.RCR1.bit.RFRLEN1 = 0; // Receive frame length = 1 word in phase 1
    McbspbRegs.RCR2.bit.RFRLEN2 = 0; // Receive frame length = 1 word in phase 2
    McbspbRegs.XCR2.bit.XPHASE = 1; // Dual-phase frame for transmit
    McbspbRegs.XCR1.bit.XFRLEN1 = 0; // Transmit frame length = 1 word in phase 1
    McbspbRegs.XCR2.bit.XFRLEN2 = 0; // Transmit frame length = 1 word in phase 2
    // I2S mode: R/XDATDLY = 1 always
    McbspbRegs.RCR2.bit.RDATDLY = 1;
    McbspbRegs.XCR2.bit.XDATDLY = 1;
    // Frame Width = 1 CLKG period, CLKGDV must be 1 as slave
    McbspbRegs.SRGR1.all = 0x0001;
    McbspbRegs.PCR.all=0x0000;
    // Transmit frame synchronization is supplied by an external source via the FSX pin
    McbspbRegs.PCR.bit.FSXM = 0;
    // Receive frame synchronization is supplied by an external source via the FSR pin
    McbspbRegs.PCR.bit.FSRM = 0;
    // Select sample rate generator to be signal on MCLKR pin
    McbspbRegs.PCR.bit.SCLKME = 1;
    McbspbRegs.SRGR2.bit.CLKSM = 0;
    // Receive frame-synchronization pulses are active low - (L-channel first)
    McbspbRegs.PCR.bit.FSRP = 1;
    // Transmit frame-synchronization pulses are active low - (L-channel first)
    McbspbRegs.PCR.bit.FSXP = 1;
    // Receive data is sampled on the rising edge of MCLKR
    McbspbRegs.PCR.bit.CLKRP = 1;



    // Transmit data is sampled on the rising edge of CLKX
    McbspbRegs.PCR.bit.CLKXP = 1;



    // The transmitter gets its clock signal from MCLKX
    McbspbRegs.PCR.bit.CLKXM = 0;
    // The receiver gets its clock signal from MCLKR
    McbspbRegs.PCR.bit.CLKRM = 0;
    // Enable Receive Interrupt
    McbspbRegs.MFFINT.bit.RINT = 1;
    // Ignore unexpected frame sync
    //McbspbRegs.XCR2.bit.XFIG = 1;
    McbspbRegs.SPCR2.all |=0x00C0; // Frame sync & sample rate generators pulled out of reset
    delay_loop();
    McbspbRegs.SPCR2.bit.XRST=1; // Enable Transmitter
    McbspbRegs.SPCR1.bit.RRST=1; // Enable Receiver
    EDIS;
}


void InitSPIA(void)
{
    /* Init GPIO pins for SPIA */

    //enable pullups for each pin
    //set to asynch qualification
    //configure each mux

    //SPISTEA -> GPIO19
    //SPISIMOA -> GPIO58
    //SPICLKA -> GPIO18

    EALLOW;

    //enable pullups
    GpioCtrlRegs.GPAPUD.bit.GPIO19 = 0;
    GpioCtrlRegs.GPAPUD.bit.GPIO18 = 0;
    GpioCtrlRegs.GPBPUD.bit.GPIO58 = 0;

    GpioCtrlRegs.GPAGMUX2.bit.GPIO19 = 0;
    GpioCtrlRegs.GPAGMUX2.bit.GPIO18 = 0;
    GpioCtrlRegs.GPBGMUX2.bit.GPIO58 = 3;

    GpioCtrlRegs.GPAMUX2.bit.GPIO19 = 1;
    GpioCtrlRegs.GPAMUX2.bit.GPIO18 = 1;
    GpioCtrlRegs.GPBMUX2.bit.GPIO58 = 3;

    //asynch qual
    GpioCtrlRegs.GPAQSEL2.bit.GPIO19 = 3;
    GpioCtrlRegs.GPAQSEL2.bit.GPIO18 = 3;
    GpioCtrlRegs.GPBQSEL2.bit.GPIO58 = 3;

    EDIS;

    /* Init SPI peripheral */
    SpiaRegs.SPICCR.all = 0x5F; //CLKPOL = 0, SOMI = SIMO (loopback), 16 bit characters
    SpiaRegs.SPICTL.all = 0x06; //master mode, enable transmissions
    SpiaRegs.SPIBRR.all = 50; //gives baud rate of approx 850 kHz

    SpiaRegs.SPICCR.bit.SPISWRESET = 1;
    SpiaRegs.SPIPRI.bit.FREE = 1;

}
void SpiTransmit(uint16_t data)
{
    /* Transmit 16 bit data */
    SpiaRegs.SPIDAT = data; //send data to SPI register
    while(SpiaRegs.SPISTS.bit.INT_FLAG == 0); //wait until the data has been sent
    Uint16 dummyLoad = SpiaRegs.SPIRXBUF; //reset flag
}
