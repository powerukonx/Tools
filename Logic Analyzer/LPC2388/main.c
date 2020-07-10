/* =============================================================================

                  ██╗   ██╗██╗  ██╗ ██████╗ ███╗   ██╗██╗  ██╗
                  ██║   ██║██║ ██╔╝██╔═══██╗████╗  ██║╚██╗██╔╝
                  ██║   ██║█████╔╝ ██║   ██║██╔██╗ ██║ ╚███╔╝
                  ██║   ██║██╔═██╗ ██║   ██║██║╚██╗██║ ██╔██╗
                  ╚██████╔╝██║  ██╗╚██████╔╝██║ ╚████║██╔╝ ██╗
                   ╚═════╝ ╚═╝  ╚═╝ ╚═════╝ ╚═╝  ╚═══╝╚═╝  ╚═╝

  File name:    main.c
  Date:         23 07 2017
  Author:       Power.
  Description:  Logic analyzer (LPC2388 part) - Body file.

============================================================================= */

/* =============================================================================
                                 DEBUG Section
============================================================================= */
#pragma ARM

/* =============================================================================
                                 Include Files
============================================================================= */
#include <LPC23xx.H>
#include "getdata.h"


/* =============================================================================
                          Private defines and typedefs
============================================================================= */
#define SAMPLING_DEPTH1     (1<<0)
#define SAMPLING_DEPTH2     (1<<1)
#define SAMPLING_DEPTH4     (1<<2)
#define SAMPLING_DEPTH8     (1<<3)
#define SAMPLING_DEPTH16    (1<<4)
#define SAMPLING_DEPTH32    (1<<5)
#define SAMPLING_DEPTH64    (1<<6)
#define SAMPLING_DEPTH128   (1<<7)
#define SAMPLING_DEPTH256   (1<<8)
#define SAMPLING_DEPTH512   (1<<9)
#define SAMPLING_DEPTH1024  (1<<10)
#define SAMPLING_DEPTH2048  (1<<11)

#define TRIGGER_FALL        (0)
#define TRIGGER_RISE        (1)

#define TRUE                (1)
#define FALSE               (0)

/* =============================================================================
                        Private constants and variables
============================================================================= */
__align(4) int i32_Datas[8193];
int i32_GetDatasOptions[4];

/* =============================================================================
                        Private function declarations
============================================================================= */
static void v_Senddata (void);

/* =============================================================================
                               Public functions
============================================================================= */

