// Karl's MS G4 Transmitter Code
// Nordic packet: NO_FFTS, FFT_SIZE, 32 Floating Pt. Magnitude Sums
#include "DSP28x_Project.h"
#include "Nordic_TX.h"
#include <FPU.h>
#include <math.h>
#define FFT_SIZE        256         //32, 64, 128, 256, etc.
#define FFT_STAGES      8       //log2(FFT_SIZE)
#define NO_FFTS         1       //number of FFTs to sample & process
                                //Linker file reserves space for up to 32*64 bins

#define Nordic_CSN_low  GpioDataRegs.GPACLEAR.bit.GPIO20 = 1     //Nordic Interface
#define Nordic_CSN_high GpioDataRegs.GPASET.bit.GPIO20 = 1
#define Nordic_CE_low   GpioDataRegs.GPACLEAR.bit.GPIO21 = 1
#define Nordic_CE_high  GpioDataRegs.GPASET.bit.GPIO21 = 1
#define NordicIRQ       GpioDataRegs.GPADAT.bit.GPIO24
#define PACKET_BYTES    288         //NO_FFTS, FFT_SIZE, 32 Mag Sums = 136 bytes + 24 dummy bytes to
                                    //create 160 bytes = 5x32 bytes = 5 packets

void            System_init(void);
void            FFT_init(void);
void            Transmit_Data(void);

float           samplebuffer[FFT_SIZE*NO_FFTS];
float           OutBuffer[FFT_SIZE];
float           TwiddleBuffer[FFT_SIZE];
float           MagBuffer[FFT_SIZE/2];
float           SumSpectrum[FFT_SIZE/2];

float           Hanning[FFT_SIZE] =
{0,0.000151774,0.000607003,0.001365411,0.002426538,0.003789739,0.005454187,0.007418871,0.009682598,0.012243995,0.0
15101507,0.018253397,0.021697754,0.025432485,0.029455324,0.033763829,0.038355383,0.043227199,0.04837632,0.05379961
9,0.059493804,0.065455419,0.071680843,0.078166298,0.084907845,0.091901394,0.099142696,0.106627358,0.114350833,0.12
2308435,0.130495331,0.138906552,0.14753699,0.156381408,0.165434434,0.174690573,0.184144206,0.193789594,0.203620881
,0.213632097,0.223817167,0.234169905,0.244684028,0.255353151,0.266170799,0.277130403,0.28822531,0.299448785,0.3107
94013,0.322254107,0.33382211,0.345490998,0.357253688,0.369103039,0.381031857,0.393032899,0.405098881,0.417222477,0
.429396327,0.441613039,0.453865199,0.466145366,0.478446086,0.490759892,0.503079307,0.515396853,0.527705052,0.53999
6431,0.552263528,0.564498896,0.576695108,0.588844758,0.600940471,0.612974903,0.624940749,0.636830744,0.64863767,0.
660354359,0.671973697,0.683488631,0.69489217,0.706177391,0.717337442,0.728365549,0.739255017,0.749999234,0.7605916
78,0.771025917,0.781295619,0.791394547,0.801316571,0.811055667,0.820605923,0.82996154,0.83911684,0.848066263,0.856
804377,0.865325877,0.87362559,0.881698476,0.889539635,0.897144306,0.904507873,0.911625866,0.918493962,0.925107993,
0.931463942,0.937557953,0.943386324,0.948945517,0.954232157,0.959243036,0.963975111,0.968425508,0.972591527,0.9764
70637,0.980060485,0.983358891,0.986363851,0.989073543,0.99148632,0.993600719,0.995415455,0.996929427,0.998141715,0
.999051584,0.999658482,0.999962039,0.999962071,0.99965858,0.999051748,0.998141944,0.99692972,0.995415814,0.9936011
42,0.991486808,0.989074095,0.986364467,0.98335957,0.980061227,0.976471442,0.972592393,0.968426436,0.9639761,0.9592
44085,0.954233267,0.948946685,0.94338755,0.937559237,0.931465283,0.92510939,0.918495414,0.911627372,0.904509433,0.
897145918,0.889541298,0.88170019,0.873627353,0.865327689,0.856806236,0.848068168,0.83911879,0.829963534,0.82060795
9,0.811057745,0.801318688,0.791396703,0.781297813,0.771028147,0.760593942,0.750001532,0.739257347,0.72836791,0.717
339832,0.706179808,0.694894614,0.6834911,0.671976189,0.660356872,0.648640204,0.636833297,0.624943319,0.612977488,0
.60094307,0.588847369,0.57669773,0.564501528,0.552266167,0.539999076,0.527707701,0.515399506,0.503081961,0.4907625
45,0.478448737,0.466148013,0.453867841,0.441615675,0.429398954,0.417225094,0.405101486,0.393035491,0.381034434,0.3
691056,0.357256231,0.345493522,0.333824612,0.322256587,0.310796469,0.299451215,0.288227714,0.277132778,0.266173144
,0.255355466,0.244686309,0.234172152,0.223819379,0.213634273,0.203623018,0.193791692,0.184146264,0.174692588,0.165
436406,0.156383335,0.147538873,0.138908387,0.130497119,0.122310174,0.114352522,0.106628996,0.099144282,0.091902927
,0.084909325,0.078167722,0.071682212,0.065456731,0.05949506,0.053800817,0.048377459,0.043228279,0.038356402,0.0337
64788,0.029456222,0.025433321,0.021698527,0.018254108,0.015102154,0.012244579,0.009683118,0.007419326,0.005454578,
0.003790065,0.002426799,0.001365607,0.000607134,0.000151839,0.0};
RFFT_F32_STRUCT     fft;
float               TempSum;
unsigned long       Nordic_Fail_Counter;
char                Nordic_Fail_Flg;        // if counter overflows, flg is set & the entire transmit
                                            // event is dumped
