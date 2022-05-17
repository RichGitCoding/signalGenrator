// Richard Robertson 
// Mohammed Suhail Hussainy Safder 

//-----------------------------------------------------------------------------
// Hardware Target
//-----------------------------------------------------------------------------

// Target Platform: EK-TM4C123GXL Evaluation Board
// Target uC:       TM4C123GH6PM
// System Clock:    40 MHz

//-----------------------------------------------------------------------------
// Device includes, defines, and assembler directives
//-----------------------------------------------------------------------------

#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "tm4c123gh6pm.h"
#include "clock.h"
#include "gpio.h"
#include "spi1.h"
#include "nvic.h"
#include "wait.h"
#include "uart0.h"
#include "adc0.h"
#include <math.h>
#include <float.h>


//suhail code
#define AIN2 PORTE,1 //channel A or 1
#define AIN0 PORTE,3 //channel B or 2


//for struct
#define MAX_CHARS 80
#define MAX_FIELDS 5


// Pins
#define CS PORTD,1
#define LDAC PORTE,2
#define LUT_SIZE 4096
uint16_t LUT1[LUT_SIZE];
uint16_t LUT2[LUT_SIZE];
int Lut1Counter=0;
int Lut2Counter = 0;
#define gainA ((0x3449-0x3BCC)/-5)
#define gainB ((0xB443-0xBBC5)/-5)
#define fs (40e6/488)
//#define fs 160000
#define ts (1/fs)
int cycles = 0;
int stopCycle = 0;
int cyclesReq=0;
int fieldType = 0;
uint16_t step;
uint16_t phaseG=0;
int cycles2=0;
int cycles3=0;

// Initialize Hardware
void initHw()
{
    // Initialize system clock to 40 MHz
    initSystemClockTo40Mhz();
    SYSCTL_RCGCTIMER_R |= SYSCTL_RCGCTIMER_R1;


    enablePort(PORTD);
    enablePort(PORTE);
//    enablePort(PORTF);
    _delay_cycles(3);


//    //configure ains
    selectPinAnalogInput(PORTE,3);
    selectPinAnalogInput(PORTE,1);
    setPinAuxFunction(PORTE,1, GPIO_PCTL_PE1_AIN2); //Aux for channel A
    setPinAuxFunction(PORTE,3, GPIO_PCTL_PE3_AIN0); //Aux for channel B



    selectPinPushPullOutput(CS);

    ///
//    setPinValue(CS, 1);
//    waitMicrosecond(100);
//    setPinValue(CS, 0);
//    waitMicrosecond(100);
//    setPinValue(CS, 1);
//    ///

    initSpi1(USE_SSI_FSS);
    setSpi1BaudRate(20e6, 40e6);
    setSpi1Mode(0, 0);
    //setPinAuxFunction(PORTC,6,GPIO_PCTL_PC6_WT1CCP0);

    //enable LDAC, set high
    selectPinPushPullOutput(LDAC);


    // Configure Timer 1 as the time base
    TIMER1_CTL_R &= ~TIMER_CTL_TAEN;                 // turn-off timer before reconfiguring
    TIMER1_CFG_R = TIMER_CFG_32_BIT_TIMER;           // configure as 32-bit timer (A+B)
    TIMER1_TAMR_R = TIMER_TAMR_TAMR_PERIOD;          // configure for periodic mode (count down)
    TIMER1_TAILR_R = 4000;                       // set load value to 40e6 for 1 Hz interrupt rate
    TIMER1_IMR_R = TIMER_IMR_TATOIM;                 // turn-on interrupts
    NVIC_EN0_R |= 1 << (INT_TIMER1A-16);             // turn-on interrupt 37 (TIMER1A)
    TIMER1_CTL_R |= TIMER_CTL_TAEN;                  // turn-on timer

}



//-----------------------------------------------------------------------------
// Main
//-----------------------------------------------------------------------------
void sendDac(uint32_t data)
{

    setPinValue(CS, 0);
    writeSpi1Data(data);
    setPinValue(CS, 1);
    setPinValue(LDAC,0);
    _delay_cycles(4);
    setPinValue(LDAC,1);

}