/*==============================================================================
Function    : main.

Describe    : Main entry function.

Parameters  : None.

Returns     : None.
==============================================================================*/
int main (void)
{
  /* Locals variables declaration. */
  unsigned char byChar;
  
  /* Select ports IO function. */
  PINSEL0 = 0xC5500A55U;    /* P0B0 as RD1                  */
                            /* P0B1 as TD1                  */
                            /* P0B2 as TXD0                 */
                            /* P0B3 as RXD0                 */
                            /* P0B4 as RD2                  */
                            /* P0B5 as TD2                  */
                            /* P0B6 as GPIO                 */
                            /* P0B7 as GPIO                 */
                            /* P0B8 as GPIO                 */
                            /* P0B9 as GPIO                 */
                            /* P0B10 as TXD2                */
                            /* P0B11 as RXD2                */
                            /* P0B12 as USB_PPWR2           */
                            /* P0B13 as USB_UP8LED2         */
                            /* P0B14 as GPIO                */
                            /* P0B15 as SCK                 */
  PINMODE0= 0xAAAAAAAAU;    /* Neither pull up or pull down */
  PINSEL1 = 0x417D6ABCU;    /* P0B16 as GPIO                */
                            /* P0B17 as MISO                */
                            /* P0B18 as MOSI                */
                            /* P0B19 as MCICLK              */
                            /* P0B20 as MCICMD              */
                            /* P0B21 as MCIPWR              */
                            /* P0B22 as MCIDAT0             */
                            /* P0B23 as AD0B0               */
                            /* P0B24 as AD0B1               */
                            /* P0B25 as TXD3                */
                            /* P0B26 as RXD3                */
                            /* P0B27 as SDA0                */
                            /* P0B28 as SCL0                */
                            /* P0B29 as GPIO                */
                            /* P0B30 as GPIO                */
                            /* P0B31 as USB_D+2             */
  PINSEL2 = 0x50150105U;    /* P1B0 as ENET_TXD0            */
                            /* P1B1 as ENET_TXD1            */
                            /* P1B4 as ENET_TX_EN           */
                            /* P1B8 as ENET_CRS             */
                            /* P1B9 as ENET_RXD0            */
                            /* P1B10 as ENET_RXD1           */
                            /* P1B14 as ENET_RX_ER          */
                            /* P1B15 as ENET_REF_CLK        */
  PINSEL3 = 0x5003C305U;    /* P1B16 as ENET_MDC            */
                            /* P1B17 as ENET_MDIO           */
                            /* P1B18 as GPIO                */
                            /* P1B19 as GPIO                */
                            /* P1B20 as SCK0                */
                            /* P1B21 as GPIO                */
                            /* P1B22 as GPIO                */
                            /* P1B23 as MISO0               */
                            /* P1B24 as MOSI0               */
                            /* P1B25 as GPIO                */
                            /* P1B26 as GPIO                */
                            /* P1B27 as GPIO                */
                            /* P1B28 as GPIO                */
                            /* P1B29 as GPIO                */
                            /* P1B30 as USB_PWRD2           */
                            /* P1B31 as USB_OVRCR2          */
  PINSEL4 = 0x0A800000U;    /* P2B0 as GPIO                 */
                            /* P2B1 as GPIO                 */
                            /* P2B2 as GPIO                 */
                            /* P2B3 as GPIO                 */
                            /* P2B4 as GPIO                 */
                            /* P2B5 as GPIO                 */
                            /* P2B6 as GPIO                 */
                            /* P2B7 as GPIO                 */
                            /* P2B8 as GPIO                 */
                            /* P2B9 as GPIO                 */
                            /* P2B10 as GPIO                */
                            /* P2B11 as MCIDAT1             */
                            /* P2B12 as MCIDAT2             */
                            /* P2B13 as MCIDAT3             */
  PINSEL6 = 0x00000000U;    /* P3B0 as GPIO                 */
                            /* P3B1 as GPIO                 */
                            /* P3B2 as GPIO                 */
                            /* P3B3 as GPIO                 */
                            /* P3B4 as GPIO                 */
                            /* P3B5 as GPIO                 */
                            /* P3B6 as GPIO                 */
                            /* P3B7 as GPIO                 */
  PINSEL7 = 0x003F0000U;    /* P3B23 as GPIO                */
                            /* P3B24 as PWM1B1              */
                            /* P3B25 as PWM2B2              */
                            /* P3B26 as PWM3B3              */
  PINSEL8 = 0x00000000U;    /* P4B0 as GPIO                 */
                            /* P4B1 as GPIO                 */
                            /* P4B2 as GPIO                 */
                            /* P4B3 as GPIO                 */
                            /* P4B4 as GPIO                 */
                            /* P4B5 as GPIO                 */
                            /* P4B6 as GPIO                 */
                            /* P4B7 as GPIO                 */
                            /* P4B8 as GPIO                 */
                            /* P4B9 as GPIO                 */
                            /* P4B10 as GPIO                */
                            /* P4B11 as GPIO                */
                            /* P4B12 as GPIO                */
                            /* P4B13 as GPIO                */
                            /* P4B14 as GPIO                */
                            /* P4B15 as GPIO                */
  PINSEL9 = 0x00000000U;    /* P4B24 as GPIO                */
                            /* P4B25 as GPIO                */
                            /* P4B28 as GPIO                */
                            /* P4B29 as GPIO                */
                            /* P4B30 as GPIO                */
                            /* P4B31 as GPIO                */
  PINSEL10= 0x00000000U;    /* P4B3 , ETM disabled          */

  /* Neither pull up or pull down. */
  PINMODE1 = 0xAAAAAAAAU;
  PINMODE2 = 0xAAAAAAAAU;
  PINMODE3 = 0xAAAAAAAAU;
  PINMODE4 = 0xAAAAAAAAU;
  PINMODE6 = 0xAAAAAAAAU;
  PINMODE7 = 0xAAAAAAAAU;
  PINMODE8 = 0xAAAAAAAAU;
  PINMODE9 = 0xAAAAAAAAU;
  
  /* Enable FastIO for port 0 and 1. */
  SCS |= 0x00000001U;

  /* Set I/O direction. */
  FIO0DIR = 0x60010000U;    /* B30 - B29 as output          */
  FIO1DIR = 0x3E600000U;    /* B22 - B21 as output          */
                            /* B19 - B22 as output          */
  FIO2DIR = 0x00000000U; 
  FIO3DIR = 0x00800080U;    /* P3B7 as Output               */
                            /* P3B23 as Output              */
  FIO4DIR = 0xA10000FFU;    /* B7 - B0 as Output            */
                            /* B15 - B8 as Input            */
                            /* B24 as Output                */
                            /* B25 as Input                 */
                            /* B28 as Input                 */
                            /* B29 as Output                */
                            /* B31 as Output                */

  FIO4MASK = ~(  (1<<29)
               | (1U<<4)
               | (1U<<5)
               | (1U<<6)
               | (1<<28)
               | (1<<25)
               | (0xFF<<8) );//0x5E0000FFU;

  /* Clear all outputs. */
  FIO0PIN = 0x0000000000;
  FIO1PIN = 0x0000000000;
  FIO2PIN = 0x0000000000;
  FIO3PIN = 0x0000000000;
  FIO4PIN = 0x0000000000;
  
  /* Set port 4 bit 29. */
  FIO4SET = (1U<<29);

  /* UART0 CLK. */
  PCLKSEL0 &= 0xFFFFFF3FU;
  PCLKSEL0 |= 0x00000040U;  /* CCLK_DIV_BY_1                  */

  /* Enable UART 0. */
  PCONP |= (1U<<3);         /* Enable UART0                   */

  /* Configure UART 0. */
  U0LCR  |= (1U<<7);        /* Access to latch divisor        */
  U0DLM = 0x00;             /* Configure to 115200            */
  U0DLL = 0x11;
  U0FDR = 0xAD;
  U0LCR = 0x03;             /* 8 bits, 1 stop, no parity      */
  U0FCR = 0x07;             /* FIFO enable ,trig all one char */

  /* Power on led. */
  FIO4SET = (1U<<4);

  /* Initialize default configuration, depth = 8192, trig fall edge of probe 8. */
  i32_GetDatasOptions[0] = (1<<13);  //  8192 DWORD
  i32_GetDatasOptions[1] = (1<<15);  //   PROBE 15
  i32_GetDatasOptions[2] = TRIGGER_FALL;
  i32_GetDatasOptions[3] = 0;

  /* Infinite loop. */
  while (1)
  {    
    /* Wait first char. */
    while (!(U0LSR&0x01));
    
    /* Read char. */
    byChar = U0RBR;
    
    /* Check incoming data. */
    switch (byChar)
    {
      case 71:  /* Go for sampling. */
      
        /* Sample data. */
        v_Get24Datas (i32_Datas, i32_GetDatasOptions);
        
        /* Send data. */
        v_Senddata ();      
        break;
      
      case 91:  /* Configuration. */
        
        /* Wait second char. */
        while (!(U0LSR&0x01));
      
        /* Read char. */
        byChar = U0RBR;

        /* Save trigger probe. */
        i32_GetDatasOptions[1] = ( 1<<(byChar & 0x1f));
        
        /* Save trigger edge. */
        i32_GetDatasOptions[2] = (byChar>>5) & 0x01;

        /* Wait second char. */
        while (!(U0LSR&0x01));
    
        /* Read char. */
        byChar = U0RBR;

        /* Save sampling depth. */
        i32_GetDatasOptions[0] =  (1<<(byChar&0x0F));
        
        /* Wait third char. */
        while (!(U0LSR&0x01));
    
        /* Read char. */
        byChar = U0RBR;
        
        /* Save LSB delay capture. */
        i32_GetDatasOptions[3] =  byChar;        

        /* Wait fourth char. */
        while (!(U0LSR&0x01));
    
        /* Read char. */
        byChar = U0RBR;
        
        /* Save MSB delay capture. */
        i32_GetDatasOptions[3] =  i32_GetDatasOptions[3] | (byChar<<8);        
        
        break;
      
      default:
        break;
    }
  }
  
  return 0;
}


/* =============================================================================
                              Private functions
============================================================================= */

/*==============================================================================
Function    : v_Senddata.

Describe    : Send data to UART.

Parameters  : None.

Returns     : None.
==============================================================================*/
static void v_Senddata (void)
{  
  /* Locals variables declaration. */
  int i32_Temp;
  int i32_Index = 0;
  int i32_NbrCar;
  
  /* led on. */
  FIO4SET = (1U<<6);
      
  /* Send data. */
  do
  {
    i32_Temp = i32_Datas[i32_Index];
    i32_NbrCar = 4;
    do
    {
      if ((U0LSR & (1<<6))) /* If empty */
      {
        U0THR = i32_Temp&0xff;
        i32_Temp>>=8;
        i32_NbrCar--;
      }
    }
    while (i32_NbrCar > 0);
      
    /* Next word. */
    i32_Index++;
  }
  while (i32_Index < i32_GetDatasOptions[0]);
    
  /* clear FIFO for next time. */
  U0FCR |= (1U<<2);

  /* vSenddata led off. */
  FIO4CLR = (1U<<6);    
}

