/**********************************************************************************************************/
/** @file     net_user.c
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

#include    <assert.h>
#include    <stdint.h>
#include    <stdio.h>
#include    <string.h>

#include    "stm32f4xx.h"
#include    "cmsis_os.h"                /* CMSIS RTOS definitions             */
#include    "rl_net.h"                  /* Network definitions                */
#include    "rl_net_lib.h"

#include    "cfg.h"
#include    "crc.h"
#include    "net_user.h"

/**********************************************************************************************************/
/** @addtogroup NET_USER
*** @{
*** @addtogroup NET_USER_Pravate
*** @{
*** @addtogroup                 NET_USER_Private_Variables
*** @{
***********************************************************************************************************/

static osTimerId    g_osTimerId;                    /* Timer ID                                 */
static void Timer_Callback(void const *arg);        /* prototype for timer callback function    */
static osTimerDef(Timer, Timer_Callback);           /* define timers                            */

static uint8_t      g_LinkStatus;                   /* ETH Link status: (0)LinkDown, (1)LinkUP  */

/**********************************************************************************************************/
/** @}
*** @addtogroup                 NET_USER_Private_Functions
*** @{
***********************************************************************************************************/
/** @brief      Net Initialize of user
***
*** @param[in]  config  [0]---System Password.\n
***                     [1]---DHCP enable or disable.\n
***                     [2]---IP address.\n
***                     [3]---Default Gateway Address.\n
***                     [4]---Local Subnet mask.\n
***                     [5]---Ethernet MAC Address.\n
***********************************************************************************************************/

void net_init(void)
{
    const char* cfg;
    char        buff[20];
    uint32      unique_id;

    if( (g_osTimerId = osTimerCreate(osTimer(Timer), osTimerOnce, NULL)) == NULL ) {
        printf("[ETH] Initialize Timer Failed!\r\n");
        return;
    }

    cfg = cfg_get_macaddr();                /* Config of Ethernet MAC Address       */
    if( (cfg != NULL) && (strcasecmp(cfg, "UNDEF") != 0) ) {
        eth_macaddr_set(cfg);
    }
    else {
        unique_id = crc32_pkzip(CRC32_PKZIP_INIT, (const void*)0x1FFF7A10, 12) ^ CRC32_PKZIP_XOROUT;
        sprintf( buff, "1E-31-%02X-%02X-%02X-%02X", (unsigned)(unique_id >> 24) & 0xFF
                                                  , (unsigned)(unique_id >> 16) & 0xFF
                                                  , (unsigned)(unique_id >> 8)  & 0xFF
                                                  , (unsigned)(unique_id >> 0)  & 0xFF
               );
        eth_macaddr_set(buff);
    }

    if( netOK != net_initialize() ) {
        printf("[ETH] Initialize Failed!\r\n");
        return;
    }
    NVIC_SetPriority(ETH_IRQn, 4);

    if( !cfg_get_dhcp() ) {                     /* Config of DHCP enable or disable     */
        eth_dhcp_disable();
    }
    if( NULL != (cfg = cfg_get_ipaddr()) ) {    /* Config of IP address                 */
        net_ipaddr_set(cfg);
    }
    if( NULL != (cfg = cfg_get_defgw()) ) {     /* Config of Default Gateway Address    */
        net_defgw_set(cfg);
    }
    if( NULL != (cfg = cfg_get_mask()) ) {      /* Config of Local Subnet mask          */
        net_mask_set(cfg);
    }
    if( NULL != (cfg = cfg_get_password()) ) {  /* Config of System password            */
        net_psw_set(cfg);
    }

    eth_macaddr_get(buff); // eth_dhcp_disable();
    printf("[ETH] Initialize Succeed!\r\n[ETH]     MAC addr: %s\r\n", buff);

    if( eth_dhcp_status() ) {
        printf("[ETH]     DHCP is Enable.\r\n");
    }
    else {
        printf("[ETH]     DHCP is Disable.\r\n");
        net_ipaddr_get(buff);
        printf("[ETH]     IP Address:      %s\r\n", buff);
        net_defgw_get(buff);
        printf("[ETH]     Default Gateway: %s\r\n", buff);
        net_mask_get(buff);
        printf("[ETH]     Subnet Mask:     %s\r\n", buff);
    }
    net_psw_get(buff);
    printf("[ETH]     Web&Ftp User: admin, PSW: %s\r\n", buff);

    return;
}

