/*
 Lab_4_Part_3.c
 Ryan Simoneau
 Last Modified: Feb, 16 2021

 Description: This program stores and reads memory on a serial SRAM

*/


#include <F28x_Project.h>
#include "OneToOneI2CDriver.h"
#include "Lab4_Functions.h"




int main(void){

    InitSysCtrl();


    EALLOW;

    SPIINIT();

    while(1){
        // code to test RAM functionality
        //SRAMWRITE(0xB00B, 0x007FFFAA);
       // Uint16 cnt = 10;
       // cnt = cnt -1;
       // cnt = cnt -1;
       // cnt = cnt -1;
       // cnt = cnt -1;
       // cnt = cnt -1;
       // cnt = cnt -1;
       // cnt = cnt -1;
       // cnt = cnt -1;

        volatile Uint16 Val;
        Val = SRAMREAD(0x007FFFAA);

        }

    while(1); // infinite loop
}



