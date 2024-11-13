/*
 Lab6_Part1_2.c
 Ryan Simoneau
 Last Modified: Mar, 07 2021

 Description: This program records to the SRAMs and allows for playback via the CODEC

*/

#include <F28x_Project.h>
#include "Lab4_Functions.h"
#include "AIC23.h"
interrupt void Mcbspb_isr(void);

// external interrupts for the push buttons
interrupt void PB1_isr(void);
interrupt void PB2_isr(void);
interrupt void PB3_isr(void);


#define CodecSPI_CLK_PULS {EALLOW; GpioDataRegs.GPASET.bit.GPIO18 = 1; GpioDataRegs.GPACLEAR.bit.GPIO18 = 1;}
#define CodecSPI_CS_LOW {EALLOW; GpioDataRegs.GPACLEAR.bit.GPIO19 = 1;}
#define CodecSPI_CS_HIGH {EALLOW; GpioDataRegs.GPASET.bit.GPIO19 = 1;}


void PBintEnb(void);

void McBSPbintEnb(void);
void InitAIC23(void);
void InitMcBSPb(void);
void InitSPIA(void);
void SpiTransmit(uint16_t data);

void CODECGPIOINIT(void);


// GPIO variables
Uint16 DIP = 0; // value of the DIP switches
Uint16 volatile PB1test = 0;
Uint16 volatile PB2test = 0;
Uint16 volatile PB3test = 0;

Uint16 NoMix = ~1;
Uint16 Mix = ~9;
Uint16 Three = ~3;
Uint16 Zero = ~(0xFF);
Uint16 Off = 0xFF;

// McBSP variables
Uint16 LeftoRight = 0;
int16 Record  = 0;
Uint16 Play = 0;
Uint16 Dcare  = 0;
Uint16 zero = 0;
Uint16 testL = 0;
Uint16 testR = 0;


// SRAM variables
Uint32 RecordPointer = 0;
Uint32 PlayPointer = 0;
Uint32 ZeroPointer = 0;

Uint16 volatile SRAMdata = 0;

Uint16 volatile dummy;

Uint16 volatile RecordKey = 0;
Uint16 volatile PlayKey = 0;
Uint16 volatile ZeroKey = 0;
Uint16 volatile DataSend = 0;

int16 Mixer = 0;

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

    // enable interrupts from McBSPb
    McBSPbintEnb();
    // enable interrups for the push buttons
    PBintEnb();

    EnableInterrupts(); // Enable PIE and CPU interrupts

    // ---------------- //
    //uint16_t command;
    //command = CLKsampleratecontrol (SR32);
    //SpiTransmit(command);
    //SmallDelay();
    // ---------------- //







    while(1){ // top level while loop to cycle between which mode we're in

        // RECORD FUNCTION //
        while(RecordKey == 1){
            DIP = (~(GpioDataRegs.GPADAT.all) & 0xF);
            if(DIP == 0){
                GpioDataRegs.GPACLEAR.bit.GPIO4 = 1;

                while(DataSend == 0);
                SRAMWRITE(Record,RecordPointer);
                RecordPointer = RecordPointer + 2;
                DataSend = 0;

                if((PlayKey == 1) || (RecordKey == 0)){ // if play is pressed or the recorder is stopped break from the while loop

                    RecordKey = 0; // stop the recording
                    GpioDataRegs.GPASET.all |= (0xFF << 4); // shut the LEDs off once we have filled the entire memory
                    break; // break out of the loop if the play button(3) or record button(1) has been pressed

                }

                if(RecordPointer >= 0x80000){ // this loop will stop the pointer from being set to zero if the contents of the SRAM were played in the middle of a recording

                RecordPointer = 0; // set record pointer back to zero
                RecordKey = 0; // set equal to zero so that we don't write over what has already been written to the SRAM
                GpioDataRegs.GPASET.all |= (0xFF << 4); // shut the LEDs off once we have filled the entire memory

                }

            }
            if(DIP == 1){

                GpioDataRegs.GPACLEAR.bit.GPIO4 = 1;
                GpioDataRegs.GPACLEAR.bit.GPIO11 = 1;

                while(DataSend == 0);
                Mixer = SRAMREAD(RecordPointer);
                Record = (int16)((((float)Mixer)*0.5) + (((float)Record)*0.5)); // mix the audio in floating point
                SRAMWRITE(Record,RecordPointer);
                RecordPointer = RecordPointer + 2;
                DataSend = 0;

                if((PlayKey == 1) || (RecordKey == 0)){ // if play is pressed or the recorder is stopped break from the while loop

                    RecordKey = 0; // stop the recording
                    GpioDataRegs.GPASET.all |= (0xFF << 4); // shut the LEDs off once we have filled the entire memory
                    break; // break out of the loop if the play button(3) or record button(1) has been pressed

                }

                if(RecordPointer >= 0x80000){ // this loop will stop the pointer from being set to zero if the contents of the SRAM were played in the middle of a recording

                RecordPointer = 0; // set record pointer back to zero
                RecordKey = 0; // set equal to zero so that we don't write over what has already been written to the SRAM
                GpioDataRegs.GPASET.all |= (0xFF << 4); // shut the LEDs off once we have filled the entire memory

                }

            }

        }


        // PLAY FUNCTION //
        while(PlayKey == 1){

            GpioDataRegs.GPACLEAR.bit.GPIO4 = 1;
            GpioDataRegs.GPACLEAR.bit.GPIO5 = 1;

            while(DataSend == 0);
            Play = SRAMREAD(PlayPointer);
            PlayPointer = PlayPointer + 2;
            DataSend = 0;

            if(PlayPointer >= 0x80000){ // this loop will stop the pointer from being set to zero if the contents of the SRAM were played in the middle of a recording

                PlayPointer = 0; // set record pointer back to zero
                PlayKey = 0; // set equal to zero so that we don't write over what has already been written to the SRAM
                GpioDataRegs.GPASET.all |= (0xFF << 4); // shut the LEDs off once we have filled the entire memory

            }

        }

        while(ZeroKey == 1){

            GpioDataRegs.GPACLEAR.all |= (0xFF << 4);


            SRAMWRITE(0, ZeroPointer);
            ZeroPointer = ZeroPointer + 2;

            if(ZeroPointer >= 0x80000){ // this loop will stop the pointer from being set to zero if the contents of the SRAM were played in the middle of a recording

                ZeroPointer = 0; // set record pointer back to zero
                RecordPointer = 0; // set the record pointer back t zero
                ZeroKey = 0; // set equal to zero so that we don't write over what has already been written to the SRAM
                GpioDataRegs.GPASET.all |= (0xFF << 4); // shut the LEDs off once we have filled the entire memory

            }


        }

    }

}






