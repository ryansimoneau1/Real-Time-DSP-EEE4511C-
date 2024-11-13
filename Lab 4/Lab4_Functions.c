/*
 Lab4_Functions.c
 Ryan Simoneau
 Last Modified: Feb, 13 2021

 Description:

*/

#include <F2837xD_Device.h>
#include "OneToOneI2CDriver.h"
#include "Lab4_Functions.h"

// function to send any single command to the LCD
        void LCDCTRL(Uint16 command){

           Uint16 x[4];

           x[0]  = (command & 0xF0) | 0xC;
           x[1]  = (command & 0xF0) | 0x8;

           x[2]  = (command << 4) | 0xC;
           x[2]  = (x[2] & 0x00FF);
           x[3]  = (command << 4) | 0x8;
           x[3]  = (x[3] & 0x00FF);

           I2C_O2O_SendBytes(&x[0], 4);

        }


        void LCDDATA(Uint16 data){

            Uint16 x[4];

            x[0]  = (data & 0xF0) | 0xD;
            x[1]  = (data & 0xF0) | 0x9;

            x[2]  = (data << 4) | 0xD;
            x[2]  = (x[2] & 0x00FF);
            x[3]  = (data << 4) | 0x9;
            x[3]  = (x[3] & 0x00FF);

            I2C_O2O_SendBytes(&x[0], 4);

        }


        // function with hardcodded values meant to initialize the LCD
         void LCDINIT(void){


            #define LCDinits    5

            Uint16  LCDI[LCDinits]  = {0x33, 0x32, 0x28, 0x0F, 0x01};

            Uint16 temp[20];
            // 0x33
            temp[0]  = (LCDI[0] & 0xF0) | 0xC;
            temp[1]  = (LCDI[0] & 0xF0) | 0x8;

            temp[2]  = (LCDI[0] << 4) | 0xC;
            temp[2]  = (temp[2] & 0x00FF);
            temp[3]  = (LCDI[0] << 4) | 0x8;
            temp[3]  = (temp[3] & 0x00FF);


            // 0x32
            temp[4]  = (LCDI[1] & 0xF0) | 0xC;
            temp[5]  = (LCDI[1] & 0xF0) | 0x8;

            temp[6]  = (LCDI[1] << 4) | 0xC;
            temp[6]  = (temp[6] & 0x00FF);
            temp[7]  = (LCDI[1] << 4) | 0x8;
            temp[7]  = (temp[7] & 0x00FF);

            // 0x28
            temp[8]  = (LCDI[2] & 0xF0) | 0xC;
            temp[9]  = (LCDI[2] & 0xF0) | 0x8;

            temp[10]  = (LCDI[2] << 4) | 0xC;
            temp[10]  = (temp[10] & 0x00FF);
            temp[11]  = (LCDI[2] << 4) | 0x8;
            temp[11]  = (temp[11] & 0x00FF);

            // 0x0F
            temp[12] = (LCDI[3] & 0xF0) | 0xC;
            temp[13] = (LCDI[3] & 0xF0) | 0x8;

            temp[14] = (LCDI[3] << 4) | 0xC;
            temp[14]  = (temp[14] & 0x00FF);
            temp[15]  = (LCDI[3] << 4) | 0x8;
            temp[15]  = (temp[15] & 0x00FF);

            // 0x01
            temp[16] = (LCDI[4] & 0xF0) | 0xC;
            temp[17] = (LCDI[4] & 0xF0) | 0x8;

            temp[18] = (LCDI[4] << 4) | 0xC;
            temp[18]  = (temp[18] & 0x00FF);
            temp[19]  = (LCDI[4] << 4) | 0x8;
            temp[19]  = (temp[19] & 0x00FF);

            I2C_O2O_SendBytes(&temp[0], 20);

        }

         void LCDSTRING(char * const string, Uint16 length){

             for (Uint16 i = 0; i < length; i++)
                 {
                     LCDDATA(string[i]);
                 }


         }

         void SPIINIT(void){

             // config spi initializations
             SpibRegs.SPICCR.all = 0x0027;
             SpibRegs.SPICTL.all = 0x000E;

             // div 2 (100MHz)
             ClkCfgRegs.LOSPCP.all = 0x00000001;

             // div 4 (25MHz)
             SpibRegs.SPIBRR.all = 0x0003;

             SpibRegs.SPIPRI.all = 0x0010;


             // configure GPIO for SPI
             GpioCtrlRegs.GPBMUX2.all  = 0xC0000000;
             GpioCtrlRegs.GPBGMUX2.all = 0xC0000000;
             GpioCtrlRegs.GPCMUX1.all  = 0x0000000F; // configures SPI on pins 63,64, and 65
             GpioCtrlRegs.GPCGMUX1.all = 0x0000000F;
             GpioCtrlRegs.GPBQSEL2.all = 0xC0000000;
             GpioCtrlRegs.GPCQSEL1.all = 0x03;
             GpioDataRegs.GPCSET.all   = 0x0000000C; // only the chip selects are outputs and they are initially high
             GpioCtrlRegs.GPCDIR.all   = 0x0000000C;

             // enable the spi system
             SpibRegs.SPICCR.all = 0x00A7;


         }

         Uint16 SPITRANSMIT(Uint16 data){

             Uint16 transfer;
             volatile Uint16 receive;

             while(SpibRegs.SPISTS.bit.BUFFULL_FLAG){}

             transfer = data;
             SpibRegs.SPITXBUF = transfer;

             while(SpibRegs.SPISTS.bit.INT_FLAG !=1){} // after this while loop, good data will be available

             receive = SpibRegs.SPIRXBUF;


             return(receive); // return what was recieved in case we want it

         }

         void SRAMWRITE(Uint16 data, Uint32 address){


             Uint16 sramData = data;
             Uint16 volatile dummy; // a dummy variable for the value that SPI recieves upon writing to the sram modules
             Uint16 writeCMD = 0x0200; // a 2 in the MSB for write



             if(address >= 0x400000){

             Uint32 ramaddr  = (address - 0x400000); // this will be the modified ram address for the 2nd ram module
             Uint16 ramaddrH = (ramaddr >> 16); // !! what's loaded into these !!
             Uint16 ramaddrL = (ramaddr);

             GpioDataRegs.GPCDAT.bit.GPIO67 = 0;

             // send the command for write
             dummy = SPITRANSMIT(writeCMD);

             // send the upper bits of the address
             dummy = SPITRANSMIT(ramaddrH << 8);
             // send the next 8 bits
             dummy = SPITRANSMIT(ramaddrL);
             // send the next 8 bits
             dummy = SPITRANSMIT(ramaddrL << 8);
             // send the data
             dummy = SPITRANSMIT(sramData);
             dummy = SPITRANSMIT(sramData << 8);

             GpioDataRegs.GPCDAT.bit.GPIO67 = 1;


             }else{

             Uint32 ramaddr  = address; // this will be the modified ram address for the 1st ram module
             Uint16 ramaddrH = (ramaddr >> 16); // !! what's loaded into these !!
             Uint16 ramaddrL = (ramaddr);

             GpioDataRegs.GPCDAT.bit.GPIO66 = 0;

             // send the command for write
             dummy = SPITRANSMIT(0x0200);
             // send the upper bits of the address
             dummy = SPITRANSMIT(ramaddrH << 8);
             // send the next 8 bits
             dummy = SPITRANSMIT(ramaddrL);
             // send the next 8 bits
             dummy = SPITRANSMIT(ramaddrL << 8);
             // send the data
             dummy = SPITRANSMIT(sramData);
             dummy = SPITRANSMIT(sramData << 8);

             GpioDataRegs.GPCDAT.bit.GPIO66 = 1;

             }

         }

         Uint16 SRAMREAD(Uint32 address){

             Uint16 transfer = 0x0000; // dummy data used to recieve data from the SRAM
             Uint16 volatile SRAMdata; // a dummy variable for the value that SPI recieves upon writing to the sram modules
             Uint16 readCMD = 0x0300; // a 2 in the MSB for write



             if(address >= 0x400000){

             Uint32 ramaddr  = (address - 0x400000); // this will be the modified ram address for the 2nd ram module
             Uint16 ramaddrH = (ramaddr >> 16); // !! what's loaded into these !!
             Uint16 ramaddrL = (ramaddr);

             GpioDataRegs.GPCDAT.bit.GPIO67 = 0;

             // send the command for read
             SRAMdata = SPITRANSMIT(readCMD);

             // send the upper bits of the address
             SRAMdata = SPITRANSMIT(ramaddrH << 8);
             // send the next 8 bits
             SRAMdata = SPITRANSMIT(ramaddrL);
             // send the next 8 bits
             SRAMdata = SPITRANSMIT(ramaddrL << 8);
             // send the data
             SRAMdata = SPITRANSMIT(transfer); // extra dummy cycle
             SRAMdata = SPITRANSMIT(transfer);
             SRAMdata = SPITRANSMIT(transfer);

             GpioDataRegs.GPCDAT.bit.GPIO67 = 1;


             }else{

             Uint32 ramaddr  = address; // this will be the modified ram address for the 1st ram module
             Uint16 ramaddrH = (ramaddr >> 16); // !! what's loaded into these !!
             Uint16 ramaddrL = (ramaddr);

             GpioDataRegs.GPCDAT.bit.GPIO66 = 0;

             // send the command for write
             SRAMdata = SPITRANSMIT(readCMD);

             // send the upper bits of the address
             SRAMdata = SPITRANSMIT(ramaddrH << 8);
             // send the next 8 bits
             SRAMdata = SPITRANSMIT(ramaddrL);
             // send the next 8 bits
             SRAMdata = SPITRANSMIT(ramaddrL << 8);
             // send the data
             SRAMdata = SPITRANSMIT(transfer); // extra dummy cycle
             SRAMdata = SPITRANSMIT(transfer);
             SRAMdata = SPITRANSMIT(transfer);

             GpioDataRegs.GPCDAT.bit.GPIO66 = 1;
             }

             return(SRAMdata);

         }
























