/*
 EffectsBoard.c
 Ryan Simoneau
 Last Modified: Mar, 18 2021

 Description: This program implements Tremolo and Fuzz on a real time signal. Effects are controlled via a codec based UI

*/

// --------- Includes --------- //
#include <F28x_Project.h>
#include <stdint.h>
#include <math.h>
#include "fpu.h"
#include <string.h>
#include "Lab4_Functions.h"
#include "AIC23.h"
#include "CommonDrivers.h"
#include "OneToOneI2CDriver.h"
// ---------------------------- //

// ----------- ISRs ----------- //
interrupt void Mcbspb_isr(void);
interrupt void PB0_isr   (void);
interrupt void PB1_isr   (void);
interrupt void PB2_isr   (void);
// ---------------------------- //

// ---- Function Pototypes ---- //
void LCDtremolo(Uint16 RMfreqVal, Uint16 RMdepthVal, Uint16 RMflagVal);
void LCDfuzz   (Uint16 alphaVal, Uint16 MixVal,     Uint16 FuzzflagVal);
// ---------------------------- //

// --------- Defines ---------- //
#define TAPS 52
#define SIZE 1024
#define Fs 48000
// ---------------------------- //

// ------- Data Sections ------ //
//#pragma DATA_SECTION(FilterOut, "ramgs0")
// ---------------------------- //

//################# DSP Variables #################//

// ---------- Codec Variables -------- //
int16 volatile  Ldata = 0;
int16 volatile  Rdata = 0;
// ----------------------------------- //

// ------- FIR Filter variables ------ //
float32 Fircoeff[TAPS] = {-0.00148501112464391,-0.00216511121792752,-0.000723053277505731,0.00403403124923830,0.0102197645705532,0.0132131450078683,0.00913497757476832,-0.000784550283315511,-0.00942587291765276,-0.00889930696506271,
                          0.00178611651547416,0.0137612297983277,0.0146645268767443,0.000437162915029469,-0.0181909499023688,-0.0227687698562058,-0.00421187022817865,0.0249140815923874,0.0367784863842858,0.0121538111473628,-0.0368615982873706,
                          -0.0672301739887619,-0.0337476567784458,0.0731313328581026,0.211018644983489,0.307550301560996,0.307550301560996,0.211018644983489,0.0731313328581026,-0.0337476567784458,-0.0672301739887619,-0.0368615982873706,
                          0.0121538111473628,0.0367784863842858,0.0249140815923874,-0.00421187022817865,-0.0227687698562058,-0.0181909499023688,0.000437162915029469,0.0146645268767443,0.0137612297983277,0.00178611651547416,-0.00889930696506271,
                          -0.00942587291765276,-0.000784550283315511,0.00913497757476832,0.0132131450078683,0.0102197645705532,0.00403403124923830,-0.000723053277505731,-0.00216511121792752,-0.00148501112464391};
float32         SampleIn    = 0;  // sample from the codec
float32         FIRBuff   [TAPS]; // signal input to the FIR filter
float32         FilterOut   = 0;  // output of the FIR filter
Uint16          FIRindex    = 0;  // index of the circular buffer
Uint16          FIRcurr     = 0;  // position of the current input in the buffer
Uint16          FIRsumIndex = 0;
// ----------------------------------- //

// -------- Tremelo Variables -------- //
Uint16  RMfreq  = 5;   // default value
float32 RMdepth = 0.5f; // default value
Uint16  RMcnt   = 0;   // a counter to indicate position in the sine wave
Uint16  RMflag  = 0;   // default state of effects is OFF
// ----------------------------------- //

// ---------- Fuzz Variables --------- //
float32 alpha    = 0.5f; // default value
float32 Mix      = 0.5f; // default value
Uint16  Fuzzflag = 0;   // default state of effects is OFF
// ----------------------------------- //

//#################################################//


//################# UI Variables ##################//

// ---- Push Buttons and Switches ---- //
Uint16 PB2press = 0; // Selects effects and effect qualities
Uint16 PB1press = 0; // Goes down list of effects | decreases value of effects
Uint16 PB0press = 0; // Goes up list of effects | increases value of effects
// ----------------------------------- //