interrupt void PB1_isr(void){ // Button 1 (Record)

    PB1test++;

    XintRegs.XINT1CR.bit.ENABLE = 0; // disable interrupts to avoid bouncing
    DELAY_US(5E2);
    XintRegs.XINT1CR.bit.ENABLE = 1; // re-enable after bouncing is gone
    if(RecordKey == 0){ // If we are going to record, set recordkey equal to one

        RecordKey = 1;

    }else{

        RecordKey = 0; // if button 1 has been pressed to stop the recording, set record flag low

    }

    PieCtrlRegs.PIEACK.all = PIEACK_GROUP1;

}

interrupt void PB2_isr(void){ // Button 2 (Zero)

    PB2test++;

    XintRegs.XINT2CR.bit.ENABLE = 0; // disable interrupts to avoid bouncing
    DELAY_US(5E2);
    XintRegs.XINT2CR.bit.ENABLE = 1; // re-enable after bouncing is gone

    ZeroKey = 1;

    PieCtrlRegs.PIEACK.all = PIEACK_GROUP1;

}

interrupt void PB3_isr(void){ // Button 3 (Play)

    PB3test++;

    XintRegs.XINT3CR.bit.ENABLE = 0; // disable interrupts to avoid bouncing
    DELAY_US(5E2);
    XintRegs.XINT3CR.bit.ENABLE = 1; // re-enable after bouncing is gone

    PlayKey = 1;

    PieCtrlRegs.PIEACK.all = PIEACK_GROUP12;

}


interrupt void Mcbspb_isr(void) {

        if(0 == LeftoRight){

        LeftoRight = 1;

        testL++;
            if((RecordKey == 0) && (PlayKey == 0)){

                // recieve data
                Dcare = McbspbRegs.DRR2.all;
                Dcare = McbspbRegs.DRR1.all;

            }
            if(RecordKey == 1){

                // recieve data
                Record = McbspbRegs.DRR2.all;
                Dcare = McbspbRegs.DRR1.all;


                // set DataSend to allow new data to be saved to the SRAMS
                DataSend = 1;

            }

            if(PlayKey == 1){

                // recieve data
                Dcare = McbspbRegs.DRR2.all;
                Dcare = McbspbRegs.DRR1.all;

                // send data
                McbspbRegs.DXR2.all = Play;
                McbspbRegs.DXR1.all = zero;

                DataSend = 1; // allow new data to be read from SRAMs

            }

        }else{


            LeftoRight = 0;

            testR++;

            // recieve data
            Dcare = McbspbRegs.DRR2.all;
            Dcare = McbspbRegs.DRR1.all;

            }

            PieCtrlRegs.PIEACK.all = PIEACK_GROUP6;



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

}

void McBSPbintEnb(void){

    PieCtrlRegs.PIEIER6.bit.INTx7 = 1; // enable interrupts to McBSPB (Interrupt 6.7)

    PieVectTable.MCBSPB_RX_INT = &Mcbspb_isr;      // Assign ISR to PIE vector table
    IER |= M_INT6;                             // Enable INT6 in CPU

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
