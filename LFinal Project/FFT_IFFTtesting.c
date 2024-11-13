/*
 Lab8_Part3.c
 Ryan Simoneau
 Last Modified: Mar, 16 2021

 Description: This program implements a 512 point FFT utilizing pingpong registers and DMA

*/

// Standard Includes
#include <F28x_Project.h>
#include <stdint.h>
#include <math.h>
#include <complex.h>
#include <string.h>
#include "Lab4_Functions.h"
#include "AIC23.h"
#include "CommonDrivers.h"
#include "OneToOneI2CDriver.h"

// FFT/IFFT Includes
#include "fpu.h"
#include "dsp.h"
#include "fpu32/fpu_cfft.h"

// -- Defines -- //
#define FFT_STAGES 9
#define FFT_SIZE   (1 << FFT_STAGES)
#define BURST 1 //BURST is number of burst - 1 in an xfer
#define TRANSFER 511 //TRANSFER is number of bursts - 1 in an xfer

#define Fs  48000     //sampling frequency
#define Fw  1000      //sine frequency
#define L  10         //number of periods
#define SPP  Fs/Fw    //samples per period


// ------------- //


// -- Variables and Arrays -- //

// FFT Variables
int16           Input     [FFT_SIZE];
float32         FFTin     [FFT_SIZE];
float32         FFTout    [FFT_SIZE + 2U];
volatile int16  Output    [FFT_SIZE];
float32         FFTphase  [FFT_SIZE];
float32         FFTmag    [FFT_SIZE];
float32         FFTF32Coef[FFT_SIZE];
float32         Hann       = 0;
Uint16          FFTflag    = 0;
Uint16          Counter    = 0;

volatile Uint16 MagTest   [FFT_SIZE/2 + 1];

float32 SCALE = 0.001953125;




// -------------------------- //
// FFT Data sections
#pragma DATA_SECTION(Input,      "ramgs0")
#pragma DATA_SECTION(FFTin,      "ramgs1")
#pragma DATA_SECTION(FFTout,     "ramgs2")
#pragma DATA_SECTION(FFTF32Coef, "ramgs3")
#pragma DATA_SECTION(FFTphase,   "ramgs4")
#pragma DATA_SECTION(FFTmag,     "ramgs5")
#pragma DATA_SECTION(Output,     "ramgs6")

// FFT Struct
CFFT_F32_STRUCT cfft;
CFFT_F32_STRUCT_Handle hnd_cfft = &cfft;

interrupt void local_D_INTCH6_ISR(void);
interrupt void local_D_INTCH5_ISR(void);