// ---------- LCD Indicators --------- //
Uint16 LCDframe   = 0; // Indicates which effect LCD will display. Tremolo is the initial frame
Uint16 Settingptr = 0; // Indicates which setting cursor will hover over and can be selected
Uint16 NumEffects = 2; // Number of effects in the program
// ----------------------------------- //

// ------------- Tremolo ------------- //
float32 FMul = 5;   // dictates how much the frequency of the modulator changes upon every increment
float32 Finc = 1;   // amount frequency should be incremented/decremented by. Controlled by PB1 and PB0 being pressed
float32 DMul = 0.1; // dictates how much the depth of the modulator changes upon every increment
float32 Dinc = 1;   // amount depth should be incremented/decremented by. Controlled by PB1 and PB0 being pressed
// ----------------------------------- //

// --------------- Fuzz -------------- //
float32 aMul = 0.1; // amount alpha will change upon increment/decrement
float32 ainc = 1;   // how much alpha should be incremented by
float32 MMul = 0.1; // dictates how much the mixing of the fuzz changes upon every increment
float32 Minc = 1;   // amount mix should be incremented/decremented by. Controlled by PB1 and PB0 being pressed
// ----------------------------------- //

//#################################################//

int main(void)
{

    InitSysCtrl(); // Set SYSCLK to 200 MHz, disables watchdog timer, turns on peripheral clocks

    Uint16 intRMdepth = (Uint16)(10*RMdepth); // copies of float variables so they canbe displayed on LCD correctly
    Uint16 intalpha   = (Uint16)(10*alpha);
    Uint16 intMix     = (Uint16)(100*Mix);

    EALLOW;
    I2C_O2O_Master_Init(0x27, 200.0, 12);
    LCDINIT();
    LCDCTRL(0x0C); // disable cursor blinking, shut off the cursor

    for(Uint16 j=0;j<TAPS;j++){ // clear the input buffer of the FIR filter
        FIRBuff[j]=0;
    }

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

    // enable interrupts from McBSPb and push buttons
    McBSPbintEnb();
    PBintEnb();

    PieVectTable.MCBSPB_RX_INT = &Mcbspb_isr; // Assign ISR to PIE vector table
    PieVectTable.XINT1_INT     = &PB0_isr;
    PieVectTable.XINT2_INT     = &PB1_isr;
    PieVectTable.XINT5_INT     = &PB2_isr;

    IER |= M_INT1;  // Enable INT1  in  CPU  or PBs
    IER |= M_INT6;  // Enable INT6  in  CPU for McBSPb
    IER |= M_INT12; // Enable INT12 in  CPU for PBs

    EnableInterrupts();                        // Enable PIE and CPU interrupts

    while(1){

        // %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%% User Interface %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%% //

        // UI is organized in 3 levels, and in each level the user can:
        // Frame   Level: Transition between Effect Frames, Turns effects ON/OFF, enter Effect Level (indicated by still cursor on leftmost setting)
        // Effect  Level: Possition cursor on a setting, Enter Setting Level (indicated by flashing cursor)
        // Setting Level: Modify a setting's value, Exit Setting Level (indicated by still cursor)

        // Frame Level: ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ //

        // Frame Transitioning: Transition between frames (no selections made)##################
        if(PB1press){
            // if LCDframe is at the bottom, roll over to the top
            if(LCDframe == 0){
                LCDframe = NumEffects;
            }else{
                LCDframe--; // otherwise, decrement LCDframe
            }
            PB1press = 0; // set the press flag back to zero
        }
        if(PB0press){
            // if LCDframe is at the top, roll over to the bottom
            if(LCDframe == (NumEffects-1)){
                LCDframe = 0;
            }else{
                LCDframe++; // otherwise, increment LCDframe
            }
            PB0press = 0; // set the press flag back to zero
        }
        // End of Frame Transitioning ##########################################################

        // Effect Toggle: Turns an effect ON/OFF depending on what frame the LCD is on #########
        if(PB2press){
            if(LCDframe == 0){
                RMflag ^= 1 << 0; // ON/OFF tremolo
            }
            if(LCDframe == 1){
                Fuzzflag ^= 1 << 0; // ON/OFF Fuzz
            }
            PB2press = 0; // set the press flag back to zero
        }
        // End of Effect Toggle ################################################################

        //Effect Level: ********************************************************************* //
        if(GpioDataRegs.GPADAT.bit.GPIO3 == 1){ // SW3
            LCDCTRL(0x0E); // turn on the cursor, no blink
            LCDCTRL(0xC0);
            while(GpioDataRegs.GPADAT.bit.GPIO3 == 1){

                if(PB1press){
                    // if Settingptr is at the bottom, roll over to the top
                    if(Settingptr == 0){
                        Settingptr = 1;
                        LCDCTRL(0xC9); // move the cursor to 9th possition of 2nd line
                    }else{
                        Settingptr--;    // otherwise, decrement Settingptr
                        LCDCTRL(0xC0); // move the cursor to start of 2nd line
                    }
                    PB1press = 0; // set the press flag back to zero
                }
                if(PB0press){
                    // if Settingptr is at the bottom, roll over to the top
                    if(Settingptr == 1){
                        Settingptr = 0;
                        LCDCTRL(0xC0); // move the cursor to 0th possition of 2nd line
                    }else{
                        Settingptr++;  // otherwise, increment Settingptr
                        LCDCTRL(0xC9); // move the cursor to start of 2nd line
                    }
                    PB0press = 0; // set the press flag back to zero
                }

                //Setting Level: ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^ //

                if(PB2press){

                    LCDCTRL(0x0D); // turn on the cursor, blink
                    PB2press = 0;
                    while(PB2press == 0){

                        if(LCDframe == 0){ // tremolo

                            if(Settingptr == 0){ // RMfreq
                                float32 RMFconst = RMfreq;
                                if(PB1press){ // decrement value
                                    PB1press = 0; // set the press flag back to zero
                                    if(RMfreq == 5.0f){ // already reached bottom
                                        RMfreq = 50.0f;
                                    }else{
                                        RMfreq = RMfreq - FMul*Finc;
                                    }
                                }
                                if(PB0press){ // increment value
                                    PB0press = 0;
                                    if(RMfreq == 50.0f){ // already reached top
                                        RMfreq = 5.0f;
                                    }else{
                                        RMfreq = RMfreq + FMul*Finc;
                                    }
                                }
                                if(RMfreq != RMFconst){
                                LCDtremolo( RMfreq, intRMdepth, RMflag);
                                LCDCTRL   (0x0D);
                                LCDCTRL   (0xC0);
                                }
                            }

                            if(Settingptr == 1){ // depth
                                float32 RMDconst = RMdepth;
                                if(PB1press){ // decrement value
                                    PB1press = 0; // set the press flag back to zero
                                    if(intRMdepth == 0){ // already reached bottom
                                        RMdepth  = 1.0f;
                                        intRMdepth = 10;
                                    }else{
                                        RMdepth  = RMdepth - DMul*Dinc;
                                        intRMdepth--;
                                    }
                                }
                                if(PB0press){ // increment value
                                    PB0press = 0;
                                    if(intRMdepth == 10){ // already reached top
                                        RMdepth  = 0.0f;
                                        intRMdepth = 0;
                                    }else{
                                        RMdepth = RMdepth + DMul*Dinc;
                                        intRMdepth++;
                                    }
                                }
                                if(RMdepth != RMDconst){
                                LCDtremolo( RMfreq, intRMdepth, RMflag);
                                LCDCTRL   (0x0D);
                                LCDCTRL   (0xC9);
                                }
                            }
                        }

                        if(LCDframe == 1){ // Fuzz

                            if(Settingptr == 0){ // alpha
                                float32 alphaconst = alpha;
                                if(PB1press){ // decrement value
                                    PB1press = 0; // set the press flag back to zero
                                    if(intalpha == 0){ // already reached bottom
                                        alpha = 1.0f;
                                        intalpha = 10;
                                    }else{
                                        alpha = alpha - aMul*ainc;
                                        intalpha--;
                                    }
                                }
                                if(PB0press){ // increment value
                                    PB0press = 0;
                                    if(intalpha == 10){ // already reached top
                                        alpha = 0.0f;
                                        intalpha = 0;
                                    }else{
                                        alpha = alpha + aMul*ainc;
                                        intalpha++;
                                    }
                                }
                                if(alpha != alphaconst){
                                LCDfuzz( intalpha, intMix, Fuzzflag);
                                LCDCTRL   (0x0D);
                                LCDCTRL   (0xC0);
                                }
                            }

                            if(Settingptr == 1){ // RMfreq
                                float32 Mixconst = Mix;
                                if(PB1press){ // decrement value
                                    PB1press = 0; // set the press flag back to zero
                                    if(intMix == 0){ // already reached bottom
                                        Mix = 1.0f;
                                        intMix = 100;
                                    }else{
                                        Mix = Mix - MMul*Minc;
                                        intMix -= 10;
                                    }
                                }
                                if(PB0press){ // increment value
                                    PB0press = 0;
                                    if(intMix == 100){ // already reached top
                                        Mix = 0.0f;
                                        intMix = 0;
                                    }else{
                                        Mix = Mix + MMul*Minc;
                                        intMix += 10;
                                    }
                                }
                                if(Mix != Mixconst){
                                LCDfuzz( intalpha, intMix, Fuzzflag);
                                LCDCTRL   (0x0D);
                                LCDCTRL   (0xC9);
                                }
                            }
                        }



                    }
                    PB2press = 0;
                    LCDCTRL(0x0E); // turn on the cursor, no blink

                }

                //^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^ //

            }
        LCDCTRL(0x0C); // turn off the cursor
        }

        // ********************************************************************************** //

        // Tremolo Frame
        if(LCDframe == 0){

            LCDtremolo( RMfreq, intRMdepth, RMflag);
        }

        // Fuzz Frame
        if(LCDframe == 1){

            LCDfuzz( intalpha, intMix, Fuzzflag);

        }
        // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ //
        // %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%% //
    }


}


