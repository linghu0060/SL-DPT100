/**********************************************************************************************************/
/** @file     stm32f4_iap_flash.c
*** @author   Linghu
*** @version  V1.0.0
*** @date     2015/7/20
*** @brief    File System Flash Device Driver for STM32F4XX IAP.
***********************************************************************************************************
*** @par Last Commit:
***      \$Author$ \n
***      \$Date$ \n
***      \$Rev$ \n
***      \$URL$ \n
***
*** @par Change Logs:
***      2015/7/20 -- Linghu -- the first version
***********************************************************************************************************/

#include    <string.h>
#include    "Driver_Flash.h"
#include    "stm32f4xx.h"
#include    "stm32f4_iap_flash.h"


/**********************************************************************************************************/
/** @addtogroup IAP_FLASH
*** @{
*** @addtogroup IAP_FLASH_Pravate
*** @{
*** @addtogroup                 IAP_FLASH_Private_Constants
*** @{
***********************************************************************************************************/

#ifndef ARM_FLASH_DRV_VERSION
#define ARM_FLASH_DRV_VERSION  ARM_DRIVER_VERSION_MAJOR_MINOR(1,00) /* driver version       */
#endif

#ifndef DRIVER_FLASH_NUM
#define DRIVER_FLASH_NUM        0                                   /* driver number        */
#endif

/* Base address of the Flash sectors Bank 1 */
#define ADDR_FLASH_SEC_0     ((uint32_t)0x08000000)     /* Base @ of Sector 0, 16 Kbytes    */
#define ADDR_FLASH_SEC_1     ((uint32_t)0x08004000)     /* Base @ of Sector 1, 16 Kbytes    */
#define ADDR_FLASH_SEC_2     ((uint32_t)0x08008000)     /* Base @ of Sector 2, 16 Kbytes    */
#define ADDR_FLASH_SEC_3     ((uint32_t)0x0800C000)     /* Base @ of Sector 3, 16 Kbytes    */
#define ADDR_FLASH_SEC_4     ((uint32_t)0x08010000)     /* Base @ of Sector 4, 64 Kbytes    */
#define ADDR_FLASH_SEC_5     ((uint32_t)0x08020000)     /* Base @ of Sector 5, 128 Kbytes   */
#define ADDR_FLASH_SEC_6     ((uint32_t)0x08040000)     /* Base @ of Sector 6, 128 Kbytes   */
#define ADDR_FLASH_SEC_7     ((uint32_t)0x08060000)     /* Base @ of Sector 7, 128 Kbytes   */
#define ADDR_FLASH_SEC_8     ((uint32_t)0x08080000)     /* Base @ of Sector 8, 128 Kbytes   */
#define ADDR_FLASH_SEC_9     ((uint32_t)0x080A0000)     /* Base @ of Sector 9, 128 Kbytes   */
#define ADDR_FLASH_SEC_10    ((uint32_t)0x080C0000)     /* Base @ of Sector 10, 128 Kbytes  */
#define ADDR_FLASH_SEC_11    ((uint32_t)0x080E0000)     /* Base @ of Sector 11, 128 Kbytes  */

/* Base address of the Flash sectors Bank 2 */
#define ADDR_FLASH_SEC_12    ((uint32_t)0x08100000)     /* Base @ of Sector 0, 16 Kbytes    */
#define ADDR_FLASH_SEC_13    ((uint32_t)0x08104000)     /* Base @ of Sector 1, 16 Kbytes    */
#define ADDR_FLASH_SEC_14    ((uint32_t)0x08108000)     /* Base @ of Sector 2, 16 Kbytes    */
#define ADDR_FLASH_SEC_15    ((uint32_t)0x0810C000)     /* Base @ of Sector 3, 16 Kbytes    */
#define ADDR_FLASH_SEC_16    ((uint32_t)0x08110000)     /* Base @ of Sector 4, 64 Kbytes    */
#define ADDR_FLASH_SEC_17    ((uint32_t)0x08120000)     /* Base @ of Sector 5, 128 Kbytes   */
#define ADDR_FLASH_SEC_18    ((uint32_t)0x08140000)     /* Base @ of Sector 6, 128 Kbytes   */
#define ADDR_FLASH_SEC_19    ((uint32_t)0x08160000)     /* Base @ of Sector 7, 128 Kbytes   */
#define ADDR_FLASH_SEC_20    ((uint32_t)0x08180000)     /* Base @ of Sector 8, 128 Kbytes   */
#define ADDR_FLASH_SEC_21    ((uint32_t)0x081A0000)     /* Base @ of Sector 9, 128 Kbytes   */
#define ADDR_FLASH_SEC_22    ((uint32_t)0x081C0000)     /* Base @ of Sector 10, 128 Kbytes  */
#define ADDR_FLASH_SEC_23    ((uint32_t)0x081E0000)     /* Base @ of Sector 11, 128 Kbytes  */


