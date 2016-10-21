/**********************************************************************************************************/
/** @file     cfg.c
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

#include    <stdint.h>
#include    <stdio.h>
#include    <assert.h>

#include    "iniparser.h"
#include    "cfg.h"


/**********************************************************************************************************/
/** @addtogroup CFG
*** @{
*** @addtogroup CFG_Pravate
*** @{
*** @addtogroup                 CFG_Private_Variables
*** @{
***********************************************************************************************************/

dictionary*     g_CfgDic;                   /* Pointer to allocated dictionary  */
char            g_CfgFileName[64];          /* Pointer to config file name      */


/**********************************************************************************************************/
/** @}
*** @addtogroup                 CFG_Private_Functions
*** @{
***********************************************************************************************************/
/** @brief      Config module Initialize
***
*** @param[in]  cfg_file_name   Config file name.
***********************************************************************************************************/

void cfg_init(const char* cfg_file_name)
{
    strncpy(g_CfgFileName, cfg_file_name, sizeof(g_CfgFileName) - 1);
    g_CfgFileName[sizeof(g_CfgFileName) - 1] = '\0';

    iniparser_set_error_callback(NULL);

    g_CfgDic = iniparser_load(g_CfgFileName);
    if( g_CfgDic == NULL )  cfg_reset();
}


/**********************************************************************************************************/
/** @brief      Reset all Config to default value
***********************************************************************************************************/

void cfg_reset(void)
{
    FILE*   fini;

    if( g_CfgDic != NULL ) {
        iniparser_freedict(g_CfgDic);
        g_CfgDic = NULL;
    }

    fini = fopen(g_CfgFileName, "w");  assert(fini);
    fprintf(fini,
            "[System]\n"
            "Password = sanlian       ;\n"
            "\n"
            "[Ethernet]\n"
            "DHCP     = Y             ;\n"
            "IPADDR   = 192.168.0.99  ;\n"
            "DefGW    = 192.168.0.1   ;\n"
            "MASK     = 255.255.255.0 ;\n"
            "MACADDR  = UNDEF         ;\"1E-30-AA-BB-CC-DD\" or \"UNDEF\"\n"
            "\n"
            "[ProfiBUS]\n"
            "BAUD     = 187500        ;\n"
            "\n"
           );
    fclose(fini);

    g_CfgDic = iniparser_load(g_CfgFileName);  assert(g_CfgDic);
}


/**********************************************************************************************************/
/** @brief      Save config to config file.
***********************************************************************************************************/

static void cfg_save(const char* entry, const char* val)
{
    FILE*   fini;

    iniparser_set(g_CfgDic, entry, val);

    fini = fopen(g_CfgFileName, "w");  assert(fini);
    iniparser_dump_ini(g_CfgDic, fini);
    fclose(fini);
}

/**********************************************************************************************************/
/** @brief      Read system password from config file
***********************************************************************************************************/

const char* cfg_get_password(void)
{
    return( iniparser_getstring(g_CfgDic, "System:Password", "sanlian") );
}


/**********************************************************************************************************/
/** @brief      Write system password to config file
***********************************************************************************************************/

void cfg_set_password(const char* psw)
{
    if( psw == NULL )  psw = "sanlian";
    cfg_save("System:Password", psw);
}


/**********************************************************************************************************/
/** @brief      Read DHCP enable or disable form config file.
***********************************************************************************************************/

int cfg_get_dhcp(void)
{
    return( iniparser_getboolean(g_CfgDic, "Ethernet:DHCP", 1) );
}


/**********************************************************************************************************/
/** @brief      Write DHCP enable or disable to config file.
***********************************************************************************************************/

void cfg_set_dhcp(int dhcp)
{
    cfg_save("Ethernet:DHCP", dhcp ? "Y" : "N");
}


/**********************************************************************************************************/
/** @brief      Read IP address from config file.
***********************************************************************************************************/

const char* cfg_get_ipaddr(void)
{
    return( iniparser_getstring(g_CfgDic, "Ethernet:IPADDR", "192.168.0.99") );
}


/**********************************************************************************************************/
/** @brief      Write IP address to config file.
***********************************************************************************************************/

void cfg_set_ipaddr(const char* ipaddr)
{
    if( ipaddr == NULL )  ipaddr = "192.168.0.99";
    cfg_save("Ethernet:IPADDR", ipaddr);
}


/**********************************************************************************************************/
/** @brief      Read Default Gateway Address from config file.
***********************************************************************************************************/

const char* cfg_get_defgw(void)
{
    return( iniparser_getstring(g_CfgDic, "Ethernet:DefGW", "192.168.0.1") );
}


/**********************************************************************************************************/
/** @brief      Write Default Gateway Address to config file.
***********************************************************************************************************/

void cfg_set_defgw(const char* defgw)
{
    if( defgw == NULL )  defgw = "192.168.0.1";
    cfg_save("Ethernet:DefGW", defgw);
}


/**********************************************************************************************************/
/** @brief      Read Local Subnet mask from config file.
***********************************************************************************************************/

const char* cfg_get_mask(void)
{
    return( iniparser_getstring(g_CfgDic, "Ethernet:MASK", "255.255.255.0") );
}


/**********************************************************************************************************/
/** @brief      Write Local Subnet mask to config file.
***********************************************************************************************************/

void cfg_set_mask(const char* mask)
{
    if( mask == NULL )  mask = "255.255.255.0";
    cfg_save("Ethernet:MASK", mask);
}


/**********************************************************************************************************/
/** @brief      Read Local Ethernet MAC Address from config file.
***********************************************************************************************************/

const char* cfg_get_macaddr(void)
{
    return( iniparser_getstring(g_CfgDic, "Ethernet:MACADDR", "UNDEF") );
}


/**********************************************************************************************************/
/** @brief      Write Local Ethernet MAC Address to config file.
***********************************************************************************************************/

void cfg_set_macaddr(const char* macaddr)
{
    if( macaddr == NULL )  macaddr = "UNDEF";
    cfg_save("Ethernet:MACADDR", macaddr);
}


/**********************************************************************************************************/
/** @brief      Read ProfiBUS-DP baudrate from config file.
***********************************************************************************************************/

uint32_t cfg_get_baudrate(void)
{
    return( iniparser_getlongint(g_CfgDic, "ProfiBUS:BAUD", 187500) );
}


/**********************************************************************************************************/
/** @brief      Write ProfiBUS-DP baudrate to config file.
***********************************************************************************************************/

void cfg_set_baudrate(uint32_t baud)
{
    const char* pb;

    switch( baud ) { 
    case 9600:    pb = "9600";    break;  
    case 19200:   pb = "19200";   break;
    case 45450:   pb = "45450";   break;
    case 93750:   pb = "93750";   break;
    case 187500:  pb = "187500";  break;
    case 500000:  pb = "500000";  break;
    case 1500000: pb = "1500000"; break;
    default:      pb = "187500";  break;
    }
    cfg_save("ProfiBUS:BAUD", pb);
}


/*****************************  END OF FILE  **************************************************************/
/** @}
*** @}
*** @}
***********************************************************************************************************/