interrupt void Mcbspb_isr(void) {

   // recieve data
   Ldata = McbspbRegs.DRR2.all;
   Rdata = McbspbRegs.DRR1.all;
   SampleIn = (float32)Ldata;

   // FIR Routine
   // store new sample into the FIRbuffer and increment the index
   FIRBuff[FIRindex] = SampleIn;
   FIRindex++;
   // check to see if the index is outside the bounds of the buffer
   if(FIRindex == TAPS){
       FIRindex = 0;
   }
   FilterOut = 0.0f;
   FIRsumIndex = FIRindex;
   for(Uint16 j=0;j<TAPS;j++){

       if(FIRsumIndex > 0){
           FIRsumIndex--;
       }else{
           FIRsumIndex = TAPS - 1;
       }

       FilterOut += Fircoeff[j]*FIRBuff[FIRsumIndex];
   }
   if(RMflag){
       float32 Modulator = (1-RMdepth) + RMdepth*sinf(2*M_PI*RMfreq*RMcnt/(Fs)); // sine for ring modulation
       FilterOut = FilterOut*Modulator;
       RMcnt++;

       if(RMcnt == Fs){
           RMcnt = 0;
       }
   }
   // Implement Fuzz
   if(Fuzzflag){
       float32 Fiabs = fabsf(FilterOut);
       float32 Fuzz  = (FilterOut/Fiabs)*(1 - expf((alpha*pow(FilterOut,2))/(Fiabs)));
       FilterOut     = ((float32)(int16)(Mix*Fuzz) + (1 - Mix)*FilterOut);
   }

   McbspbRegs.DXR2.all = (int16)FilterOut;  // Test the FIR real quick
   McbspbRegs.DXR1.all = (int16)FilterOut; // Test the FIR real quick

    PieCtrlRegs.PIEACK.all = PIEACK_GROUP6;
}