/**********************************************************************************************************/
/** @}
*** @addtogroup                 IAP_FLASH_Private_Macros
*** @{
***********************************************************************************************************/


/**********************************************************************************************************/
/** @}
*** @addtogroup                 IAP_FLASH_Private_Variables
*** @{
***********************************************************************************************************/

static ARM_FLASH_SECTOR  FLASH_SECTOR_INFO[FLASH_SECTOR_COUNT] = {  /* Sector Information   */
    FLASH_SECTORS
};

static ARM_FLASH_INFO  FlashInfo = {                                /* Flash Information    */
    FLASH_SECTOR_INFO,
    FLASH_SECTOR_COUNT,
    FLASH_SECTOR_SIZE,
    FLASH_PAGE_SIZE,
    FLASH_PROGRAM_UNIT,
    FLASH_ERASED_VALUE
};

static ARM_FLASH_STATUS     FlashStatus;                            /* Flash Status         */

static const ARM_DRIVER_VERSION  DriverVersion = {                  /* Driver Version       */
    ARM_FLASH_API_VERSION,
    ARM_FLASH_DRV_VERSION
};

static const ARM_FLASH_CAPABILITIES DriverCapabilities = {          /* Driver Capabilities  */
    0,    /* event_ready */
    2,    /* data_width = 0:8-bit, 1:16-bit, 2:32-bit */
    1     /* erase_chip */
};

// static const uint8_t  g_FlashEFS[FLASH_EFS_SIZE-16]              /* IAP FLASH region     */
//                    __attribute__((section(".ARM.__at_0x08040000"), deprecated, zero_init));


/**********************************************************************************************************/
/** @}
*** @addtogroup                 IAP_FLASH_Private_Functions
*** @{
***********************************************************************************************************/
/** @brief      Gets the sector of a given address.
***
*** @param[in]  Address     given address.
***
*** @return     The sector of a given address.
***********************************************************************************************************/

