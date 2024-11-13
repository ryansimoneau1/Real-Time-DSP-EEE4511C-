/*
 PhaseVocoder.c
 Ryan Simoneau
 Last Modified: Mar, 17 2021

 Description: This program implements a phase vocoder using DMA, FFT, and Hanning windows

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

// --------- Defines ---------- //
// FFT includes
#define FFT_STAGES 9
#define FFT_SIZE   (1 << FFT_STAGES)
#define BURST 1 //BURST is number of burst - 1 in an xfer
#define TRANSFER 511 //TRANSFER is number of bursts - 1 in an xfer

// Buffer includes
#define HOPi 128
#define HOPo 190

// ---------------------------- //


// --- Variables and Arrays --- //

// Circular Buffers
int16   InCir [HOPi*5];
float32 OutCir[HOPo*7];
Uint16  HopNum     = 0; // Dictates which section of InCir the FFT will be computed on
Uint16  TransCount = 0; // Dictates when InCir is filled with enough data for the FFT to be computed
Uint16  TransComp  = 0; // indicates when a transfer is complete

// Intermediate buffers
float32 CurrentFrame[FFT_SIZE]; // input will be of form: R(0), 0, R(1), ... // Will also be used as the input for the FFT
float32 OutFrame    [FFT_SIZE];

// FFT Buffers
float32 FFTF32Coef[FFT_SIZE];
float32 FFTin     [FFT_SIZE*2];
float32 FFTout    [FFT_SIZE*2]; // Output buffer for FFT/IFFT
float32 FFTphase  [FFT_SIZE*2]; // Phase of FFT
float32 FFTmag    [FFT_SIZE*2]; // Magnitude of FFT
float32 Hann    = 0;
Uint16  FFTflag = 0;

CFFT_F32_STRUCT cfft;
CFFT_F32_STRUCT_Handle hnd_cfft = &cfft;
// ---------------------------- //

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

    // Clear input buffers:
    for(Uint16 i=0; i < (FFT_SIZE*2); i=i+2){
        FFTin [i]   = 0.0f;
        FFTin [i+1] = 0.0f;
        FFTout[i]   = 0.0f;
        FFTout[i+1] = 0.0f;
    }


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
    DMACH6TransferConfig(127,-1,0); // increments between bursts //
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

    while(TransCount < 4); // wait for the first 4 sections of InCir to be filled with data

    while(1){




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