interrupt void PB0_isr(void){ // Button 0 (increment)
    PB0press = 1;

    PieCtrlRegs.PIEACK.all = PIEACK_GROUP1;

}

interrupt void PB1_isr(void){ // Button 1 (Decrement)
    PB1press = 1;

    PieCtrlRegs.PIEACK.all = PIEACK_GROUP1;
}

interrupt void PB2_isr(void){ // Button 2 (Selector)
    PB2press = 1;

    PieCtrlRegs.PIEACK.all = PIEACK_GROUP12;
}

void LCDtremolo(Uint16 RMfreqVal, Uint16 RMdepthVal, Uint16 RMflagVal){

    // convert setting values to ascii
    Uint16 Ften   = 0; // setting: Frequency
    Uint16 Fone   = 0; // setting: Frequency
    Uint16 Done   = 0; // setting: Depth
    Uint16 Dten = 0; // setting: Depth

    //Temporary conversion variables
    float32 RMFf = (float32)(RMfreqVal);
    float32 RMFd = (float32)(RMdepthVal);

    Ften = (Uint16)(0.1f*RMFf);
    Fone = (Uint16)(RMFf - 10*((float32)(Ften)));

    Dten = (Uint16)(0.1*RMFd);
    Done = (Uint16)(RMFd - 10*((float32)(Dten)));

    // add 0x30 to each and output to the LCD
    Ften = Ften + 0x30;
    Fone = Fone + 0x30;
    Done = Done + 0x30;
    Dten = Dten + 0x30;
    char RMLCDtopON [16] = {'T','r','e','m','o','l','o',' ','S','t','a','t',':','O','N',' '};
    char RMLCDtopOFF[16] = {'T','r','e','m','o','l','o',' ','S','t','a','t',':','O','F','F'};
    char RMLCDbot   [16] = {' ','M','F','r','e','q',':',Ften,Fone,' ','D',':',Dten,'.',Done,' '};
    // reset the LCD screen output (return Home)
    LCDCTRL(0x02);
    if(RMflagVal){
        LCDSTRING(RMLCDtopON,16);
    }else{
        LCDSTRING(RMLCDtopOFF,16);
    }
    // go to the next line
    LCDCTRL(0xC0);

    LCDSTRING(RMLCDbot,16);

}
void LCDfuzz(Uint16 alphaVal, Uint16 MixVal, Uint16 FuzzflagVal){

    // convert setting values to ascii
    Uint16 Aone  = 0; // setting: Alpha
    Uint16 Aten  = 0; // setting: Alpha
    Uint16 Mone  = 0; // setting: Mix
    Uint16 Mten  = 0; // setting: Mix
    Uint16 Mhund = 0;

    float32 aF = (float32)(alphaVal);
    float32 MF = (float32)(MixVal);

    Aten  = (Uint16)(0.1*aF);
    Aone  = (Uint16)(aF - 10*((float32)(Aten)));

    Mhund = (Uint16)(0.01*MF);
    Mten  = (Uint16)(0.1*MF - 10*(float32)Mhund);
    Mone  = (Uint16)(MF - (100*(float32)(Mhund) + 10*(float32)(Mten)));


    // add 0x30 to each and output to the LCD
    Aone  = Aone  + 0x30;
    Aten  = Aten  + 0x30;
    Mone  = Mone  + 0x30;
    Mten  = Mten  + 0x30;
    Mhund = Mhund + 0x30;

    char FLCDtopON [16] = {'F','u','z','z',' ',' ',' ',' ','S','t','a','t',':','O','N',' '};
    char FLCDtopOFF[16] = {'F','u','z','z',' ',' ',' ',' ','S','t','a','t',':','O','F','F'};
    char FLCDbot   [16] = {' ',0xE0,':',Aten,'.',Aone,' ',' ',' ',' ','M',':',Mhund,Mten,Mone,'%'}; // 0xE0 is alphaVal 0x25 is %
    // reset the LCD screen output (return Home)
    LCDCTRL(0x02);
    if(FuzzflagVal){
        LCDSTRING(FLCDtopON,16);
    }else{
        LCDSTRING(FLCDtopOFF,16);
    }
    // go to the next line
    LCDCTRL(0xC0);

    LCDSTRING(FLCDbot,16);

}




