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
#include "fpu.h"
#include "Lab4_Functions.h"
#include "AIC23.h"
#include "CommonDrivers.h"
#include "OneToOneI2CDriver.h"

// FFT/IFFT Includes
#include "fpu_rfft.h"
#include "dsp.h"
#include "fpu32/fpu_cfft.h"

// -- Defines -- //
#define RFFT_STAGES 9
#define RFFT_SIZE   (1 << RFFT_STAGES)
#define BURST 1 //BURST is number of burst - 1 in an xfer
#define TRANSFER 511 //TRANSFER is number of bursts - 1 in an xfer


// ------------- //


// -- Variables and Arrays -- //

// FFT Variables
int16           Input      [RFFT_SIZE];
float32         RFFTin1Buff[RFFT_SIZE];
float32         RFFToutBuff[RFFT_SIZE];
float32         RFFTmagBuff[RFFT_SIZE/2 + 1];
float32         RFFTphsBuff[RFFT_SIZE/2];
float32         RFFTF32Coef[RFFT_SIZE];
float32         Hann       = 0;
Uint16          FFTflag    = 0;
Uint16          Counter    = 0;
volatile Uint16 MagTest   [RFFT_SIZE/2 + 1];

// IFFT Variables
float32 *IFFTpointer;
float32 IFFTinBuff      [RFFT_SIZE];
float32 IFFToutBuff     [RFFT_SIZE];
volatile Uint16 OutTest [RFFT_SIZE/2];

float32 SCALE = 0.001953125;

// -------------------------- //
// FFT Data sections
#pragma DATA_SECTION(Input,       "ramgs0")
#pragma DATA_SECTION(RFFTin1Buff, "ramgs1")
#pragma DATA_SECTION(RFFToutBuff, "ramgs2")
#pragma DATA_SECTION(RFFTF32Coef, "ramgs3")

#pragma DATA_SECTION(IFFTinBuff,  "ramgs4")
#pragma DATA_SECTION(IFFToutBuff, "ramgs5")
#pragma DATA_SECTION(OutTest,     "ramgs6")



// FFT Struct
RFFT_F32_STRUCT rfft;
RFFT_F32_STRUCT_Handle hnd_rfft = &rfft;

// IFFT Struct
CFFT_F32_STRUCT cfft;
CFFT_F32_STRUCT_Handle hnd_cfft = &cfft;

interrupt void local_D_INTCH6_ISR(void);

