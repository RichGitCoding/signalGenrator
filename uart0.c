// UART0 Library
// Jason Losh

//-----------------------------------------------------------------------------
// Hardware Target
//-----------------------------------------------------------------------------

// Target Platform: EK-TM4C123GXL
// Target uC:       TM4C123GH6PM
// System Clock:    -

// Hardware configuration:
// UART Interface:
//   U0TX (PA1) and U0RX (PA0) are connected to the 2nd controller
//   The USB on the 2nd controller enumerates to an ICDI interface and a virtual COM port

//-----------------------------------------------------------------------------
// Device includes, defines, and assembler directives
//-----------------------------------------------------------------------------

#include <stdint.h>
#include <stdbool.h>
#include "tm4c123gh6pm.h"
#include "uart0.h"
#include <string.h>

// PortA masks
#define UART_TX_MASK 2
#define UART_RX_MASK 1

//-----------------------------------------------------------------------------
// Global variables
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Subroutines
//-----------------------------------------------------------------------------

// Initialize UART0
void initUart0()
{
    // Enable clocks
    SYSCTL_RCGCUART_R |= SYSCTL_RCGCUART_R0;
    SYSCTL_RCGCGPIO_R |= SYSCTL_RCGCGPIO_R0;
    _delay_cycles(3);

    // Configure UART0 pins
    GPIO_PORTA_DIR_R |= UART_TX_MASK;                   // enable output on UART0 TX pin
    GPIO_PORTA_DIR_R &= ~UART_RX_MASK;                   // enable input on UART0 RX pin
    GPIO_PORTA_DR2R_R |= UART_TX_MASK;                  // set drive strength to 2mA (not needed since default configuration -- for clarity)
    GPIO_PORTA_DEN_R |= UART_TX_MASK | UART_RX_MASK;    // enable digital on UART0 pins
    GPIO_PORTA_AFSEL_R |= UART_TX_MASK | UART_RX_MASK;  // use peripheral to drive PA0, PA1
    GPIO_PORTA_PCTL_R &= ~(GPIO_PCTL_PA1_M | GPIO_PCTL_PA0_M); // clear bits 0-7
    GPIO_PORTA_PCTL_R |= GPIO_PCTL_PA1_U0TX | GPIO_PCTL_PA0_U0RX;
                                                        // select UART0 to drive pins PA0 and PA1: default, added for clarity

    // Configure UART0 to 115200 baud, 8N1 format
    UART0_CTL_R = 0;                                    // turn-off UART0 to allow safe programming
    UART0_CC_R = UART_CC_CS_SYSCLK;                     // use system clock (40 MHz)
    UART0_IBRD_R = 21;                                  // r = 40 MHz / (Nx115.2kHz), set floor(r)=21, where N=16
    UART0_FBRD_R = 45;                                  // round(fract(r)*64)=45
    UART0_LCRH_R = UART_LCRH_WLEN_8 | UART_LCRH_FEN;    // configure for 8N1 w/ 16-level FIFO
    UART0_CTL_R = UART_CTL_TXE | UART_CTL_RXE | UART_CTL_UARTEN;
                                                        // enable TX, RX, and module
}

// Set baud rate as function of instruction cycle frequency
void setUart0BaudRate(uint32_t baudRate, uint32_t fcyc)
{
    uint32_t divisorTimes128 = (fcyc * 8) / baudRate;   // calculate divisor (r) in units of 1/128,
                                                        // where r = fcyc / 16 * baudRate
    UART0_IBRD_R = divisorTimes128 >> 7;                 // set integer value to floor(r)
    UART0_FBRD_R = ((divisorTimes128 + 1)) >> 1 & 63;    // set fractional value to round(fract(r)*64)
}

// Blocking function that writes a serial character when the UART buffer is not full
void putcUart0(char c)
{
    while (UART0_FR_R & UART_FR_TXFF);               // wait if uart0 tx fifo full
    UART0_DR_R = c;                                  // write character to fifo
}

// Blocking function that writes a string when the UART buffer is not full
void putsUart0(char* str)
{
    uint8_t i = 0;
    while (str[i] != '\0')
        putcUart0(str[i++]);
}

// Blocking function that returns with serial data once the buffer is not empty
char getcUart0()
{
    while (UART0_FR_R & UART_FR_RXFE);               // wait if uart0 rx fifo empty
    return UART0_DR_R & 0xFF;                        // get character from fifo
}

