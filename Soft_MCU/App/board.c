/**********************************************************************************************************/
/** @file     board.c
*** @author   Linghu
*** @version  V1.0.0
*** @date     2015/7/23
*** @brief    Board Support Package.
***********************************************************************************************************
*** @par Last Commit:
***      \$Author$ \n
***      \$Date$ \n
***      \$Rev$ \n
***      \$URL$ \n
***
*** @par Change Logs:
***      2015/7/23 -- Linghu -- the first version
***********************************************************************************************************/

#include    <assert.h>
#include    <stdio.h>
#include    "cmsis_os.h"                /* CMSIS RTOS definitions             */
#include    "stm32f4xx.h"
#include    "board.h"


/**********************************************************************************************************/
/** @addtogroup BOARD
*** @{
*** @addtogroup BOARD_Pravate
*** @{
*** @addtogroup                 BOARD_Private_Constants
*** @{
***********************************************************************************************************/



/**********************************************************************************************************/
/** @}
*** @addtogroup                 BOARD_Private_Macros
*** @{
***********************************************************************************************************/



/**********************************************************************************************************/
/** @}
*** @addtogroup                 BOARD_Private_Types
*** @{
***********************************************************************************************************/



/**********************************************************************************************************/
/** @}
*** @addtogroup                 BOARD_Private_Variables
*** @{
***********************************************************************************************************/



/**********************************************************************************************************/
/** @}
*** @addtogroup                 BOARD_Private_Prototypes
*** @{
***********************************************************************************************************/



/**********************************************************************************************************/
/** @}
*** @addtogroup                 BOARD_Private_Functions
*** @{
***********************************************************************************************************/
/** @brief      Board Initialize
***********************************************************************************************************/

void board_init(void)
{
    extern int stdio_init(void);


    MODIFY_REG(RCC->CFGR,      (0x3u << 21) | (0x7u << 24)      /* MCO1 selected HSE oscillator clock       */
                        ,      (0x2u << 21) | (0x0u << 24));
    MODIFY_REG(RCC->AHB1ENR,   0                                /* Enable GPIOA clock                       */
                           ,   RCC_AHB1ENR_GPIOAEN);
    MODIFY_REG(GPIOA->MODER,   (0x3u << 2*(8))                  /* PA8 to MCO1                              */
                           ,   (0x2u << 2*(8)));
    MODIFY_REG(GPIOA->AFR[1],  (0xFu << 4*(8-8))
                            ,  (0x0u << 4*(8-8)));
    MODIFY_REG(GPIOA->OTYPER,  (0x1u << 1*(8))                  /* PA8 Output push-pull                    */
                            ,  (0x0u << 1*(8)));
    MODIFY_REG(GPIOA->OSPEEDR, (0x3u << 2*(8))                  /* PA8 Output High speed                   */
                             , (0x3u << 2*(8)));
    MODIFY_REG(GPIOA->PUPDR,   (0x3u << 2*(8))                  /* PA8 No pull-up, pull-down               */
                           ,   (0x0u << 2*(8)));

  //stdio_init();       // Init StdIO of Compiler retarget_io
    HAL_Init();         // Init STM32 HAL drivers
    stdio_init();       // Init StdIO of Compiler retarget_io

    printf("\r\n[Board] Syetme start...\r\n");
    printf("[Board] System Clock Configed. System Core Clock: %luHz\r\n", (unsigned long)HAL_RCC_GetHCLKFreq());
}


/**********************************************************************************************************/
/** @brief      System Clock Configuration.
***********************************************************************************************************/

static void SystemClock_Config(void)
{
    RCC_OscInitTypeDef  RCC_OscInitStruct;
    RCC_ClkInitTypeDef  RCC_ClkInitStruct;

    /* Enable Power Control clock */
//  __PWR_CLK_ENABLE();

    /* The voltage scaling allows optimizing the power consumption when the
       device is clocked below the maximum system frequency (see datasheet). */
//  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

    /* Enable HSE Oscillator and activate PLL with HSE as source */
    RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
    RCC_OscInitStruct.HSEState       = RCC_HSE_ON;
    RCC_OscInitStruct.PLL.PLLState   = RCC_PLL_ON;
    RCC_OscInitStruct.PLL.PLLSource  = RCC_PLLSOURCE_HSE;
    RCC_OscInitStruct.PLL.PLLM       = 25;
    RCC_OscInitStruct.PLL.PLLN       = 336;
    RCC_OscInitStruct.PLL.PLLP       = RCC_PLLP_DIV2;
    RCC_OscInitStruct.PLL.PLLQ       = 7;
    HAL_RCC_OscConfig(&RCC_OscInitStruct);

    /* Select PLL as system clock source and configure the HCLK, PCLK1 and PCLK2 clocks dividers */
    RCC_ClkInitStruct.ClockType      = RCC_CLOCKTYPE_SYSCLK | RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
    RCC_ClkInitStruct.SYSCLKSource   = RCC_SYSCLKSOURCE_PLLCLK;
    RCC_ClkInitStruct.AHBCLKDivider  = RCC_SYSCLK_DIV1;
    RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV4;
    RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV2;
    HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_5);

    /* Update SystemCoreClock variable  */
    SystemCoreClockUpdate();
}


/**********************************************************************************************************/
/** brief      Redefine HAL function.
***********************************************************************************************************/

void HAL_MspInit(void)
{
    SystemClock_Config();
}

void HAL_MspDeInit(void)
{
}

#ifdef __RTX
HAL_StatusTypeDef HAL_InitTick(uint32_t TickPriority)
{
    (void)TickPriority;
    return HAL_OK;
}

void HAL_IncTick(void)
{
}

uint32_t HAL_GetTick(void)
{
    extern uint32_t os_time;
    return os_time;
}

void HAL_Delay(__IO uint32_t Delay)
{
    osStatus  HAL_Delay_Status = osDelay(Delay);
    assert(HAL_Delay_Status == osEventTimeout);  (void)HAL_Delay_Status;
}

void HAL_SuspendTick(void)
{
    assert(0);
}

void HAL_ResumeTick(void)
{
    assert(0);
}
#endif

#ifdef USE_FULL_ASSERT
void assert_failed(uint8_t *file, uint32_t line)
{
    void _ttywrs(const char *s);
    void _ttywrd(int32_t n);

    _ttywrs("\r\n*** assertion failed at file: ");
    _ttywrs((const char*)file);
    _ttywrs(", line: ");
    _ttywrd((int32_t)line);
    _ttywrs("\r\n");

    abort();
}

__inline void _ttywrs(const char *s)
{
    extern void _ttywrch(int ch);

    for(;  s[0] != '\0';  s++) {
        _ttywrch(s[0]);
    }
}

__inline void _ttywrd(int32_t n)
{
    extern void _ttywrch(int ch);
    int32_t     x, y;
    int         b;

    for(b = 0, y = 1000000000;  y != 0;  y /= 10)
    {
        x = n / y;  n %= y;
        if( x != 0 )  b = 1;
        if( b != 0 )  _ttywrch(x + '0');
    }
}
#endif


/*****************************  END OF FILE  **************************************************************/
/** @}
*** @}
*** @}
***********************************************************************************************************/

