// Colin Adema
// DMA sound-in / sound-out

#include <F28x_Project.h>
#include "AIC23.h"
#include "CommonDrivers.h"

//BURST is number of burst - 1 in an xfer
//TRANSFER is number of bursts - 1 in an xfer
#define BURST 1
#define TRANSFER 0


interrupt void local_D_INTCH6_ISR(void);

int main(void)
{

    // Turn off watchdog timer
    // Set clock to 200MHz
    // Turn on peripheral clocks
    InitSysCtrl();

    // Disable global interrupt
    DINT;

    // Initialize PIE
    // Disable all interrupt groups and clears all interrupt flags
    // Initialize PIE vector table
    InitPieCtrl();
    IER = 0x0000;
    IFR = 0x0000;
    InitPieVectTable();

    // Initialize codec and McBSP
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
    volatile Uint16 *DMA_CH6_Dest = (volatile Uint16 *)&McbspbRegs.DXR2.all;

    // Initialize CH6 source and destination addresses
    DMACH6AddrConfig(DMA_CH6_Dest,DMA_CH6_Source);

    // Configures the burst size and source/destination step size
    // Burst size is 2 16-bit words (1 + 1)
    // Source address increments by 1 after word is transmitted (DRR2 -> DRR1)
    // Destination address increments by 1 after word is transmitted (DXR2 -> DXR1)
    DMACH6BurstConfig(1,0,6);

    // Configures the transfer size and source/destination step size
    // Transfer size is 1 burst (0 + 1)
    // Since transfer size is only 1 burst, no need for source address changing after burst
    // Since transfer size is only 1 burst, no need for destination address changing after burst
    DMACH6TransferConfig(0,2,6);

    // Configures source and destination wrapping
    // Source wrapping doesn't matter -> set to 0xFFFF so it's ignored
    // Destination wrapping doesn't matter -> set to 0xFFFF so it's ignored
    DMACH6WrapConfig(0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF);

    // CH6 mode configuration:
    // Burst triggers after McBSPb RX, oneshot disabled, continuous mode enable
    // 16-bit data, interrupt enabled, interrupt triggers at end of transfer
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

    // Enables the PIE
    // Clears PIEACK register
    // Enables global interrupts
    EnableInterrupts();

    // Start DMA CH6
    StartDMACH6();


    // Loop
    while(1);

}

// DMA ISR
interrupt void local_D_INTCH6_ISR(void)
{
    EALLOW;

    // ACK to receive more interrupts
    PieCtrlRegs.PIEACK.all |= PIEACK_GROUP7;

    EDIS;
}