static void Timer_Callback(void const *arg)
{
    eth_linkstatus_set(1);      // Set Link Status LinkUp
}

/**********************************************************************************************************/
/** brief      net and ethernet.
***********************************************************************************************************/
static int eth_dhcp_en = 1;

int eth_dhcp_status(void)
{
    extern ETH_CFG  eth0_config;    // net_config.h

    if( eth0_config.DhcpCfg.Vcid ) {
        return( eth_dhcp_en );
    }
    else {
        return( 0 );
    }
}

void eth_dhcp_disable(void)
{
    extern ETH_CFG  eth0_config;    // net_config.h

    if((eth_dhcp_en) && (eth0_config.DhcpCfg.Vcid)) {
        eth_dhcp_en = 0;
        if( netOK != dhcp_disable(0) ) {
            printf("[ETH] Disables the Dynamic Host Failed!\r\n");
        }
    }
}

int eth_macaddr_get(char mac_out[18])
{
    extern ETH_CFG  eth0_config;    // net_config.h
    strcpy(mac_out, mac_ntoa(eth0_config.MacAddr));
    return( 0 );
}

int eth_macaddr_set(const char* mac_in)
{
    extern ETH_CFG  eth0_config;    // net_config.h
    return( !mac_aton(mac_in, eth0_config.MacAddr) );
}

int eth_linkstatus_get(void)
{                                   // status: (0)LinkDown, (1)LinkUp
    return(g_LinkStatus);
}

void eth_linkstatus_set(uint8_t status)
{                                   // status: (0)LinkDown, (1)LinkUp
  //Disable Interrupt
    g_LinkStatus = status;
  //Enable Interrupt
}

uint32_t net_ipaddr_local(void)
{
    extern LOCALM   localm[];       // net_sys.c
    return( ((uint32_t*)(localm[NETIF_ETH].IpAddr))[0] );
}

int net_ipaddr_get(char ip_out[12])
{
    extern LOCALM   localm[];       // net_sys.c
    strcpy(ip_out, ip4_ntoa(localm[NETIF_ETH].IpAddr));
    return( 0 );
}

int net_ipaddr_set(const char* ip_in)
{
    extern LOCALM   localm[];       // net_sys.c
    return( !ip4_aton(ip_in, localm[NETIF_ETH].IpAddr) );
}

uint32_t net_defgw_local(void)
{
    extern LOCALM   localm[];       // net_sys.c
    return( ((uint32_t*)(localm[NETIF_ETH].DefGW))[0] );
}

int net_defgw_get(char ip_out[12])
{
    extern LOCALM   localm[];       // net_sys.c
    strcpy(ip_out, ip4_ntoa(localm[NETIF_ETH].DefGW));
    return( 0 );
}

int net_defgw_set(const char* ip_in)
{
    extern LOCALM   localm[];       // net_sys.c
    return( !ip4_aton(ip_in, localm[NETIF_ETH].DefGW) );
}

uint32_t net_mask_local(void)
{
    extern LOCALM   localm[];       // net_sys.c
    return( ((uint32_t*)(localm[NETIF_ETH].NetMask))[0] );
}

int net_mask_get(char ip_out[12])
{
    extern LOCALM   localm[];       // net_sys.c
    strcpy(ip_out, ip4_ntoa(localm[NETIF_ETH].NetMask));
    return( 0 );
}

int net_mask_set(const char* ip_in)
{
    extern LOCALM   localm[];       // net_sys.c
    return( !ip4_aton(ip_in, localm[NETIF_ETH].NetMask) );
}

int net_pridns_get(char ip_out[12])
{
    extern LOCALM   localm[];       // net_sys.c
    strcpy(ip_out, ip4_ntoa(localm[NETIF_ETH].PriDNS));
    return( 0 );
}

int net_pridns_set(const char* ip_in)
{
    extern LOCALM   localm[];       // net_sys.c
    return( !ip4_aton(ip_in, localm[NETIF_ETH].PriDNS) );
}

int net_secdns_get(char ip_out[12])
{
    extern LOCALM   localm[];       // net_sys.c
    strcpy(ip_out, ip4_ntoa(localm[NETIF_ETH].SecDNS));
    return( 0 );
}

int net_secdns_set(const char* ip_in)
{
    extern LOCALM   localm[];       // net_sys.c
    return( !ip4_aton(ip_in, localm[NETIF_ETH].SecDNS) );
}

