/*
 Lab_4_Part_2.c
 Ryan Simoneau
 Last Modified: Feb, 13 2021

 Description: This program displays my name and the name of the class on the 2nd line

*/

#include <F28x_Project.h>
#include "OneToOneI2CDriver.h"
#include "Lab4_Functions.h"

#define Nlength     13
#define EEL4511C    8


char    Name[Nlength]   = "Ryan Simoneau";
char    Class[EEL4511C] = "EEL4511C";




int main(void){

    InitSysCtrl();


    EALLOW;

    I2C_O2O_Master_Init(0x27, 200.0, 12);


    LCDINIT();


    LCDSTRING(Name, 13);

    // go to the next line
    LCDCTRL(0xC0);

    // display the course name
    LCDSTRING(Class, 8);



    while(1); // infinite loop

}
