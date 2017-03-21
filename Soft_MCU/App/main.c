/**********************************************************************************************************/
/** @file     main.c
*** @author   Linghu
*** @version  V1.0.0
*** @date     2015/7/24
*** @brief    C Run Library main() function.
***********************************************************************************************************
*** @par Last Commit:
***      \$Author$ \n
***      \$Date$ \n
***      \$Rev$ \n
***      \$URL$ \n
***
*** @par Change Logs:
***      2015/7/24 -- Linghu -- the first version
***********************************************************************************************************/

#include    <stdlib.h>
#include    <stdint.h>
#include    <stdio.h>
#include    <string.h>
#include    <assert.h>

#include    "cmsis_os.h"                /* CMSIS RTOS definitions             */
#include    "rl_net.h"                  /* Network definitions                */
#include    "rl_fs.h"                   /* FileSystem definitions             */

#include    "cfg.h"
#include    "board.h"
#include    "net_user.h"
#include    "netiod.h"
#include    "ProfiBUS_DP.h"


/**********************************************************************************************************/
/** @addtogroup MAIN
*** @{
*** @addtogroup MAIN_Pravate
*** @{
*** @addtogroup                 MAIN_Private_Constants
*** @{
***********************************************************************************************************/

#define PORT_DP2NET     18354
#define PORT_NET2DP     18355


/**********************************************************************************************************/
/** @}
*** @addtogroup                 MAIN_Private_Macros
*** @{
***********************************************************************************************************/



/**********************************************************************************************************/
/** @}
*** @addtogroup                 MAIN_Private_Types
*** @{
***********************************************************************************************************/


/**********************************************************************************************************/
/** @}
*** @addtogroup                 MAIN_Private_Prototypes
*** @{
***********************************************************************************************************/

static void fs_init(const char *drive);
static void thread_dp2net(void const *arg);
static void thread_net2dp(void const *arg);


/**********************************************************************************************************/
/** @}
*** @addtogroup                 MAIN_Private_Variables
*** @{
***********************************************************************************************************/

osThreadDef(thread_dp2net, osPriorityNormal, 1, 0);
osThreadDef(thread_net2dp, osPriorityNormal, 1, 0);

static uint32_t     g_statistic[8] = {0, 0, 0, 0, 0, 0, 0, 0};


/**********************************************************************************************************/
/** @}
*** @addtogroup                 MAIN_Private_Functions
*** @{
***********************************************************************************************************/
/** @brief      Standard entry point for C code.
***********************************************************************************************************/

int main(void)
{
    osThreadSetPriority(osThreadGetId(), osPriorityBelowNormal);
    board_init();                   osDelay(10);    /* Board Initialize                                 */
    fs_init("F0:");                 osDelay(10);    /* File System Initialize: NOR or SPI Flash drive 0 */
    fs_init("M0:");                 osDelay(10);    /* File System Initialize: Memory Card drive 0      */
    fs_init("N0:");                 osDelay(10);    /* File System Initialize: NAND Flash drive 0       */
    if( bsp_clear_key() ) {
        fdelete("config.sys", NULL);
        printf("[Main] System setting reset to default!\r\n");
    }
    cfg_init("config.sys");         osDelay(10);
    PBDP_Init(cfg_get_baudrate());  osDelay(10);    /* PorfiBUS_DP Initialize                           */
    net_init();                     osDelay(10);    /* Net Initialize                                   */
    netiod_init();                  osDelay(10);    /* NetIO Server Initialize                          */

    if( osThreadCreate(osThread(thread_net2dp), NULL) == NULL ) {
        printf("[Main] Initialize Failed!\r\n");    /* Create thread of Net to ProfiBUS_DP              */
    }
    osDelay(10);
    if( osThreadCreate(osThread(thread_dp2net), NULL) == NULL ) {
        printf("[Main] Initialize Failed!\r\n");    /* Create thread of ProfiBUS_DP to Net              */
    }
    osDelay(10);
    osThreadSetPriority(osThreadGetId(), osPriorityNormal);

    while(1) {
        net_main();
        osThreadYield();
    }
}


/**********************************************************************************************************/
/** @brief      Thread of ProfiBUS_DP to Net
***********************************************************************************************************/