int net_psw_get(char psw_out[20])
{
    extern HTTP_CFG  http_config;   // Net_Config.c
    extern FTP_CFG   ftp_config;    // Net_Config.c

    if( http_config.EnAuth && ftp_config.EnAuth ) {
      //memcpy(ftp_config.Passw, http_config.Passw, NET_PASSWORD_SIZE);
        strncpy(psw_out, http_config.Passw, 20 - 1);
        psw_out[20 - 1] = '\0';
        return( 0 );
    }
    else {
        return( 1 );
    }
}

int net_psw_set(const char* psw_in)
{
    extern HTTP_CFG  http_config;   // Net_Config.c
    extern FTP_CFG   ftp_config;    // Net_Config.c

    if( http_config.EnAuth && ftp_config.EnAuth ) {
        strncpy(http_config.Passw, psw_in, NET_PASSWORD_SIZE - 1);
        http_config.Passw[NET_PASSWORD_SIZE - 1] = '\0';
        memcpy(ftp_config.Passw, http_config.Passw, NET_PASSWORD_SIZE);
    }

    return( 0 );
}


/**********************************************************************************************************/
/** @brief      Notify the user of DHCP event or extended DHCP option
***
*** @param[in]  if_num  interface number.
*** @param[in]  opt     DHCP option as defined in dhcpClientOption.
*** @param[in]  val     pointer to the option value.
*** @param[in]  len     length of the option value in bytes.
***********************************************************************************************************/

void dhcp_client_notify(uint32_t if_num, dhcpClientOption opt, const uint8_t *val, uint32_t len)
{
    char    buff[12];
    (void)if_num;  (void)len;

    switch( opt ) {
    case dhcpClientIPaddress:       // IP address has changed
        if( eth_dhcp_en ) {
            printf("[ETH] DHCP set IP address: %s\r\n", ip4_ntoa(val));
            net_defgw_get(buff);
            printf("[ETH]     Default Gateway: %s\r\n", buff);
            net_mask_get(buff);
            printf("[ETH]     Subnet Mask:     %s\r\n", buff);
        }
        break;
//  case dhcpClientNTPservers:
//  case dhcpClientBootfileName:
    default:
        break;
    }
}


/**********************************************************************************************************/
/** @brief      Notify the user of Ethernet link state change event.
***
*** @param[in]  if_num  interface number.
*** @param[in]  event   Ethernet link state event as defined in ethLinkEvent.
***********************************************************************************************************/

void eth_link_notify(uint32_t if_num, ethLinkEvent event)
{
    (void)if_num;

    switch( event ) {
    case ethLinkDown:
        printf("[ETH] Ethernet Link Down.\r\n");
        eth_linkstatus_set(0);      // Set Link Status LinkDown
        break;
    case ethLinkUp_10MHalfDuplex:
        printf("[ETH] Ethernet Link Up: 10MBit half duplex.\r\n");
        goto LINK_DELAY;            // Delay to Set Link Status LinkUp
    case ethLinkUp_10MFullDuplex:
        printf("[ETH] Ethernet Link Up: 10MBit full duplex.\r\n");
        goto LINK_DELAY;            // Delay to Set Link Status LinkUp
    case ethLinkUp_100MHalfDuplex:
        printf("[ETH] Ethernet Link Up: 100MBit half duplex.\r\n");
        goto LINK_DELAY;            // Delay to Set Link Status LinkUp
    case ethLinkUp_100MFullDuplex:
        printf("[ETH] Ethernet Link Up: 100MBit full duplex.\r\n");
        goto LINK_DELAY;            // Delay to Set Link Status LinkUp
    case ethLinkUp_1GHalfDuplex:
        printf("[ETH] Ethernet Link Up: 1000MBit half duplex.\r\n");
        goto LINK_DELAY;            // Delay to Set Link Status LinkUp
    case ethLinkUp_1GFullDuplex:
        printf("[ETH] Ethernet Link Up: 1000MBit full duplex.\r\n");
        LINK_DELAY:                 // Delay to Set Link Status LinkUp
        if( osTimerStart(g_osTimerId, 500) != osOK ) {
            printf("[ETH] Timer Start Failed!\r\n");
        }
        break;
    default:
        break;
    }
}


/*****************************  END OF FILE  **************************************************************/
/** @}
*** @}
*** @}
***********************************************************************************************************/

