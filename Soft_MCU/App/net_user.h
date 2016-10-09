/**********************************************************************************************************/
/** @file     net_user.h
*** @author   Linghu
*** @version  V1.0.0
*** @date     2015/7/27
*** @brief    TCP/IP stack user.
***********************************************************************************************************
*** @par Last Commit:
***      \$Author$ \n
***      \$Date$ \n
***      \$Rev$ \n
***      \$URL$ \n
***
*** @par Change Logs:
***      2015/7/27 -- Linghu -- the first version
***********************************************************************************************************/



/**********************************************************************************************************/
#ifndef __NET_USER_H___20150727_110308
#define __NET_USER_H___20150727_110308
#ifdef  __cplusplus
extern  "C"
{
#endif
/**********************************************************************************************************/
/** @addtogroup NET_USER
*** @{
*** @addtogroup                 NET_USER_Exported_Constants
*** @{
***********************************************************************************************************/



/**********************************************************************************************************/
/** @}
*** @addtogroup                 NET_USER_Exported_Macros
*** @{
***********************************************************************************************************/



/**********************************************************************************************************/
/** @}
*** @addtogroup                 NET_USER_Exported_Types
*** @{
***********************************************************************************************************/



/**********************************************************************************************************/
/** @}
*** @addtogroup                 NET_USER_Exported_Variables
*** @{
***********************************************************************************************************/



/**********************************************************************************************************/
/** @}
*** @addtogroup                 NET_USER_Exported_Functions
*** @{
***********************************************************************************************************/

void      net_init(const char* config);

int       eth_dhcp_status(void);
void      eth_dhcp_disable(void);

int       eth_macaddr_get(char mac_out[18]);
int       eth_macaddr_set(const char* mac_in);

int       eth_linkstatus_get(void);
void      eth_linkstatus_set(uint8_t status);

uint32_t  net_ipaddr_local(void);
int       net_ipaddr_get(char ip_out[12]);
int       net_ipaddr_set(const char* ip_in);

uint32_t  net_defgw_local(void);
int       net_defgw_get(char ip_out[12]);
int       net_defgw_set(const char* ip_in);

uint32_t  net_mask_local(void);
int       net_mask_get(char ip_out[12]);
int       net_mask_set(const char* ip_in);

int       net_pridns_get(char ip_out[12]);
int       net_pridns_set(const char* ip_in);

int       net_secdns_get(char ip_out[12]);
int       net_secdns_set(const char* ip_in);

int       net_psw_get(char psw_out[20]);
int       net_psw_set(const char* psw_in);


/*****************************  END OF FILE  **************************************************************/
/** @}
*** @}
*****/
#ifdef  __cplusplus
}
#endif
#endif
/**********************************************************************************************************/

