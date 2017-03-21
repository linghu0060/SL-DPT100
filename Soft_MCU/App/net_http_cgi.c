/**********************************************************************************************************/
/** @file     net_http_cgi.c
*** @author   Linghu
*** @version  V1.0.0
*** @date     2017-01-09
*** @brief    HTTP Server CGI Module
***********************************************************************************************************
*** @par Last Commit:
***      \$Author$ \n
***      \$Date$ \n
***      \$Rev$ \n
***      \$URL$ \n
***
*** @par Change Logs:
***      2017-01-09 -- Linghu -- the first version
***********************************************************************************************************/

#include    <stdio.h>
#include    <stdlib.h>
#include    <string.h>
#include    "rl_net.h"
#include    "net_user.h"
#include    "cfg.h"


/**********************************************************************************************************/
/** @addtogroup net_http_cgi
*** @{
*** @addtogroup net_http_cgi_Pravate
*** @{
*** @addtogroup                 net_http_cgi_Private_Functions
*** @{
***********************************************************************************************************/
/** @brief      Generate dynamic web data from a script line.
***********************************************************************************************************/

uint32_t cgi_script (const char *env, char *buf, uint32_t buflen, uint32_t *pcgi)
{
    uint32_t    len = 0;

    // Analyze a 'c' script line starting position 2
    switch( env[0] ) {
    case 'a' :              // Network parameters from 'setting.cgi'
        switch( env[2] ) {
        case 'i':           // --- Write local IP address
            len = sprintf(buf, &env[4], net_ipaddr_get());
            break;
        case 'm':           // --- Write local network mask
            len = sprintf(buf, &env[4], net_mask_get());
            break;
        case 'g':           // --- Write default gateway IP address
            len = sprintf(buf, &env[4], net_defgw_get());
            break;
        case 'p':           // --- Write primary DNS server IP address
            len = sprintf(buf, &env[4], net_pridns_get());
            break;
        case 's':           // --- Write secondary DNS server IP address
            len = sprintf(buf, &env[4], net_secdns_get());
            break;
        case 'a':           // --- Write MAC Address
            len = sprintf(buf, &env[4], eth_macaddr_get());
            break;
        case 'b':           // --- Write ProfiBUS-DP Baud rate
            len = sprintf(buf, &env[4], cfg_get_baudrate());
            break;
        case 'd':           // --- Write DHCP status
            len = sprintf(buf, &env[4], cfg_get_dhcp() ? "" : "checked");
            break;
        case 'w':           // --- Write Password
            len = sprintf(buf, &env[4], net_psw_get());
            break;
        }
        break;
    }

    return( len );
}


/**********************************************************************************************************/
/** @brief      Process query string received by GET request.
***********************************************************************************************************/

void cgi_process_query(const char *qstr)
{
    char   *var = malloc(40 * sizeof(char));

    do{
        qstr = http_get_env_var(qstr, var, 40 * sizeof(char));  // Loop through all the parameters
        if( var[0] != 0 ) {
            if( strncmp(var, "dhcp=", 5) == 0 ) {               // DHCP
                cfg_set_dhcp(var[5] == 'Y');
            }
            else if( strncmp(var, "ip=", 3) == 0 ) {            // Local IP address
                net_ipaddr_set(&var[3]);
                cfg_set_ipaddr(&var[3]);
            }
            else if( strncmp (var, "msk=", 4) == 0 ) {          // Local network mask
                net_mask_set(&var[4]);
                cfg_set_mask(&var[4]);
            }
            else if( strncmp (var, "gw=", 3) == 0 ) {           // Default gateway IP address
                net_defgw_set(&var[3]);
                cfg_set_defgw(&var[3]);
            }
            else if( strncmp (var, "pdns=", 5) == 0 ) {         // Primary DNS server IP address
                net_pridns_set(&var[5]);
            }
            else if( strncmp (var, "sdns=", 5) == 0 ) {         // Secondary DNS server IP address
                net_secdns_set(&var[5]);
            }
            else if( strncmp (var, "mac=", 4) == 0 ) {          // MAC Address
                eth_macaddr_set(&var[4]);
                cfg_set_macaddr(&var[4]);
            }
            else if( strncmp (var, "bdr=", 4) == 0 ) {          // ProfiBUS-DP Baud rate
                cfg_set_baudrate(atol(&var[4]));
            }
            else if( strncmp (var, "psw=", 4) == 0 ) {          // Password
                net_psw_set(&var[4]);
                cfg_set_password(&var[4]);
            }
        }
    } while(qstr);

    free(var);
}


/*****************************  END OF FILE  **************************************************************/
/** @}
*** @}
*** @}
***********************************************************************************************************/