static uint32_t  GetSector( uint32_t Address )
{
    uint32_t  sector;

#if defined(STM32F405xx) || defined(STM32F415xx) || defined(STM32F407xx) || defined(STM32F417xx)
    if(      (Address < ADDR_FLASH_SEC_1)  && (Address >= ADDR_FLASH_SEC_0)  )  sector = FLASH_SECTOR_0;
    else if( (Address < ADDR_FLASH_SEC_2)  && (Address >= ADDR_FLASH_SEC_1)  )  sector = FLASH_SECTOR_1;
    else if( (Address < ADDR_FLASH_SEC_3)  && (Address >= ADDR_FLASH_SEC_2)  )  sector = FLASH_SECTOR_2;
    else if( (Address < ADDR_FLASH_SEC_4)  && (Address >= ADDR_FLASH_SEC_3)  )  sector = FLASH_SECTOR_3;
    else if( (Address < ADDR_FLASH_SEC_5)  && (Address >= ADDR_FLASH_SEC_4)  )  sector = FLASH_SECTOR_4;
    else if( (Address < ADDR_FLASH_SEC_6)  && (Address >= ADDR_FLASH_SEC_5)  )  sector = FLASH_SECTOR_5;
    else if( (Address < ADDR_FLASH_SEC_7)  && (Address >= ADDR_FLASH_SEC_6)  )  sector = FLASH_SECTOR_6;
    else if( (Address < ADDR_FLASH_SEC_8)  && (Address >= ADDR_FLASH_SEC_7)  )  sector = FLASH_SECTOR_7;
    else if( (Address < ADDR_FLASH_SEC_9)  && (Address >= ADDR_FLASH_SEC_8)  )  sector = FLASH_SECTOR_8;
    else if( (Address < ADDR_FLASH_SEC_10) && (Address >= ADDR_FLASH_SEC_9)  )  sector = FLASH_SECTOR_9;
    else if( (Address < ADDR_FLASH_SEC_11) && (Address >= ADDR_FLASH_SEC_10) )  sector = FLASH_SECTOR_10;
    else   /*(Address < ADDR_FLASH_SEC_12) && (Address >= ADDR_FLASH_SEC_11)*/  sector = FLASH_SECTOR_11;
#elif defined(STM32F427xx) || defined(STM32F437xx) || defined(STM32F429xx)|| defined(STM32F439xx)
    if(      (Address < ADDR_FLASH_SEC_1)  && (Address >= ADDR_FLASH_SEC_0)  )  sector = FLASH_SECTOR_0;
    else if( (Address < ADDR_FLASH_SEC_2)  && (Address >= ADDR_FLASH_SEC_1)  )  sector = FLASH_SECTOR_1;
    else if( (Address < ADDR_FLASH_SEC_3)  && (Address >= ADDR_FLASH_SEC_2)  )  sector = FLASH_SECTOR_2;
    else if( (Address < ADDR_FLASH_SEC_4)  && (Address >= ADDR_FLASH_SEC_3)  )  sector = FLASH_SECTOR_3;
    else if( (Address < ADDR_FLASH_SEC_5)  && (Address >= ADDR_FLASH_SEC_4)  )  sector = FLASH_SECTOR_4;
    else if( (Address < ADDR_FLASH_SEC_6)  && (Address >= ADDR_FLASH_SEC_5)  )  sector = FLASH_SECTOR_5;
    else if( (Address < ADDR_FLASH_SEC_7)  && (Address >= ADDR_FLASH_SEC_6)  )  sector = FLASH_SECTOR_6;
    else if( (Address < ADDR_FLASH_SEC_8)  && (Address >= ADDR_FLASH_SEC_7)  )  sector = FLASH_SECTOR_7;
    else if( (Address < ADDR_FLASH_SEC_9)  && (Address >= ADDR_FLASH_SEC_8)  )  sector = FLASH_SECTOR_8;
    else if( (Address < ADDR_FLASH_SEC_10) && (Address >= ADDR_FLASH_SEC_9)  )  sector = FLASH_SECTOR_9;
    else if( (Address < ADDR_FLASH_SEC_11) && (Address >= ADDR_FLASH_SEC_10) )  sector = FLASH_SECTOR_10;
    else if( (Address < ADDR_FLASH_SEC_12) && (Address >= ADDR_FLASH_SEC_11) )  sector = FLASH_SECTOR_11;
    else if( (Address < ADDR_FLASH_SEC_13) && (Address >= ADDR_FLASH_SEC_12) )  sector = FLASH_SECTOR_12;
    else if( (Address < ADDR_FLASH_SEC_14) && (Address >= ADDR_FLASH_SEC_13) )  sector = FLASH_SECTOR_13;
    else if( (Address < ADDR_FLASH_SEC_15) && (Address >= ADDR_FLASH_SEC_14) )  sector = FLASH_SECTOR_14;
    else if( (Address < ADDR_FLASH_SEC_16) && (Address >= ADDR_FLASH_SEC_15) )  sector = FLASH_SECTOR_15;
    else if( (Address < ADDR_FLASH_SEC_17) && (Address >= ADDR_FLASH_SEC_16) )  sector = FLASH_SECTOR_16;
    else if( (Address < ADDR_FLASH_SEC_18) && (Address >= ADDR_FLASH_SEC_17) )  sector = FLASH_SECTOR_17;
    else if( (Address < ADDR_FLASH_SEC_19) && (Address >= ADDR_FLASH_SEC_18) )  sector = FLASH_SECTOR_18;
    else if( (Address < ADDR_FLASH_SEC_20) && (Address >= ADDR_FLASH_SEC_19) )  sector = FLASH_SECTOR_19;
    else if( (Address < ADDR_FLASH_SEC_21) && (Address >= ADDR_FLASH_SEC_20) )  sector = FLASH_SECTOR_20;
    else if( (Address < ADDR_FLASH_SEC_22) && (Address >= ADDR_FLASH_SEC_21) )  sector = FLASH_SECTOR_21;
    else if( (Address < ADDR_FLASH_SEC_23) && (Address >= ADDR_FLASH_SEC_22) )  sector = FLASH_SECTOR_22;
    else   /*(Address < ADDR_FLASH_SEC_24) && (Address >= ADDR_FLASH_SEC_23)*/  sector = FLASH_SECTOR_23;
#else
    #error  "EFS on STM32F4xx IAP Flash does not support!!!"
#endif
    return sector;
}



/**********************************************************************************************************/
/** @brief      Get driver version.
***
*** @return     ARM_DRIVER_VERSION.
***********************************************************************************************************/

static ARM_DRIVER_VERSION  GetVersion( void )
{
    return DriverVersion;
}


/**********************************************************************************************************/
/** @brief      Get driver capabilities.
***
*** @return     ARM_FLASH_CAPABILITIES.
***********************************************************************************************************/