void getPhase(float frequency)
{
//    step = (float) frequency/((float)fs/(float)LUT_SIZE);
//    uint16_t integer = (step<<16);
//    step=step-floor(step);
 //   uint32_t period = 40e6/488;
 //   uint32_t reff = period/LUT_SIZE;
  //  uint32_t a = frequency/reff;
 //   step = (a<<16);
    //step = (step>>16);
   // deltaPhase = 4294967296 + (frequency/100000);
    //uint32_t delta = (((frequency/2.048)/fs)*4294967296);


//    return delta;
}
void Timer()//interrupt used to plot the points
{
//    if(Lut1Counter >= 4095)
//    {
//        Lut1Counter=0;
//        cycles++;
//    }
//
//    if(Lut2Counter >= 4095)
//    {
//        Lut2Counter=0;
//        cycles++;
//    }



    phaseG = phaseG+step;

    if((stopCycle == 0))
    {
        sendDac(0x3000 | LUT1[Lut1Counter]); //send daca the sin
        sendDac(0xB000 | LUT2[Lut1Counter]);
    }

//    sendDac(0x3000 | LUT1[Lut1Counter]); //send daca the sin
    Lut1Counter++;
   // Lut2Counter++;
   // setPinValue(LDAC,0);
    TIMER1_ICR_R = TIMER_ICR_TATOCINT;


}
uint16_t calculateV(float v,int output)//return r value to be loaded into lut to send to dac
{
    if(output == 1)
    {
        uint16_t r;
        float dacV = (v-5)/-5;
        r = (dacV * 4096) / 2.048;
        r+=0x5A;//offset daca
      //  r =1986 - 380.9628 * v;
        return r;

    }
    if(output == 2)
    {
        uint16_t r;
        float dacV = (v-5)/-5;
        r = (dacV * 4096) / 2.048;
        r+=0x48; //offset dacb
        //r =1989 - 380.9628 * v;

        return r;
    }

}


void sendDc(float v, int output)
{
    if(output==1)
    {
        uint16_t r;
//        float dacV = (v-5)/-5;
//        r = (dacV * 4096) / 2.048;
//        r+=0x4;
        r =1986 - 380.9628 * v;
                                                        //r = (r + gainA);//1 s for user input
        sendDac(0x3000 | r);
    }
    if(output==2)
    {
        uint16_t r;
//        float dacV = (v-5)/-5;
//        r = (dacV * 4096) / 2.048;
//        r+=0x48;
        r =1989 - 380.9628 * v;
                                                       //r = (r + gainA);//1 s for user input
        sendDac(0xB000 | r);

    }
}
void sawtoothGen(float gain, int output, float frequency, float offset)
{
    //r for 2.5 and r for -2.5
    float x, y;//x is the sin value that is calculated
    int i;//index for for loop
    uint32_t pc = (frequency * pow(2,32)) * fs;//phase change calculation
    uint16_t pcmask = (pc>>16);//gives the top 12 bits, can use 16 bits too
 //   getFrequency(frequency);
//    uint32_t deltaPhase = getPhase(frequency);
//    uint16_t phase = (deltaPhase>>16);


    if(output == 1)
    {
        for(i = 0; i < LUT_SIZE; i++)
        {
            y = -(((float)i)/((float)LUT_SIZE)) *  (M_PI);
            x = offset + (((2*gain)/M_PI) * atan((1.0/(float)tan(y))));

            LUT1[i] = calculateV(x, 1); //calculates voltage, returns r that correlates to that voltage on dac

        }

    }
    if(output == 2)
    {
        for(i = 0; i < LUT_SIZE; i++)
        {
            y = (((float)i)/((float)LUT_SIZE)*2.0) *  (M_PI);
            x = offset + (gain * atan((1.0/tan(y))));

            LUT2[i] = calculateV(x, 1); //calculates voltage, returns r that correlates to that voltage on dac

        }

    }



}