static void thread_dp2net(void const *arg)
{
    struct sockaddr_in  addr;
    int                 sock;
    static uint8_t      buff[256 + 16];
    int                 recv;

    (void)arg;
    osDelay(5000);

    printf("[DP2NET] Server start, ");
    if( (sock = socket(AF_INET, SOCK_DGRAM, 0)) < 0 ) {
        printf("malloc socket failed!\r\n");
        return;
    }
    addr.sin_family      = PF_INET;
    addr.sin_port        = htons(PORT_DP2NET);
    addr.sin_addr.s_addr = INADDR_ANY;
    if( bind(sock, (struct sockaddr *)&addr, sizeof(addr)) < 0 ) {
        printf("bind socket failed!\r\n");
        closesocket(sock);
        return;
    } else {
        printf("UDP Port: %d\r\n", PORT_DP2NET);
    }
    addr.sin_family      = PF_INET;
    addr.sin_port        = htons(PORT_NET2DP);
    addr.sin_addr.s_addr = INADDR_NONE;

    for(; ;)
    {
        if( (recv = PBDP_Recv(buff)) <= 0 ) {
            continue;                                   // ProfiBUS_DP Recv Failed
        }
        g_statistic[0] += 1;  g_statistic[1] += recv;   // ProfiBUS_DP Recv Statistic information

        if( !eth_linkstatus_get() ) {
            continue;                                   // Network_IP ETH LinkDown
        }
        addr.sin_addr.s_addr = ~net_mask_local() | net_ipaddr_local();
        if( recv != sendto(sock, (char*)buff, recv, 0, (struct sockaddr*)&addr, sizeof(addr)) ) {
            continue;                                   // Network_IP Send Failed
        }
        g_statistic[2] += 1;  g_statistic[3] += recv;   // Network_IP Send Statistic information
    }
}


/**********************************************************************************************************/
/** @brief      Thread of Net to ProfiBUS_DP
***********************************************************************************************************/

static void thread_net2dp(void const *arg)
{
    struct sockaddr_in  addr;
    int                 alen;
    int                 sock;
    static uint8_t      buff[256 + 16];
    int                 recv;

    (void)arg;
    osDelay(5000);

    printf("[NET2DP] Server start, ");
    if( (sock = socket(AF_INET, SOCK_DGRAM, 0)) < 0 ) {
        printf("malloc socket failed!\r\n");
        return;
    }
    addr.sin_family      = PF_INET;
    addr.sin_port        = htons(PORT_NET2DP);
    addr.sin_addr.s_addr = INADDR_ANY;
    if( bind(sock, (struct sockaddr *)&addr, sizeof(addr)) < 0 ) {
        printf("bind socket failed!\r\n");
        closesocket(sock);
        return;
    } else {
        printf("UDP Port: %d\r\n", PORT_NET2DP);
    }

    for(; ;)
    {
        alen = sizeof(addr);
        if( (recv = recvfrom(sock, (char*)buff, sizeof(buff), 0, (struct sockaddr*)&addr, &alen)) <= 0) {
            continue;                                   // Network_IP Recv Failed
        }
        if( (addr.sin_addr.s_addr == net_ipaddr_local()) || (addr.sin_port != htons(PORT_DP2NET)) ) {
            continue;                                   // Network_IP Recv Invalid
        }
        if( !eth_linkstatus_get() ) {
            continue;                                   // Network_IP ETH LinkDown
        }
        g_statistic[4] += 1;  g_statistic[5] += recv;   // Network_IP Recv Statistic information

        if( recv != PBDP_Send(buff, recv) ) {
            continue;                                   // ProfiBUS_DP Send Failed
        }
        g_statistic[6] += 1;  g_statistic[7] += recv;   // ProfiBUS_DP Send Statistic information
    }
}


/**********************************************************************************************************/
/** @brief      File System Initialize
***
*** @param[in]  drive   a string specifying the memory or storage device. Ref: finit().
***********************************************************************************************************/

static void fs_init(const char *drive)
{
    printf("[FileSystem] Initialize and Mount drive: %.2s ", drive);

    if( fsOK == finit(drive) ) {
        switch( fmount(drive) )
        {
        case fsOK:
            printf ("Succeed! Free space: %lld\r\n", ffree(drive));
            break;

        case fsNoFileSystem:
            printf("not formatted!\r\n");
            break;

        default:
            printf("Mount failed!\r\n");
            break;
        }
    }
    else {  printf("Failed!\r\n");  }
}






uint16_t AD_in (uint32_t ch) {
  return (0);
}
uint8_t get_button (void) {
  return (0);
}

bool LEDrun;
bool LCDupdate;
char lcd_text[2][20+1];

/*****************************  END OF FILE  **************************************************************/
/** @}
*** @}
*** @}
***********************************************************************************************************/

