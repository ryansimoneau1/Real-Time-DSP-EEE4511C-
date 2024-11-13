/* Author: Dave Muscle Russell
* Blink BLUE LED on Launchpad, don't forget your EALLOWs
*/
#include <F28x_Project.h>

int main(void)
{
    //init system clocks and get board speed running at 200 MHz
    //your board will run at 100 MHz if you do not have all the predefines!
InitSysCtrl();

    EALLOW;
    GpioCtrlRegs.GPADIR.bit.GPIO31 = 1; //set blue light on LP as an output

while(1){

    //delay for 1 second
    DELAY_US(1E6);
    //toggle the light
    EALLOW;
    GpioDataRegs.GPATOGGLE.bit.GPIO31 = 1;
    }

}