unsigned long       TXdata[PACKET_BYTES];

#pragma DATA_SECTION(samplebuffer, "INBUFA"); //Align the INBUF section to 2*FFT_SIZE in the linker file


void main(void)
{

    //LPM_ini();                            //Configure Low Power Mode (only needed for idle)
    EALLOW;
    FlashRegs.FPWR.all = 0;                 //Turn off flash module
    EDIS;
    System_init();                          //Initialize Peripheral Clock

    EALLOW;                                 //Nordic GPIO Setup
    GpioCtrlRegs.GPADIR.all = 0x0005;       // GPIO0-GPIO31 are inputs.... 0, 2, are output
    GpioCtrlRegs.GPAPUD.bit.GPIO24 = 1;     //diable pullup on nordic IRQ line
    GpioDataRegs.GPADAT.bit.GPIO24 = 0;
    EDIS;

    sample_count = 0;                       //Initialize sample counter
    FFT_init();                             //Initialize FFT Configuration
    SetupNordicTX();                        //Initialize Nordic
    Nordic_Fail_Counter = 0;

    // while(1){ //this while loop is used for testing code...comment out when finished testing

        Nordic_Fail_Flg = 0;
        Zero_SumSpectrum();                 //Zero out Spectrum Sum values

        InitPll(1,1);                       //Clk Speed 1X (2,2), 2X (4,2), 3X (6,2), 4X (8,2)
                                            //5X (10,2) first number multiplies, second number (1) = /4
                                            //(2) = /2, (3) = /1 does not work

        for(i=0;i<FFT_SIZE;i++)
        {

            samplebuffer[i] = samplebuffer[i] * Hanning[i];

        }
            for(j=0;j<(NO_FFTS);j++)                        //COMPUTE ALL FFTs and MAGNITUDES
            {
                fft.InBuf = &samplebuffer[j*FFT_SIZE];      //Set FFT input ptr to original sample buffer
                RFFT_f32(&fft);                             //Real FFT Calc (read from sample buffer)
                fft.MagBuf = &samplebuffer[j*(FFT_SIZE/2)]; //Set Mag output ptr to orig sample buf
                RFFT_f32_mag(&fft);                         //Real Mag Calc (write back to sample buf)
            }
            for(j=0;j<(NO_FFTS);j++)                        //COMPUTE SUM SPECTRUM
            {
                for(i=0;i<(FFT_SIZE/2);i++)
                {
                    TempSum = SumSpectrum[i] + samplebuffer[(j*(FFT_SIZE/2))+i];
                    SumSpectrum[i] = TempSum;
                }
            }

        SumSpectrum[0] = 0.0;                               //make DC term zero

        if (Channel == 0)                                   //CONSTANT CARRIER TEST CODE
        {
            LED_on;
            NordicWReg(RF_CH, 40);                          //Write RF Channel Register
            NordicWReg(RF, 0x86);
            Nordic_CE_high;                                 // pulse CE high for 10uS t start packet transmit
            while(1){}
        }

        LED_on;
        Transmit_Data(); //NORDIC TRANSMIT DATA
        LED_off;
    // }//end of test while loop...comment out when finished testing
}



void Zero_SumSpectrum(void)
{
    for(i=0;i<(FFT_SIZE/2);i++)     //Zero Out SumSpectrum values
    {
            SumSpectrum[i]= 0.0;
    }
}

void Zero_SampleBuffer(void)
{
    for(j=0;j<(NO_FFTS);j++)        //DEBUG PURPOSES - load sample buffer with zeros
    {
        for(i=0;i<(FFT_SIZE);i++)
        {
            samplebuffer[j*FFT_SIZE+i]= 0.0;
        }
    }
}
void System_init(void)
{
                                    //SYSTEM INITIALIZATION 2MHZ XTAL
    InitSysCtrl();
    InitPll(2,2);                   //Clk Speed 1X (2,2), 2X (4,2), 3X (6,2), 4X (8,2) 5X (10,2) first number
                                    //multiplies, second number (1) = /4, (2) = /2, (3) = /1 does not work
}
void FFT_init(void)
{

    //FFT INITIALIZATION
    fft.InBuf = samplebuffer;       //Input data buffer
    fft.OutBuf = OutBuffer;         //FFT output buffer
    fft.CosSinBuf = TwiddleBuffer;  //Twiddle factor buffer
    fft.FFTSize = FFT_SIZE;         //FFT length
    fft.FFTStages = FFT_STAGES;     //FFT Stages
    fft.MagBuf = MagBuffer;         //Magnitude buffer
    RFFT_f32_sincostable(&fft);     //Initialize Twiddle Buffer

}