static ARM_FLASH_CAPABILITIES  GetCapabilities( void )
{
    return DriverCapabilities;
}


/**********************************************************************************************************/
/** @brief      Initialize the Flash Interface.
***
*** @param[in]  cb_event    Pointer to ARM_Flash_SignalEvent.
***
*** @return     execution_status.
***********************************************************************************************************/

static int32_t  Initialize( ARM_Flash_SignalEvent_t cb_event )
{
    FlashStatus.busy  = 0;
    FlashStatus.error = 0;
    return ARM_DRIVER_OK;
}


/**********************************************************************************************************/
/** @brief      De-initialize the Flash Interface.
***
*** @return     execution_status.
***********************************************************************************************************/

static int32_t  Uninitialize( void )
{
    return ARM_DRIVER_OK;
}


/**********************************************************************************************************/
/** @brief      Control the Flash interface power.
***
*** @param[in]  state   Power state.
***
*** @return     execution_status.
***********************************************************************************************************/

static int32_t  PowerControl( ARM_POWER_STATE state )
{
    return ARM_DRIVER_OK;
}


/**********************************************************************************************************/
/** @brief      Read data from Flash.
***
*** @param[in]  addr  Data address.
*** @param[in]  data  Pointer to a buffer storing the data read from Flash.
*** @param[in]  cnt   Number of data items to read.
***
*** @return     number of data items read or execution_status.
***********************************************************************************************************/

static int32_t  ReadData(uint32_t addr, void *data, uint32_t cnt)
{
    if( (addr >= FLASH_EFS_SIZE) || (addr & 3) || (data == NULL) ) {
        return ARM_DRIVER_ERROR_PARAMETER;
    }
    if( FlashStatus.busy ) {
        return ARM_DRIVER_ERROR_BUSY;
    }
    FlashStatus.error = 0;

    memcpy(data, (void*)(FLASH_EFS_ADDR + addr), cnt * 4);

    return cnt;
}


/**********************************************************************************************************/
/** @brief      Program data to Flash.
***
*** @param[in]  addr    Data address.
*** @param[out] data    Pointer to a buffer containing the data to be programmed to Flash.
*** @param[in]  cnt     Number of data items to program.
***
*** @return     number of data items programmed or execution_status
***********************************************************************************************************/

static int32_t  ProgramData( uint32_t addr, const void *data, uint32_t cnt )
{
    uint32_t    n;

    if( (addr >= FLASH_EFS_SIZE) || (addr & 3) || (data == NULL)  ) {
        return ARM_DRIVER_ERROR_PARAMETER;
    }
    if( FlashStatus.busy ) {
        return ARM_DRIVER_ERROR_BUSY;
    }
    FlashStatus.busy  = 1;

    if( HAL_OK == HAL_FLASH_Unlock() )
    {
        FlashStatus.error = 0;
        for(n = 0;  n < cnt;  n++)
        {
            if( HAL_OK != HAL_FLASH_Program(  TYPEPROGRAM_WORD
                                            , FLASH_EFS_ADDR + addr + n * 4
                                            , ((uint32_t*)data)[n]
                                           ) )
            {
                FlashStatus.error = 1;
                break;
            }
        }
        if( HAL_OK != HAL_FLASH_Lock() ) {
            FlashStatus.error = 1;
        }
    }
    else {
        FlashStatus.error = 1;
    }

    FlashStatus.busy  = 0;
    return( FlashStatus.error ? ARM_DRIVER_ERROR : (int32_t)cnt );
}


/**********************************************************************************************************/
/** @brief      Erase Flash Sector.
***
*** @param[in]  addr    Sector address.
***
*** @return     execution_status.
***********************************************************************************************************/

