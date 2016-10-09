/**********************************************************************************************************/
/** @file     stdio_usart.c
*** @author   Linghu
*** @version  V1.0.0
*** @date     2015/7/29
*** @brief    stderr¡¢stdout¡¢stdin¡¢ttywrch in USART.
***********************************************************************************************************
*** @par Last Commit:
***      \$Author$ \n
***      \$Date$ \n
***      \$Rev$ \n
***      \$URL$ \n
***
*** @par Change Logs:
***      2015/7/29 -- Linghu -- the first version
***********************************************************************************************************/

#include    "Driver_USART.h"

/**********************************************************************************************************/
/** @addtogroup STDIO_USART
*** @{
*** @addtogroup STDIO_USART_Pravate
*** @{
*** @addtogroup                 STDIO_USART_Private_Config
*** @{
***********************************************************************************************************/

//-------- <<< Use Configuration Wizard in Context Menu >>> --------------------

// <h>STD USART Interface
//   <o>Connect to hardware via Driver_USART# <0-255>
//   <i>Select driver control block for USART interface
#define USART_DRV_NUM           1

//   <o>Baudrate
#define USART_BAUDRATE          115200
// </h>

/**********************************************************************************************************/
/** @}
*** @addtogroup                 STDIO_USART_Private_define
*** @{
***********************************************************************************************************/

#define _USART_Driver_(n)  Driver_USART##n
#define  USART_Driver_(n) _USART_Driver_(n)

extern ARM_DRIVER_USART  USART_Driver_(USART_DRV_NUM);
#define ptrUSART       (&USART_Driver_(USART_DRV_NUM))


/**********************************************************************************************************/
/** @}
*** @addtogroup                 STDIO_USART_Private_Functions
*** @{
***********************************************************************************************************/
/** @brief      Init STD In/Out stream interface.
*** 
*** @return     (0)Succeed, (Other)Failed.
***********************************************************************************************************/

int stdio_init(void)
{
    int32_t status;

    status = ptrUSART->Initialize(NULL);
    if( status != ARM_DRIVER_OK )  return( -1 );

    status = ptrUSART->PowerControl(ARM_POWER_FULL);
    if( status != ARM_DRIVER_OK )  return( -1 );

    status = ptrUSART->Control(   ARM_USART_MODE_ASYNCHRONOUS
                                | ARM_USART_DATA_BITS_8
                                | ARM_USART_PARITY_NONE
                                | ARM_USART_STOP_BITS_1
                                | ARM_USART_FLOW_CONTROL_NONE
                                , USART_BAUDRATE
                              );
    if( status != ARM_DRIVER_OK )  return( -1 );

    status = ptrUSART->Control(ARM_USART_CONTROL_TX, 1);
    if( status != ARM_DRIVER_OK )  return( -1 );

    status = ptrUSART->Control(ARM_USART_CONTROL_RX, 1);
    if( status != ARM_DRIVER_OK )  return( -1 );

    return( 0 );
}


/**********************************************************************************************************/
/** @brief      Get a character from stdin.
*** 
*** @return     The character from the input, or -1 on read error.
***********************************************************************************************************/

int stdin_getchar(void)
{
    uint8_t buf[1];

    if( ptrUSART->Receive(buf, 1) != ARM_DRIVER_OK ) {
        return( -1 );
    }
    while( ptrUSART->GetRxCount() != 1 );
    return( buf[0] );
}

/**********************************************************************************************************/
/** @brief      Put a character to the stdout.
*** 
*** @param[in]  ch  Character to output.
*** 
*** @return     The character written, or -1 on write error.
***********************************************************************************************************/

int stdout_putchar(int ch)
{
    uint8_t buf[1];

    buf[0] = ch;
    if( ptrUSART->Send(buf, 1) != ARM_DRIVER_OK ) {
        return( -1 );
    }
    while( ptrUSART->GetTxCount() != 1 );
    return( ch );
}

/**********************************************************************************************************/
/** @brief      Put a character to the stderr.
*** 
*** @param[in]  ch  Character to output.
*** 
*** @return     The character written, or -1 on write error.
***********************************************************************************************************/

int stderr_putchar(int ch)
{
    uint8_t buf[1];

    buf[0] = ch;
    if( ptrUSART->Send(buf, 1) != ARM_DRIVER_OK ) {
        return( -1 );
    }
    while( ptrUSART->GetTxCount() != 1 );
    return( ch );
}

/**********************************************************************************************************/
/** @brief      Put a character to the teletypewritter.
*** 
*** @param[in]  ch  Character to output.
***********************************************************************************************************/

void ttywrch(int ch)
{
    uint8_t buf[1];

    buf[0] = ch;
    if( ptrUSART->Send(buf, 1) != ARM_DRIVER_OK ) {
        return;
    }
    while( ptrUSART->GetTxCount() != 1 );
    return;
}


/*****************************  END OF FILE  **************************************************************/
/** @}
*** @}
*** @}
***********************************************************************************************************/