int main(void){

    float32 *FFTpoint;

    InitSysCtrl();      // Set SYSCLK to 200 MHz, disables watchdog timer, turns on peripheral clocks

    EALLOW;
    DINT;               // Disable CPU interrupts on startup
    // Init PIE
    InitPieCtrl();      // Initialize PIE -> disable PIE and clear all PIE registers
    IER = 0x0000;       // Clear CPU interrupt register
    IFR = 0x0000;       // Clear CPU interrupt flag register
    InitPieVectTable(); // Initialize PIE vector table to known state

    // clear buffers
    for(Uint16 i = 0; i < FFT_SIZE; i++){
        Input      [i] = 0;
        FFTin      [i] = 0;
        FFTout     [i] = 0;
    }

    // FFT
    // Configure the object
    CFFT_f32_setInputPtr(hnd_cfft, FFTin);
    CFFT_f32_setOutputPtr(hnd_cfft, FFTout);
    CFFT_f32_setTwiddlesPtr(hnd_cfft, CFFT_f32_twiddleFactors);
    CFFT_f32_setStages(hnd_cfft, (FFT_STAGES - 1U));
    CFFT_f32_setFFTSize(hnd_cfft, (FFT_SIZE >> 1));


    EALLOW;
    // enable SPIA and McBSPb along with the AIC23 and GPIO for DIP, PBs, and LEDs
    InitSPIA();
    InitAIC23();
    InitMcBSPb();

    EALLOW;
    // Set DMA CH transfer complete interrupts to ISR
    PieVectTable.DMA_CH6_INT= &local_D_INTCH6_ISR;
    PieVectTable.DMA_CH5_INT= &local_D_INTCH5_ISR;

    // Performs a hard reset on the DMA
    DMAInitialize();

    // source and destination pointers
    volatile Uint16 *DMA_CH6_Source = (volatile Uint16 *)&McbspbRegs.DRR2.all;
    volatile Uint16 *DMA_CH6_Dest = (volatile Uint16 *)&Input[0];

    volatile Uint16 *DMA_CH5_Source = (volatile Uint16 *)&Output[0];
    volatile Uint16 *DMA_CH5_Dest = (volatile Uint16 *)&McbspbRegs.DXR2.all;

    // Initialize Channel source and destination addresses
    DMACH6AddrConfig(DMA_CH6_Dest,DMA_CH6_Source);
    DMACH5AddrConfig(DMA_CH5_Dest,DMA_CH5_Source);

    // Configures the burst size and source/destination step size
    DMACH6BurstConfig(BURST,1,1); // does not increment between words //
    DMACH5BurstConfig(BURST,0,1);

    // Configures the transfer size and source/destination step size
    // Transfer size is 1 burst (0 + 1)
    DMACH6TransferConfig(TRANSFER,-1,0); // increments between bursts //
    DMACH5TransferConfig(TRANSFER,1,-1);

    // Configures source and destination wrapping
    DMACH6WrapConfig(0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF);
    DMACH5WrapConfig(0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF);

    // CH6 mode configuration:
    DMACH6ModeConfig(74,PERINT_ENABLE,ONESHOT_DISABLE,CONT_ENABLE,
                     SYNC_DISABLE,SYNC_SRC,OVRFLOW_DISABLE,SIXTEEN_BIT,
                     CHINT_END,CHINT_ENABLE);
    DMACH5ModeConfig(74,PERINT_ENABLE,ONESHOT_DISABLE,CONT_ENABLE,
                     SYNC_DISABLE,SYNC_SRC,OVRFLOW_DISABLE,SIXTEEN_BIT,
                     CHINT_END,CHINT_ENABLE);

    // Dual ported bridge connected to DMA
    EALLOW;
    CpuSysRegs.SECMSEL.bit.PF2SEL = 1;
    EDIS;

    // Interrupt enabling
    // PIE group 7, interrupt 6 -> DMA CH6
    PieCtrlRegs.PIEIER7.bit.INTx6 = 1;
    // PIE group 7, interrupt 5 -> DMA CH5
    PieCtrlRegs.PIEIER7.bit.INTx5 = 1;
    IER |= M_INT7;

    CODECGPIOINIT();

    // Enables the PIE
    // Clears PIEACK register
    // Enables global interrupts
    EnableInterrupts();

    // Start DMA CH5 and 6
    StartDMACH6();
    StartDMACH5();

    while(1)
    {

       if(FFTflag)
        {


            // FFT Computations
           CFFT_f32t(hnd_cfft);                   // Calculate FFT
           CFFT_f32_unpack(hnd_cfft);

           FFTpoint      = CFFT_f32_getCurrOutputPtr(hnd_cfft);

           // swap CurrentInPtr and CurrentOutPtr pointers
           FFTpoint = CFFT_f32_getCurrInputPtr(hnd_cfft);
           CFFT_f32_setCurrInputPtr(hnd_cfft, CFFT_f32_getCurrOutputPtr(hnd_cfft));
           CFFT_f32_setCurrOutputPtr(hnd_cfft, FFTpoint);

           CFFT_f32_mag_TMU0(hnd_cfft); // at this point, the magnitudes are in the output buffer (FFTout)


           // NOTE: the N/2 point is not calculated by the mag functions;
           // we dont need to calculate |F(N/2)| as its real, so we just copy
           // it over
           //
           FFTin[FFT_SIZE/2] = FFTout[FFT_SIZE]; // I think this means that the magnitude is in the inbuffer at this point

           for(Uint16 j = 0; j< (FFT_SIZE); j++) // Grab the magnitude and make small modification
               {
                   FFTmag[j] = 0.0f*FFTout[j];
               }

           // NOTE: Final output of the FFT magnitude is stored in the
           // buffer pointed to by obj.p_currOutput
           FFTpoint     = CFFT_f32_getCurrOutputPtr(hnd_cfft);

           // To avoid overwriting the magnitude, change the output buffer for
           // the phase()
           CFFT_f32_setCurrOutputPtr(hnd_cfft, FFTphase);

           // Calculate phase, result stored in CurrentOutPtr
           // Also, by this point, the magnitude plot is still being stored in FFTout
           CFFT_f32_phase_TMU0(hnd_cfft); // OutPtr is at FFTout and CurrentOutPtr is at FFTphase
           // Turns out that the phase is actually in the FFTin buffer at this stage
           // I'm not exactly sure if it's the phase or the imaginary component of the fft. My intuition says the former.
           // The graph resembles the imaginary component from matlab a lot more than the phase however. Especially with a square wave (actually kinda looks like non-imag)

//           for(Uint16 j = 0; j< (FFT_SIZE); j++) // Grab the Phase and make small modification
//               {
//                   FFTphase[j] = 1.0f*FFTphase[j]; // 0.4f*
//               }


           // NOTE: Final output of the FFT magnitude is stored in the
           // buffer pointed to by obj.p_currOutput
           FFTpoint = CFFT_f32_getCurrOutputPtr(hnd_cfft);

//           // Put the magnitude and phase info into a form that the IFFT understands
//           // will this need to come before or after pack?
//           for(Uint16 j = 0; j< (FFT_SIZE/2 - 1); j++){
//               FFTin[j] = FFTmag[j]*cosf(FFTphase[j]); // I*sinf(FFTphase[j])
//               FFTin[j+1] = (FFTmag[j]*sinf(FFTphase[j]));
//           }

           // reset the input and output pointers
           CFFT_f32_setCurrInputPtr(hnd_cfft, &FFTin[0]);
           CFFT_f32_setCurrOutputPtr(hnd_cfft, &FFTout[0]);

           CFFT_f32_pack(hnd_cfft);

           FFTpoint = CFFT_f32_getCurrOutputPtr(hnd_cfft);
           CFFT_f32_setInputPtr(hnd_cfft, FFTpoint);
           FFTpoint = CFFT_f32_getCurrInputPtr(hnd_cfft);
           CFFT_f32_setOutputPtr(hnd_cfft, FFTpoint); // at this point, FFTout is pointed to by InPtr, not FFTin

           // Run the N/2 point inverse complex FFT
           // NOTE: Can only use the 't' version of the ICFFT with the
           // pack and unpack functions
           ICFFT_f32t(hnd_cfft); // changed this to the t version thanks to the above note

           for(Uint16 j = 0; j< (FFT_SIZE); j++) // check the output
               {
                   Output[j] = (int16)FFTin[j];
               }

           CFFT_f32_setInputPtr(hnd_cfft, FFTin);
           CFFT_f32_setOutputPtr(hnd_cfft, FFTout);

           for(Uint16 j = 0; j< (FFT_SIZE); j++) // check the output
               {
               FFTin [j] = 0;
               FFTout[j] = 0;
               }

            FFTflag = 0;

        }

        Counter++;

    }

}

interrupt void local_D_INTCH6_ISR(void)
{
    EALLOW;

    for(Uint16 i = 0; i< FFT_SIZE; i++)
    {
        Hann = 0.5f*(1 - cosf(2.0f * M_PI * i / (FFT_SIZE - 1)));
        FFTin[i] = ((float32)Input[i])*Hann;
    }
    FFTflag = 1;
    Counter = 0;

    // ACK to receive more interrupts
    PieCtrlRegs.PIEACK.all |= PIEACK_GROUP7;

    //EDIS;
}

interrupt void local_D_INTCH5_ISR(void)
{

    EALLOW;

    // ACK to receive more interrupts
    PieCtrlRegs.PIEACK.all |= PIEACK_GROUP7;

    EDIS;

}
