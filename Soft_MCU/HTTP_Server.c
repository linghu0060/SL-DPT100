/*------------------------------------------------------------------------------
 * MDK Middleware - Component ::Network
 * Copyright (c) 2004-2014 ARM Germany GmbH. All rights reserved.
 *------------------------------------------------------------------------------
 * Name:    HTTP_Server.c
 * Purpose: HTTP Server example
 *----------------------------------------------------------------------------*/

#include <assert.h>
#include <stdio.h>
#include "cmsis_os.h"                   /* CMSIS RTOS definitions             */
#include "rl_net.h"                     /* Network definitions                */
#include "rl_fs.h"                      /* FileSystem definitions             */

#include "stm32f4xx_hal.h"
#ifndef MINI_STM32F429
#include "Board_GLCD.h"
#include "GLCD_Config.h"
#endif
#include "Board_LED.h"
#include "Board_Buttons.h"
#include "Board_ADC.h"

#ifndef MINI_STM32F429
extern GLCD_FONT GLCD_Font_6x8;
extern GLCD_FONT GLCD_Font_16x24;
#endif

bool LEDrun;
bool LCDupdate;
char lcd_text[2][20+1];

static void BlinkLed (void const *arg);
static void Display (void const *arg);

osThreadDef(BlinkLed, osPriorityNormal, 1, 0);
osThreadDef(Display, osPriorityNormal, 1, 0);

/// Read analog inputs
uint16_t AD_in (uint32_t ch) {
  int32_t val = 0;

  if (ch == 0) {
    ADC_StartConversion();
    while (ADC_ConversionDone () < 0);
    val = ADC_GetValue();
  }
  return (val);
}

/// Read digital inputs
uint8_t get_button (void) {
  return (Buttons_GetState ());
}

/// IP address change notification
void dhcp_client_notify (uint32_t if_num,
                         dhcpClientOption opt, const uint8_t *val, uint32_t len) {
  if (opt == dhcpClientIPaddress) {
    // IP address has changed
    sprintf (lcd_text[0],"IP address:");
    sprintf (lcd_text[1],"%s", ip4_ntoa (val));
    LCDupdate = true;
  }
}

/*----------------------------------------------------------------------------
  Thread 'Display': LCD display handler
 *---------------------------------------------------------------------------*/
static void Display (void const *arg) {
#ifndef MINI_STM32F429
  char lcd_buf[20+1];

  GLCD_Initialize         ();
  GLCD_SetBackgroundColor (GLCD_COLOR_BLUE);
  GLCD_SetForegroundColor (GLCD_COLOR_WHITE);
  GLCD_ClearScreen        ();
  GLCD_SetFont            (&GLCD_Font_16x24);
  GLCD_DrawString         (0, 1*24, "       MDK-MW       ");
  GLCD_DrawString         (0, 2*24, "HTTP Server example ");

  sprintf (lcd_text[0], "");
  sprintf (lcd_text[1], "Waiting for DHCP");
  LCDupdate = true;

  while(1) {
    if (LCDupdate == true) {
      sprintf (lcd_buf, "%-20s", lcd_text[0]);
      GLCD_DrawString (0, 5*24, lcd_buf);
      sprintf (lcd_buf, "%-20s", lcd_text[1]);
      GLCD_DrawString (0, 6*24, lcd_buf);
      LCDupdate = false;
    }
    osDelay (250);
  }
#else
  while(1) {
    osDelay(250);
  }
#endif  
}

/*----------------------------------------------------------------------------
  Thread 'BlinkLed': Blink the LEDs on an eval board
 *---------------------------------------------------------------------------*/
static void BlinkLed (void const *arg) {
  const uint8_t led_val[16] = { 0x48,0x88,0x84,0x44,0x42,0x22,0x21,0x11,
                                0x12,0x0A,0x0C,0x14,0x18,0x28,0x30,0x50 };
  int cnt = 0;

  LEDrun = true;
  while(1) {
    // Every 100 ms
    if (LEDrun == true) {
      LED_SetOut (led_val[cnt]);
      if (++cnt >= sizeof(led_val)) {
        cnt = 0;
      }
    }
    osDelay (100);
  }
}



static void init_filesystem (void) {
  fsStatus stat;

  printf ("Initializing and mounting enabled drives...\n\n");

  /* Initialize and mount drive "F0" */
  stat = finit ("F0:");
  if (stat == fsOK) {
    stat = fmount ("F0:");
    if (stat == fsOK) {
        int64_t  sp = ffree("F0:");
        if( sp < 0 ) {
            sp = 0 -sp;
        }
        printf ("Drive F0 ready! free space: %lld\n", sp);
    }
    else if (stat == fsNoFileSystem) {
      /* Format the drive */
      printf ("Drive F0 not formatted!\n");
      fformat ("F0:", "");
    }
    else {
      printf ("Drive F0 mount failed with error code %d\n", stat);
    }
  }
  else {
    printf ("Drive F0 initialization failed!\n");
  }

  /* Initialize and mount drive "M0" */
  stat = finit ("M0:");
  if (stat == fsOK) {
    stat = fmount ("M0:");
    if (stat == fsOK) {
      printf ("Drive M0 ready!\n");
    }
    else if (stat == fsNoFileSystem) {
      /* Format the drive */
      printf ("Drive M0 not formatted!\n");
      fformat ("M0:", "");
    }
    else {
      printf ("Drive M0 mount failed with error code %d\n", stat);
    }
  }
  else {
    printf ("Drive M0 initialization failed!\n");
  }

  /* Initialize and mount drive "N0" */
  stat = finit ("N0:");
  if (stat == fsOK) {
    stat = fmount ("N0:");
    if (stat == fsOK) {
      printf ("Drive N0 ready!\n");
    }
    else if (stat == fsNoFileSystem) {
      /* Format the drive */
      printf ("Drive N0 not formatted!\n");
      fformat ("N0:", "");
    }
    else {
      printf ("Drive N0 mount failed with error code %d\n", stat);
    }
  }
  else {
    printf ("Drive N0 initialization failed!\n");
  }

  printf ("\nDone!\n");
}


/*----------------------------------------------------------------------------
  Main Thread 'main': Run Network
 *---------------------------------------------------------------------------*/

extern void netiod_init( void );
int main (void) {
  HAL_Init();
  LED_Initialize     ();
  LED_Uninitialize   ();
  Buttons_Initialize ();
  ADC_Initialize     ();
  init_filesystem    ();
  net_initialize     ();
    
//  osThreadCreate (osThread(BlinkLed), NULL);
//  osThreadCreate (osThread(Display), NULL);
  netiod_init();
    
//  osThreadSetPriority(osThreadGetId(), osPriorityIdle);
    
  while(1) {
    net_main ();
    osThreadYield ();
  }
}