void squareGen(float gain, int output, float frequency, float offset)
{
    float x, y;//x is the sin value that is calculated
    int i;//index for for loop
    uint32_t pc = (frequency * pow(2,32)) * fs;//phase change calculation
    uint16_t pcmask = (pc>>16);//gives the top 12 bits, can use 16 bits too
//    getFrequency(frequency);
//    uint32_t deltaPhase = getPhase(frequency);
//    uint16_t phase = (deltaPhase>>16);

    if(output == 1)
    {
        for(i = 0; i < LUT_SIZE; i++)
        {
            y = ((float)i/(float)LUT_SIZE) * (2.0 * M_PI);
            x = offset + (gain * (float)sin(y));

            if ((x-offset) >= 0.0)
            {
                x = 1;
                LUT1[i] = calculateV(x, 1); //calculates voltage, returns r that correlates to that voltage on dac


            }
            else if ((x-offset) < 0.0)
            {
                x= -1;
                LUT1[i] = calculateV(x, 1); //calculates voltage, returns r that correlates to that voltage on dac


            }

        }
    }
        if(output == 2)
        {
            for(i = 0; i < LUT_SIZE; i++)

            {
                y = (((float)i/(float)LUT_SIZE)) * (2 * M_PI);
                x = offset + (gain * sin(y));

                if(x > 0)
                {
                    x = 1;
                }
                else
                {
                    x=-1;
                }

                LUT2[i] = calculateV(x, 1); //calculates voltage, returns r that correlates to that voltage on dac

            }


        }


}

void triangleGen(float gain, int output, float frequency, float offset)
{
    float x,y;
    int i;
    uint32_t pc = (frequency * pow(2,32)) * fs;//phase change calculation
    uint16_t pcmask = (pc>>16);//gives the top 12 bits, can use 16 bits too
//    getFrequency(frequency);
//    uint32_t deltaPhase = getPhase(frequency);
//    uint16_t phase = (deltaPhase>>16);


    if(output == 1)
    {
        for(i = 0; i < LUT_SIZE; i++)
        {
            y = ((float)i/(float)LUT_SIZE) * (2 * M_PI);
            x = offset + (gain * (2.0/M_PI) * asin(sin(y)));

            LUT1[i] = calculateV(x, 1); //calculates voltage, returns r that correlates to that voltage on dac

        }

    }
    if(output == 2)
        {
            for(i = 0; i < LUT_SIZE; i++)
            {
                y = ((float)i/(float)LUT_SIZE) * (2 * M_PI);
                x = offset + (gain * (2.0/M_PI) * asin(sin(y)));

                LUT2[i] = calculateV(x, 1); //calculates voltage, returns r that correlates to that voltage on dac

            }

        }
}



void sinGen(float gain, int output, float frequency, float offset) //v is voltage
{
    //r for 2.5 and r for -2.5
    float x, y;//x is the sin value that is calculated
    int i;//index for for loop
    getPhase(frequency);

//    uint32_t deltaPhase = getPhase(frequency);
//    uint16_t phase = (deltaPhase>>16);


    if(output == 1)
    {
        for(i = 0; i < LUT_SIZE; i++)
        {
            y = (((float)i)/(float)LUT_SIZE) * (2 * M_PI);
            x = offset + (gain * sin(y));

            LUT1[i] = calculateV(x, 1); //calculates voltage, returns r that correlates to that voltage on dac

        }

    }
    if(output == 2)
    {
        for(i = 0; i<LUT_SIZE;i++)
        {
            y = (((float)i)/(float)LUT_SIZE) * (2 * M_PI);
            x = offset + (gain * sin(y));

            LUT2[i] = calculateV(x, 2); //calculates voltage, returns r that correlates to that voltage on dac            LUT2[i] = x;

        }
    }


}