typedef Uint16      UBYTE;          /* Unsigned 8 bit quantity */
typedef Uint16      UWORD;          /* Unsigned 16 bit quantity */

void RF_calc_crc(unsigned long *pdata, int count)
{
UWORD CRC, temp;
unsigned long value;

int i,j,k;
CRC=0xffff;

    for(i=0;i<count;i++)
    {
        value=*pdata++;
        for(j=0;j<4;j++)
        {
            temp=value>>(8*j);
            temp=temp & 0x00FF;
            CRC=CRC ^ temp;

            for(k=0;k<8;k++)
            {
                temp=CRC & 0x0001;
                CRC=CRC >> 1;
                if ((temp & 0x0001)== 0x0001)
                    CRC=CRC ^ 0xA001;
            }
        }
    }
    *pdata++= (UBYTE) CRC & 0x00ff;
    temp = CRC >> 8;
    *pdata = (UBYTE)temp & 0x00ff;
}

void Transmit_Data(void)
{
    volatile unsigned short ret;
    int i= 0,j = 0,k = 0, word_count= 0;
    float inter;

    TXdata[0] = (unsigned long) NO_FFTS;    //Build TX data packet
    TXdata[1] = (unsigned long) FFT_SIZE;   //FFT Size
    TXdata[2] = 1;                          //Battery Good

    for(i=0;i<(FFT_SIZE/4);i++)
    {
        memcpy(&TXdata[i+3],&SumSpectrum[i],sizeof(float));
    }

    for(i=0;i<5;i++)
    {
        TXdata[i+67] = 0xfab4; //5 dummy values
    }
    RF_calc_crc(TXdata,67); //Calc CRC16
    for(k = 0;k < PACKET_BYTES/32; k++)     //32 bytes per RF trans. event (8x32 bit words per tx event)
    {
        Nordic_CSN_low;                                     //enable nordic to recieve commands
        SPITX(W_TX_PAYLOAD);                                //send write payload command
        while (SpiaRegs.SPIFFRX.bit.RXFFINT == 0);          //wait for transmission to complete
        ret = SpiaRegs.SPIRXBUF;                            //grab received char for return
        SpiaRegs.SPIFFRX.bit.RXFFINTCLR = 1;                //clear flag
        for(i = 0;i <= 7; i++)
        {

            word_count++;
            SPITX(TXdata[i+j] >> 24);                       //send high byte
            while (SpiaRegs.SPIFFRX.bit.RXFFINT == 0);      //wait for transmission to complete
            ret = SpiaRegs.SPIRXBUF;                        //grab received char for return
            SpiaRegs.SPIFFRX.bit.RXFFINTCLR = 1;            //clear flag
            SPITX(TXdata[i+j] >> 16);                       //send Low byte
            while (SpiaRegs.SPIFFRX.bit.RXFFINT == 0);      //wait for transmission to complete
            ret = SpiaRegs.SPIRXBUF;                        //grab received char for return
            SpiaRegs.SPIFFRX.bit.RXFFINTCLR = 1;            //clear flag
            SPITX(TXdata[i+j] >> 8);                        //send high byte
            while (SpiaRegs.SPIFFRX.bit.RXFFINT == 0);      //wait for transmission to complete
            ret = SpiaRegs.SPIRXBUF;                        //grab received char for return
            SpiaRegs.SPIFFRX.bit.RXFFINTCLR = 1;            //clear flag
            SPITX(TXdata[i+j]);                             //send Low byte
            while (SpiaRegs.SPIFFRX.bit.RXFFINT == 0);      //wait for transmission to complete
            ret = SpiaRegs.SPIRXBUF;                        //grab received char for return
            SpiaRegs.SPIFFRX.bit.RXFFINTCLR = 1;            //clear flag

        }
        Nordic_CSN_high;                                //deselct Nodic on SPI
        Nordic_CE_high;                                 // pulse CE high for 10uS t start packet transmit
        DELAY_US(15);
        while (!NordicIRQ && !Nordic_Fail_Flg)
        {
            Nordic_Fail_Counter++;
            if (Nordic_Fail_Counter == 2001) Nordic_Fail_Flg = 1;
        }

        DELAY_US(300);
        if ((NordicRReg(STATUS) & 0x10) || Nordic_Fail_Flg)
        {
            NordicTX(0xE1);                             //Flush TX Buffer
            Nordic_Fail_Flg = 0;
            Nordic_Fail_Counter = 0;
            k = (PACKET_BYTES/32)+1;
        }
        NordicWReg(STATUS, 0x70);                       //Write Status Register to clear all flags
        j += 8;                                         //increment index (counting 8 longs per transmit)
    }
    DELAY_US(3000);
    Nordic_CE_low;
}
