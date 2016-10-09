/**********************************************************************************************************/
/** @file    crc.h
*** @brief   CRC(Cyclic Redundancy Check) calculation.
***
***********************************************************************************************************
*** @copy
***
*** The software is owned by Linghu, and is protected under applicable
*** copyright laws.  All rights are reserved.  Any use in violation of
*** the foregoing restrictions may subject the user to criminal sanctions
*** under applicable laws, as well as to civil liability for the breach of the
*** terms and conditions of this license.
***
*** <h2><center>&copy; COPYRIGHT 2010 Linghu</center></h2>
***********************************************************************************************************/
#ifndef __CRC_H
#define __CRC_H
#ifdef  __cplusplus
extern  "C" {
#endif
/**********************************************************************************************************/
/** @addtogroup CRC
*** @{
*** @addtogroup                 CRC_Exported_Types
*** @{
***********************************************************************************************************/

#ifndef uint8
#define uint8  unsigned char
#endif

#ifndef uint16
#define uint16 unsigned short
#endif

#ifndef uint
#define uint   unsigned int
#endif

#ifndef uint32
#define uint32  unsigned long
#endif


/**********************************************************************************************************/
/** @}
*** @addtogroup                 CRC_Exported_Functions
*** @{
***********************************************************************************************************/

#define CRC16_CCITT_INIT        0x1D0F
#define CRC16_CCITT_XOROUT      0x0000
uint16  crc16_ccitt(uint16 seed, const void* message, uint size);

#define CRC32_PKZIP_INIT        0xFFFFFFFF
#define CRC32_PKZIP_XOROUT      0xFFFFFFFF
uint32  crc32_pkzip(uint32 seed, const void *message, uint size);


/*****************************  END OF FILE  **************************************************************/
#ifdef  __cplusplus
}
#endif
#endif
/*****************************  END OF FILE  **************************************************************/
/** @}
*** @}
******************************  END OF FILE  **************************************************************/