static int32_t  EraseSector( uint32_t addr )
{
    static FLASH_EraseInitTypeDef   EraseInitStruct;
    uint32_t                        SectorError;

    if( FlashStatus.busy ) {
        return ARM_DRIVER_ERROR_BUSY;
    }
    FlashStatus.busy  = 1;

    if( HAL_OK == HAL_FLASH_Unlock() )
    {
        FlashStatus.error = 0;

        EraseInitStruct.TypeErase    = TYPEERASE_SECTORS;   /* Fill EraseInit structure */
        EraseInitStruct.VoltageRange = VOLTAGE_RANGE_3;
        EraseInitStruct.Sector       = GetSector(FLASH_EFS_ADDR + addr);
        EraseInitStruct.NbSectors    = 1;

        __HAL_FLASH_INSTRUCTION_CACHE_DISABLE();
        __HAL_FLASH_DATA_CACHE_DISABLE();
        if( HAL_OK != HAL_FLASHEx_Erase(&EraseInitStruct, &SectorError) ) {
            FlashStatus.error = 1;
        }
      //__HAL_FLASH_INSTRUCTION_CACHE_RESET();
        FLASH->ACR |=  FLASH_ACR_ICRST;
        FLASH->ACR &= ~FLASH_ACR_ICRST;
      //__HAL_FLASH_DATA_CACHE_RESET();
        FLASH->ACR |=  FLASH_ACR_DCRST;
        FLASH->ACR &= ~FLASH_ACR_DCRST;
        __HAL_FLASH_INSTRUCTION_CACHE_ENABLE();
        __HAL_FLASH_DATA_CACHE_ENABLE();

        if( HAL_OK != HAL_FLASH_Lock() ) {
            FlashStatus.error = 1;
        }
    }
    else {
        FlashStatus.error = 1;
    }

    FlashStatus.busy  = 0;
    return( FlashStatus.error ? ARM_DRIVER_ERROR : ARM_DRIVER_OK );
}


/**********************************************************************************************************/
/** @brief      Erase complete Flash.
***
*** @return     execution_status.
***********************************************************************************************************/

static int32_t  EraseChip( void )
{
    static FLASH_EraseInitTypeDef   EraseInitStruct;
    uint32_t                        SectorError;

    if( FlashStatus.busy ) {
        return ARM_DRIVER_ERROR_BUSY;
    }
    FlashStatus.busy  = 1;

    if( HAL_OK == HAL_FLASH_Unlock() )
    {
        FlashStatus.error = 0;

        EraseInitStruct.TypeErase    = TYPEERASE_SECTORS;   /* Fill EraseInit structure */
        EraseInitStruct.VoltageRange = VOLTAGE_RANGE_3;
        EraseInitStruct.Sector       = GetSector(FLASH_EFS_ADDR + 0);
        EraseInitStruct.NbSectors    = FLASH_SECTOR_COUNT;

        __HAL_FLASH_INSTRUCTION_CACHE_DISABLE();
        __HAL_FLASH_DATA_CACHE_DISABLE();
        if( HAL_OK != HAL_FLASHEx_Erase(&EraseInitStruct, &SectorError) ) {
            FlashStatus.error = 1;
        }
      //__HAL_FLASH_INSTRUCTION_CACHE_RESET();
        FLASH->ACR |=  FLASH_ACR_ICRST;
        FLASH->ACR &= ~FLASH_ACR_ICRST;
      //__HAL_FLASH_DATA_CACHE_RESET();
        FLASH->ACR |=  FLASH_ACR_DCRST;
        FLASH->ACR &= ~FLASH_ACR_DCRST;
        __HAL_FLASH_INSTRUCTION_CACHE_ENABLE();
        __HAL_FLASH_DATA_CACHE_ENABLE();

        if( HAL_OK != HAL_FLASH_Lock() ) {
            FlashStatus.error = 1;
        }
    }
    else {
        FlashStatus.error = 1;
    }

    FlashStatus.busy  = 0;
    return( FlashStatus.error ? ARM_DRIVER_ERROR : ARM_DRIVER_OK );
}


/**********************************************************************************************************/
/** @brief      Get Flash status.
***
*** @return     Flash status ARM_FLASH_STATUS.
***********************************************************************************************************/

static ARM_FLASH_STATUS  GetStatus( void )
{
    if( FlashStatus.busy ) {
        if( 0 != HAL_FLASH_GetError() ) {    // HAL_FLASH_ERROR_NONE
            FlashStatus.busy  = 0;
            FlashStatus.error = 1;
        }
    }
    return FlashStatus;
}


/**********************************************************************************************************/
/** @brief      Get Flash information.
***
*** @return     Pointer to Flash information.
***********************************************************************************************************/

static ARM_FLASH_INFO*  GetInfo( void )
{
    return &FlashInfo;
}


/**********************************************************************************************************/
/** @brief      Flash Driver Control Block.
***********************************************************************************************************/

ARM_DRIVER_FLASH  ARM_Driver_Flash_(DRIVER_FLASH_NUM) =
{
    GetVersion,
    GetCapabilities,
    Initialize,
    Uninitialize,
    PowerControl,
    ReadData,
    ProgramData,
    EraseSector,
    EraseChip,
    GetStatus,
    GetInfo
};


/*****************************  END OF FILE  **************************************************************/
/** @}
*** @}
*** @}
***********************************************************************************************************/