int main(void)
{
    stopCycle=1;
    USER_DATA data;
    initHw();
    initUart0();
    initAdc0Ss3();
    initAdc0Ss2();
    setUart0BaudRate(115200, 40e6);
//    sendDc(1.0,1);
//    sendDac(0x3640);
    //for last generated signals
    float lastFreq=3;//last frequency entered
    float lastGain=3; //last gain entered
    int lastGen=0;//last signal entered. 0 = sin, 1=square,2=sawtooth,3=triangle
    float lastOfs = 0;





    // for adc code channel A
    uint16_t raw=0;
    char str[100];
    setAdc0Ss3Mux(2);
    setAdc0Ss3Log2AverageCount(2);

    // for adc code channel B
    uint16_t raw2=0;
    setAdc0Ss2Mux(0);
    setAdc0Ss2Log2AverageCount(2);



    //turn both DACs off
//    sendDac(0x3811); //0x3782 was 0, now 3811
//    sendDac(0xB80B);//0xB782 was 0, now 3811
//    sinGen(1.0,1,1.0,1);
//    testDacV(1.0);
//    generate a sine wave here first


    while(true)
    {
        putsUart0("Enter Command:\n\r");
        getsUart0(&data);
        putsUart0("\n");
        putsUart0(data.buffer);
        putsUart0("\n");

        int isMinus=0;
        if(data.buffer[5] == '-')
        {
            isMinus=1;
        }



        parseFields(&data);
        putsUart0("\n");
        putsUart0("\n");




        //enter code



        int output = 0;
        uint8_t voltageReq;
        uint8_t voltageDec;

        uint16_t freq=1;
        float ofs=0;
        float gain = 0.0;



        float voltage=0;
        uint16_t l;
        char c;
        if((isCommand(&data, "cycles", 1))) //cycles N, cycles continuous
        {
            stopCycle = 0;

            //get the number of cycles, put in loop until # is met
            if(data.fieldType[1] == 'n')
            {
                cyclesReq = (data.buffer[data.fieldPosition[1]]) - '0';
                fieldType = 1;
            }
            int j;
            if((cycles >= cyclesReq) && (cycles !=0) )
            {
                for(j=0;j<4096;j++)
                {
                    LUT1[j]=0;
                    LUT2[j]=0;
                }

            }


             stopCycle = 0;

//             if(lastGen == 0)//sin
//             {
//                 sinGen(lastGain,1,lastFreq,lastOfs);
//                 stopCycle = 0;
//                 sinGen(lastGain,2,lastFreq,lastOfs);
//                 putsUart0("Generating last Sin wave.\n");
//
//             }
//             if(lastGen == 1)//square
//             {
//                 squareGen(lastGain,1,lastFreq,lastOfs);
//                 stopCycle = 0;
//                 squareGen(lastGain,2,lastFreq,lastOfs);
//                 putsUart0("Generating last Square wave.\n");
//             }
//             if(lastGen == 2)//sawtooth
//             {
//                 sawtoothGen(lastGain,1,lastFreq,lastOfs);
//                 stopCycle = 0;
//                 sawtoothGen(lastGain,2,lastFreq,lastOfs);
//                 putsUart0("Generating last Sawtooth wave.\n");
//             }
//             if(lastGen == 3)//triangle
//             {
//                 triangleGen(lastGain,1,lastFreq,lastOfs);
//                 stopCycle = 0;
//                 triangleGen(lastGain,2,lastFreq,lastOfs);
//                 putsUart0("Generating last Triangle wave.\n");
//             }
//
//
//
//
//            //sendV(voltage,output);
            putsUart0("Sending cycles to given output\n");

            //putsUart0("U ugly");
            //putsUart0(((data.buffer[data.fieldPosition[1]]) - '0'));
        }
        if((isCommand(&data, "dc", 2))) //dc output, voltage
        {
            stopCycle = 1;
            uint8_t volt[3];
            float volt1;

            //determine which output
            output = (data.buffer[data.fieldPosition[1]]) - '0';

            //get first whole numer of voltage
            voltageReq = (data.buffer[data.fieldPosition[2]]) - '0';//the whole part
            voltageDec = (data.buffer[data.fieldPosition[3]]) - '0';//the partial part
            float vr = voltageReq;
            float vd = voltageDec;
            //both values are floats now
    //        if(vd==1.0)
    //        {
    //            putsUart0("Hello");
    //        }




            voltage = vr + (vd/10);//the actual voltage, 2.2, 3.5 etc

            if(isMinus == 1 )
            {
                voltage = voltage * (-1.0);
            }



            sendDc(voltage,output);
            putsUart0("Sending voltage to given output\n");
            //putsUart0("U ugly");
            //putsUart0(((data.buffer[data.fieldPosition[1]]) - '0'));
        }

        else if((isCommand(&data, "sine", 4))) //sin out, freq, amplitude(gain), offset
        {
            if((cycles >= cyclesReq) && (cycles !=0) )
            {
                int j;
                for(j=0;j<4096;j++)
                {
                    LUT1[j]=0;
                    LUT2[j]=0;
                }

            }

            stopCycle = 0;

            //determine which output
            output = (data.buffer[data.fieldPosition[1]]) - '0';


            //get the offset
            ofs = (data.buffer[data.fieldPosition[4]])-'0';

            //get the amplitude/gain
            char g[10];
            int gainCount = 0;

            //get distance between field 3 and field 4 to get gain
            int dis = data.fieldPosition[4] - data.fieldPosition[3];
            uint8_t gainArr[8];
            for(gainCount = 0; gainCount < dis; gainCount++)
            {
                gainArr[gainCount] = (data.buffer[data.fieldPosition[3]+gainCount]);
            }

            float gainFinal = (float)atof(gainArr);

            //get entered frequency
            char f[5];
            int freqCount=0;
            dis = data.fieldPosition[3] - data.fieldPosition[2];
            uint8_t freqArr[5];
            for(freqCount = 0; freqCount < dis; freqCount++)
            {
                freqArr[freqCount] = (data.buffer[data.fieldPosition[2]+freqCount]);
            }

            float freqFinal = (float)atof(freqArr);

            sinGen(gainFinal,output,freqFinal,ofs);
            putsUart0("Generating Sin wave.\n");

            //set last variables
            lastGen=0;
            lastFreq=freqFinal;
            lastGain=gainFinal;



        }
        else if((isCommand(&data, "square", 4))) //square out, freq, amplitude(gain), offset
        {
            stopCycle = 0;

            //determine which output
            output = (data.buffer[data.fieldPosition[1]]) - '0';


            //get the offset
            ofs = (data.buffer[data.fieldPosition[4]])-'0';

            //get the amplitude/gain
            char g[10];
            int gainCount = 0;

            //get distance between field 3 and field 4 to get gain
            int dis = data.fieldPosition[4] - data.fieldPosition[3];
            uint8_t gainArr[8];
            for(gainCount = 0; gainCount < dis; gainCount++)
            {
                gainArr[gainCount] = (data.buffer[data.fieldPosition[3]+gainCount]);
            }

            float gainFinal = (float)atof(gainArr);

            //get entered frequency
            char f[5];
            int freqCount=0;
            dis = data.fieldPosition[3] - data.fieldPosition[2];
            uint8_t freqArr[5];
            for(freqCount = 0; freqCount < dis; freqCount++)
            {
                freqArr[freqCount] = (data.buffer[data.fieldPosition[2]+freqCount]);
            }

            float freqFinal = (float)atof(freqArr);

            squareGen(gainFinal,output,freqFinal,ofs);

            putsUart0("Generating Square wave.\n");

            //set last variables
            lastGen=1;
            lastFreq=freqFinal;
            lastGain=gainFinal;

        }
        else if((isCommand(&data, "sawtooth", 4))) //sawtooth out, freq, amplitude(gain), offset
        {
            stopCycle = 0;

            //determine which output
            output = (data.buffer[data.fieldPosition[1]]) - '0';


            //get the offset
            ofs = (data.buffer[data.fieldPosition[4]])-'0';

            //get the amplitude/gain
            char g[10];
            int gainCount = 0;

            //get distance between field 3 and field 4 to get gain
            int dis = data.fieldPosition[4] - data.fieldPosition[3];
            uint8_t gainArr[8];
            for(gainCount = 0; gainCount < dis; gainCount++)
            {
                gainArr[gainCount] = (data.buffer[data.fieldPosition[3]+gainCount]);
            }

            float gainFinal = (float)atof(gainArr);

            //get entered frequency
            char f[5];
            int freqCount=0;
            dis = data.fieldPosition[3] - data.fieldPosition[2];
            uint8_t freqArr[5];
            for(freqCount = 0; freqCount < dis; freqCount++)
            {
                freqArr[freqCount] = (data.buffer[data.fieldPosition[2]+freqCount]);
            }

            float freqFinal = (float)atof(freqArr);

            sawtoothGen(gainFinal,output,freqFinal,ofs);
            putsUart0("Generating Sawtooth wave.\n");

            //set last variables
            lastGen=2;
            lastFreq=freqFinal;
            lastGain=gainFinal;



        }
        else if((isCommand(&data, "triangle", 4))) //triangle out, freq, amplitude(gain), offset
        {
            stopCycle = 0;
            //determine which output
            output = (data.buffer[data.fieldPosition[1]]) - '0';


            //get the offset
            ofs = (data.buffer[data.fieldPosition[4]])-'0';

            //get the amplitude/gain
            char g[10];
            int gainCount = 0;

            //get distance between field 3 and field 4 to get gain
            int dis = data.fieldPosition[4] - data.fieldPosition[3];
            uint8_t gainArr[8];
            for(gainCount = 0; gainCount < dis; gainCount++)
            {
                gainArr[gainCount] = (data.buffer[data.fieldPosition[3]+gainCount]);
            }

            float gainFinal = (float)atof(gainArr);

            //get entered frequency
            char f[5];
            int freqCount=0;
            dis = data.fieldPosition[3] - data.fieldPosition[2];
            uint8_t freqArr[5];
            for(freqCount = 0; freqCount < dis; freqCount++)
            {
                freqArr[freqCount] = (data.buffer[data.fieldPosition[2]+freqCount]);
            }

            float freqFinal = (float)atof(freqArr);

            triangleGen(gainFinal,output,freqFinal,ofs);
            putsUart0("Generating Triangle wave.\n");

            //set last variables
            lastGen=3;
            lastFreq=freqFinal;
            lastGain=gainFinal;


        }

        else if((isCommand(&data, "run", 0))) //run last command on both outputs
        {
            stopCycle = 0;

            if(lastGen == 0)//sin
            {
                sinGen(lastGain,1,lastFreq,lastOfs);
                stopCycle = 0;
                sinGen(lastGain,2,lastFreq,lastOfs);
                putsUart0("Generating last Sin wave.\n");

            }
            if(lastGen == 1)//square
            {
                squareGen(lastGain,1,lastFreq,lastOfs);
                stopCycle = 0;
                squareGen(lastGain,2,lastFreq,lastOfs);
                putsUart0("Generating last Square wave.\n");
            }
            if(lastGen == 2)//sawtooth
            {
                sawtoothGen(lastGain,1,lastFreq,lastOfs);
                stopCycle = 0;
                sawtoothGen(lastGain,2,lastFreq,lastOfs);
                putsUart0("Generating last Sawtooth wave.\n");
            }
            if(lastGen == 3)//triangle
            {
                triangleGen(lastGain,1,lastFreq,lastOfs);
                stopCycle = 0;
                triangleGen(lastGain,2,lastFreq,lastOfs);
                putsUart0("Generating last Triangle wave.\n");
            }





        }
        else if((isCommand(&data, "stop", 0)))
        {

            //send dac1 and dac2 the voltage of 0
            stopCycle = 1;
            sendDac(0x3811);
            sendDac(0xB80B);
            putsUart0("Stopping signal, clearing LUT.\n");

            //fill the luts with 0
            int j;
            for(j=0;j<4096;j++)
            {
                LUT1[j]=0;
                LUT2[j]=0;
            }



        }
        else if((isCommand(&data, "reset", 0)))
                {

                    //send dac1 and dac2 the voltage of 0
                    stopCycle = 1;
                    sendDac(0x3811);
                    sendDac(0xB80B);
                    putsUart0("Reseting DAC, clearing LUT....\n");

                    //fill the luts with 0
                    int j;
                    for(j=0;j<4096;j++)
                    {
                        LUT1[j]=0;
                        LUT2[j]=0;
                    }
                    NVIC_APINT_R=NVIC_APINT_VECTKEY|NVIC_APINT_SYSRESETREQ;
                    putsUart0("Device Reset!\n");

                }
        else if((isCommand(&data, "voltage", 1)))//voltage, IN = IN being the input selected
        {
            putsUart0("Getting voltage...\n");
            output = (data.buffer[data.fieldPosition[1]]) - '0';

            if( output == 1)
                        {
                         raw = readAdc0Ss3();
                         float vDisplay=0.0;
                         //vDisplay = (3.3V * RAW_value / LUT_SIZE);
                         vDisplay = (3.3*raw/4095.0)-0.05;
                         sprintf(str,"Raw ADC at Channel A:       %4u\n",raw);
                         putsUart0(str);
                         sprintf(str,"Actual Voltage:       %4.1f\n",vDisplay);
                         putsUart0(str);
                         }

            if( output == 2)
                        {
                         raw2 = readAdc0Ss2();
                         float vDisplay2=0.0;
                         //vDisplay = (3.3V * RAW_value / LUT_SIZE);
                         vDisplay2 = (3.3*raw2/4095.0)+0.05;
                         sprintf(str,"Raw ADC at Channel B:       %4u\n",raw2);
                         putsUart0(str);
                         sprintf(str,"Actual Voltage:       %4.1f\n",vDisplay2);
                         putsUart0(str);
                         }

        }


        else if((isCommand(&data, "Level", 1)))// LEVEL, 1 for ON 0 for OFF being the input selected
        {
            output = (data.buffer[data.fieldPosition[1]]) - '0';
            if( output == 1) //leveling on
            {
                sendDc(2.0,1); //2.0 volts in channel A
                float diff=0.0;
                raw = readAdc0Ss3();
                raw2 = readAdc0Ss2();
                waitMicrosecond(500);
                if(raw2>raw)
                    diff=raw2-raw;
                else
                    diff=raw-raw2;
                diff=diff*(3.3/4095);
                sprintf(str,"Actual Voltage:       %4.1f\n",(3.3*raw/4096.0)+0.05);
                putsUart0(str);
                sprintf(str,"Leveling difference:       %4u\n",diff);
                putsUart0(str);
            }

            if (output == 0)
            {
                sendDc(0.0,1);
                sendDc(0.0,2);
            }
        }

        else if((isCommand(&data, "gain", 2))) //gain, freq 1, fre2
        {

            float i;
            //get entered frequency

            char f[5];
            int freqCount=0;
            char freq1Arr[5];
            char freq2Arr[5];
            int dis = data.fieldPosition[3] - data.fieldPosition[2];
            uint8_t freqArr[5];
            for(freqCount = 0; freqCount < dis; freqCount++)
            {
                freq1Arr[freqCount] = (data.buffer[data.fieldPosition[2]+freqCount]);
            }
            float freq1 = (float)atof(freqArr);
            int size = sizeof(data.buffer);
            int l;
            int lastfCount=0;
            for(l = data.fieldPosition[3]; l<sizeof(data.buffer); l++)
            {
                lastfCount++;
            }

            dis = lastfCount;




            for(freqCount = 0; freqCount < dis; freqCount++)
            {
                freq2Arr[freqCount] = (data.buffer[data.fieldPosition[3]+freqCount]);
            }



            float freq2 = (float)atof(freq2Arr);


            for(i = freq1; i < freq2; i++)//start, end
            {
                sinGen(1,i,0,0);
            }

         }
        //clear buffer and other elements


    }

    while(true);

}