// Returns the status of the receive buffer
bool kbhitUart0()
{
    return !(UART0_FR_R & UART_FR_RXFE);
}
void getsUart0(USER_DATA* data)
{
    int count = 0;
    while(true)
   {

       char c = getcUart0();

       if((c == 8 || c == 127 ) && count > 0)
       {
           count--;
           continue;
       }
       if(c == 13)
       {
          data->buffer[count] = NULL;
          break;
       }

       if(c == 32 || c > 32)
       {
           data->buffer[count] = c;
           count++;
           if(count == MAX_CHARS )
           {
               data->buffer[count] = NULL;
               break;
           }
       }
   }

    putsUart0("\n\n");
    return;
}
void parseFields(USER_DATA* data)
{
    //3 tyoes - delimeter, alphabet, number
    char prev = 'd';//assume prev type is delimeter
    int count = 0;
    uint8_t i = 0;
    uint8_t j = 0;
    uint8_t size;
    data->fieldCount = 0;
    uint8_t y = 0;
    uint8_t z = 0;
    //get length of string
    for(y = 0; data->buffer[y] != '\0'; ++y)
    {
    }
   // putcUart0(y+48);
    size = y;

    char c;

    for(i = 0; i<size; i++)//while(data->buffer[i] != NULL)
    {
        if( j == 5)
        {
            break;
        }
        //determine if current position is number or letter or delimeter
        c = data->buffer[i];
        if( (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') )// || (c >= 65 && c <= 90)
        {
            if(prev != 'a')
            {
                prev = 'a';
                data->fieldPosition[j] = i;
                data->fieldType[j] = prev;
                j++;
                //putcUart0('a');
                data->fieldCount++; //= data->fieldCount+1;//(data->fieldCount+1) & 0xFF;
            }
            continue;


            //putcUart0(prev);
        }
        else if( ((c-'0') <= 9) && ((c-'0') >= 0) ) // ((c-'0') <= 9) || ((c-'0') >= 0)   (c <= 57) && (c >= 48)
        {
            if(prev != 'n')
            {
                prev = 'n';
                data->fieldPosition[j] = i;
                data->fieldType[j] = prev;
                j++;
                //putcUart0('n');
                data->fieldCount++; //= data->fieldCount+1;
            }

            continue;
        }
        else if( (c >= 32) || (c <= 64))
        {
            prev = 'd';
            data->buffer[i] = 0x00;
            continue;
            //putcUart0('z');
        }





    }
    //putsUart0("\n");
    //putsUart0(data->fieldType);
    //putcUart0(data->fieldCount+48);
    return;

}
char* getFieldString(USER_DATA* data, uint8_t fieldNumber)
{
    fieldNumber = fieldNumber -1;
    //putsUart0(&data->buffer[data->fieldPosition[fieldNumber]]);
    uint8_t offset = data->fieldPosition[fieldNumber];
    uint8_t endOffset = data->fieldPosition[fieldNumber+1];
    char string[MAX_CHARS+1];
    char* stringv;
    uint8_t i = 0;

    putsUart0("\n");

    for(i = offset; i<endOffset; i++)
    {
        if(data->buffer[i] == NULL)
        {
            break;
        }
        string[i] = data->buffer[i];
        putcUart0(string[i]);
    }

    stringv = string;

   // putsUart0(stringv);


    return *stringv;
}
int32_t getFieldInteger(USER_DATA* data, uint8_t fieldNumber)
{
    int32_t fieldInt = 0;

    if(data->fieldPosition[fieldNumber] == NULL )
    {
        return fieldInt;
    }

    if(data->fieldType[MAX_FIELDS] != 'n')
    {
        return fieldInt;

    }

    fieldInt = &(data->fieldPosition[fieldNumber]);

    return fieldInt;



}
bool isCommand(USER_DATA* data, const char strCommand[], uint8_t minArguments)
{
    //checks if the command in buffer is = strCommand
    uint8_t i = 0;
    uint8_t j = 0;
//    if (strCommand >= minArguments)
//    {
//        while(data->buffer[i] != NULL)
//        {
//            if(data->buffer[i] != strCommand[i])
//            {
//                return false;
//            }
//
//            i++;
//        }
//    }

    while(data->buffer[i] != NULL)
    {
        if(data->buffer[i] != strCommand[i])
        {
            return false;
        }

        i++;
    }


    uint8_t size = sizeof(data->buffer);
    uint8_t numOfArguments = 0;
    for(i = i; i<=size; i++)
    {
        if(data->buffer[i] != NULL)
        {
            numOfArguments++;
        }
    }

    if(numOfArguments<minArguments)
    {
        return false;
    }

    return true;


}
uint8_t getSize(USER_DATA *data)
{
    uint8_t y = data->fieldCount;

    return y;
}
