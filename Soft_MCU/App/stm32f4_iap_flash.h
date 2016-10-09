/**********************************************************************************************************/
/** @file     stm32f4_iap_flash.h
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
#ifndef __IAP_FLASH_H___20150720_151446
#define __IAP_FLASH_H___20150720_151446
#ifdef  __cplusplus
extern  "C"
{
#endif
/**********************************************************************************************************/
/** @addtogroup IAP_FLASH
*** @{
*** @addtogroup                 IAP_FLASH_Exported_Constants
*** @{
***********************************************************************************************************/

#define DRIVER_FLASH_NUM        0                   /* Driver number                        */

#define FLASH_EFS_ADDR          0x08040000          /* Flash base address                   */
#define FLASH_EFS_SIZE          (6 * 128 * 1024)    /* Flash size in bytes                  */

#define FLASH_SECTOR_COUNT      6                   /* Number of Sectors                    */
#define FLASH_SECTOR_SIZE       0                   /* FLASH_SECTORS information used       */
#define FLASH_PAGE_SIZE         4                   /* Programming page size in bytes       */
#define FLASH_PROGRAM_UNIT      4                   /* Smallest programmable unit in bytes  */
#define FLASH_ERASED_VALUE      0xFF                /* Contents of erased memory            */


#define FLASH_SECTORS                                                                          \
    ARM_FLASH_SECTOR_INFO(0x000000*2, 0x20000),     /* Sector size 128kB                    */ \
    ARM_FLASH_SECTOR_INFO(0x010000*2, 0x20000),     /* Sector size 128kB                    */ \
    ARM_FLASH_SECTOR_INFO(0x020000*2, 0x20000),     /* Sector size 128kB                    */ \
    ARM_FLASH_SECTOR_INFO(0x030000*2, 0x20000),     /* Sector size 128kB                    */ \
    ARM_FLASH_SECTOR_INFO(0x040000*2, 0x20000),     /* Sector size 128kB                    */ \
    ARM_FLASH_SECTOR_INFO(0x050000*2, 0x20000)      /* Sector size 128kB                    */


/*****************************  END OF FILE  **************************************************************/
/** @}
*** @}
*****/
#ifdef  __cplusplus
}
#endif
#endif
/**********************************************************************************************************/

