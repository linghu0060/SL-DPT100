/**********************************************************************************************************/
/** @file     cfg.h
*** @author   Linghu
*** @version  V1.0.0
*** @date     2016-10-18
*** @brief    Read and Write Config to ".ini" file
***********************************************************************************************************
*** @par Last Commit:
***      \$Author$ \n
***      \$Date$ \n
***      \$Rev$ \n
***      \$URL$ \n
***
*** @par Change Logs:
***      2016-10-18 -- Linghu -- the first version
***********************************************************************************************************/
#ifndef __CFG_H___20161018_155239
#define __CFG_H___20161018_155239
#ifdef  __cplusplus
extern  "C"
{
#endif
/**********************************************************************************************************/
/** @addtogroup CFG
*** @{
*** @addtogr
*** @addtogroup                 CFG_Exported_Functions
*** @{
***********************************************************************************************************/

void        cfg_init(const char* cfg_file_name);
void        cfg_reset(void);

const char* cfg_get_password(void);
void        cfg_set_password(const char* psw);

int         cfg_get_dhcp(void);
void        cfg_set_dhcp(int dhcp);

const char* cfg_get_ipaddr(void);
void        cfg_set_ipaddr(const char* ipaddr);

const char* cfg_get_defgw(void);
void        cfg_set_defgw(const char* defgw);

const char* cfg_get_mask(void);
void        cfg_set_mask(const char* mask);

const char* cfg_get_macaddr(void);
void        cfg_set_macaddr(const char* macaddr);

uint32_t    cfg_get_baudrate(void);
void        cfg_set_baudrate(uint32_t baud);


/*****************************  END OF FILE  **************************************************************/
/** @}
*** @}
*****/
#ifdef  __cplusplus
}
#endif
#endif
/**********************************************************************************************************/