int main(void){

    InitSysCtrl();      // Set SYSCLK to 200 MHz, disables watchdog timer, turns on peripheral clocks

    EALLOW;
    DINT;               // Disable CPU interrupts on startup
    // Init PIE
    InitPieCtrl();      // Initialize PIE -> disable PIE and clear all PIE registers
    IER = 0x0000;       // Clear CPU interrupt register
    IFR = 0x0000;       // Clear CPU interrupt flag register
    InitPieVectTable(); // Initialize PIE vector table to known state

    // clear buffers
    for(Uint16 i = 0; i < RFFT_SIZE; i++){
        Input      [i] = 0;
        RFFTin1Buff[i] = 0;
        RFFToutBuff[i] = 0;
        RFFTmagBuff[i] = 0;

        IFFTinBuff [i] = 0;
        IFFToutBuff[i] = 0;
    }

    // FFT Buffer Initialization
    rfft.InBuf         = &RFFTin1Buff[0];
    rfft.OutBuf        = &RFFToutBuff[0];
    rfft.MagBuf        = &RFFTmagBuff[0];
    rfft.PhaseBuf      = &RFFTphsBuff[0];
    rfft.FFTSize       = RFFT_SIZE;
    rfft.FFTStages     = RFFT_STAGES;
    rfft.CosSinBuf     = &RFFTF32Coef[0];

    RFFT_f32_sincostable(&rfft);

    // Configure IFFT Object
    cfft.InPtr         = &IFFTinBuff [0];
    cfft.OutPtr        = &IFFToutBuff[0];
    cfft.CurrentInPtr  = &IFFTinBuff [0];
    cfft.CurrentOutPtr = &IFFToutBuff[0];
    cfft.FFTSize       = RFFT_SIZE;
    cfft.Stages        = RFFT_STAGES;

    cfft.CoefPtr = &RFFTF32Coef[0];
    CFFT_f32_pack(&cfft);

    EALLOW;
    // enable SPIA and McBSPb along with the AIC23 and GPIO for DIP, PBs, and LEDs
    InitSPIA();
    InitAIC23();
    InitMcBSPb();

    EALLOW;
    // Set DMA CH6 transfer complete interrupts to ISR
    PieVectTable.DMA_CH6_INT= &local_D_INTCH6_ISR;

    // Performs a hard reset on the DMA
    DMAInitialize();

    // source and destination pointers
    volatile Uint16 *DMA_CH6_Source = (volatile Uint16 *)&McbspbRegs.DRR2.all;
    volatile Uint16 *DMA_CH6_Dest = (volatile Uint16 *)&Input[0];

    // Initialize CH6 source and destination addresses
    DMACH6AddrConfig(DMA_CH6_Dest,DMA_CH6_Source); // may need this in the ISR //

    // Configures the burst size and source/destination step size

    DMACH6BurstConfig(BURST,1,1); // does not increment between words //

    // Configures the transfer size and source/destination step size
    // Transfer size is 1 burst (0 + 1)
    DMACH6TransferConfig(TRANSFER,-1,0); // increments between bursts //

    // Configures source and destination wrapping
    DMACH6WrapConfig(0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF);

    // CH6 mode configuration:
    DMACH6ModeConfig(74,PERINT_ENABLE,ONESHOT_DISABLE,CONT_ENABLE,
                     SYNC_DISABLE,SYNC_SRC,OVRFLOW_DISABLE,SIXTEEN_BIT,
                     CHINT_END,CHINT_ENABLE);

    // Dual ported bridge connected to DMA
    EALLOW;
    CpuSysRegs.SECMSEL.bit.PF2SEL = 1;
    EDIS;

    // Interrupt enabling
    // PIE group 7, interrupt 6 -> DMA CH6
    PieCtrlRegs.PIEIER7.bit.INTx6 = 1;
    IER |= M_INT7;

    CODECGPIOINIT();

    // Enables the PIE
    // Clears PIEACK register
    // Enables global interrupts
    EnableInterrupts();

    // Start DMA CH6
    StartDMACH6();

    while(1)
    {

       if(FFTflag)
        {

            // FFT Computations
            RFFT_f32(&rfft);

            RFFT_f32_mag_TMU0(&rfft);

            RFFT_f32_phase_TMU0(&rfft);

            for(Uint16 j = 0; j< (RFFT_SIZE/2); j++) // check the output
                {
                    MagTest   [j] = (Uint16)RFFTmagBuff[j];
                    IFFTinBuff[j] = SCALE*(RFFTmagBuff[j] + I*RFFTphsBuff[j]);
                }



            IFFTpointer = CFFT_f32_getCurrOutputPtr(&cfft);
            CFFT_f32_setInputPtr(&cfft, IFFTpointer);
            IFFTpointer = CFFT_f32_getCurrInputPtr(&cfft);
            CFFT_f32_setOutputPtr(&cfft, IFFTpointer);

            // IFFT Computation
            ICFFT_f32t(&cfft);

            for(Uint16 j = 0; j< (RFFT_SIZE); j++) // check the output
                {
                    OutTest[j] = (Uint16)IFFToutBuff[j];
                }

            FFTflag = 0;

        }

        Counter++;

    }

}

interrupt void local_D_INTCH6_ISR(void)
{
    EALLOW;

    for(Uint16 i = 0; i< RFFT_SIZE; i++)
    {
        Hann = 0.5f*(1 - cosf(2.0f * M_PI * i / (RFFT_SIZE - 1)));
        RFFTin1Buff[i] = ((float32)Input[i])*Hann;
    }
    FFTflag = 1;
    Counter = 0;

    // ACK to receive more interrupts
    PieCtrlRegs.PIEACK.all |= PIEACK_GROUP7;

    EDIS;
}
