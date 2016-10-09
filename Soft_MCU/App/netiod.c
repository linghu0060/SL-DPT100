/**********************************************************************************************************/
/** @file     netio.c
*** @author   Linghu
*** @version  V1.0.0
*** @date     2015/7/24
*** @brief    NetIO Server for CMSIS_OS and MDK TCP/IP stack.
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

#include    <stdint.h>
#include    <stdio.h>
#include    <assert.h>
#include    "cmsis_os.h"            /* CMSIS RTOS definitions               */
#include    "rl_net.h"              /* Network definitions                  */
#include    "netiod.h"


/**********************************************************************************************************/
/** @addtogroup NETIO
*** @{
*** @addtogroup NETIO_Pravate
*** @{
*** @addtogroup                 NETIO_Private_Constants
*** @{
***********************************************************************************************************/

#define DEFAULTPORT     0x494F      /* "IO"                                 */
#define TMAXSIZE        (1 * 1024)  /* buff size                            */
#define INTERVAL        6           /* timeout of seconds                   */

#define CMD_QUIT        0
#define CMD_C2S         1
#define CMD_S2C         2
#define CMD_RES         3
#define CTLSIZE         sizeof(CONTROL)


/**********************************************************************************************************/
/** @}
*** @addtogroup                 NETIO_Private_Macros
*** @{
***********************************************************************************************************/

#ifndef max
#define max(x, y)       ((x) > (y) ? (x) : (y))
#endif

#ifndef min
#define min(x, y)       ((x) < (y) ? (x) : (y))
#endif


/**********************************************************************************************************/
/** @}
*** @addtogroup                 NETIO_Private_Types
*** @{
***********************************************************************************************************/

typedef struct {
    uint32_t    cmd;
    uint32_t    data;
} CONTROL;


/**********************************************************************************************************/
/** @}
*** @addtogroup                 NETIO_Private_Variables
*** @{
***********************************************************************************************************/

static char  cBuffer[TMAXSIZE] __attribute__((aligned(256)));   /* Send & Recv buffer                   */

/**********************************************************************************************************/
/** @}
*** @addtogroup                 NETIO_Private_Prototypes
*** @{
***********************************************************************************************************/

static void NetioD_TCP(void const *arg);

/**********************************************************************************************************/
/** @}
*** @addtogroup                 NETIO_Private_Functions
*** @{
***********************************************************************************************************/
/** @brief      Initialize NetIO Server.
***********************************************************************************************************/

void netiod_init( void )
{
    static osThreadDef(NetioD_TCP, osPriorityAboveNormal, 1, 0);
    osThreadId  threadID;

    threadID = osThreadCreate(osThread(NetioD_TCP), NULL);
    assert(threadID != NULL);  (void)threadID;
}


/**********************************************************************************************************/
/** @brief      NetIO Server function
***********************************************************************************************************/

static void NetioD_TCP(void const *arg)
{
    CONTROL             ctl;
    struct sockaddr_in  addr;
    int                 server, client;
    uint32_t            nByte;
    uint32_t            nData;
    int                 rc;
    uint32_t            ed;

    osDelay(5000);
    printf("[NetIO] Server start, ");
    if( (server = socket(AF_INET, SOCK_STREAM, 0)) < 0 ) {
        printf("malloc socket failed!\r\n");
        return;
    }
    addr.sin_family      = PF_INET;
    addr.sin_port        = htons(DEFAULTPORT);
    addr.sin_addr.s_addr = INADDR_ANY;
    if( bind(server, (struct sockaddr *)&addr, sizeof(addr)) < 0 ) {
        printf("bind socket failed!\r\n");
        closesocket(server);
        return;
    }
    if( listen(server, 2) != 0 ) {
        printf("listen failed!\r\n");
        closesocket(server);
        return;
    } else {
        printf("TCP Port: %d\r\n", DEFAULTPORT);
    }

    for(;;)
    {
        if( (client = accept(server, NULL, NULL)) < 0 )   continue;
      //printf("[NetIO] client accepted \r\n");

        for(rc = 1;  ;  )
        {
            if( recv(client, (void*)&ctl, CTLSIZE, 0) != CTLSIZE )  break;
            ctl.cmd  = ntohl(ctl.cmd);
            ctl.data = ntohl(ctl.data);

            if( ctl.cmd == CMD_C2S )
            {
              //printf("[NetIO] CMD_C2S start: %lu \r\n", ctl.data);
                for(nData = 0, rc = ed = 1;  (rc > 0) && (ed);  nData += ctl.data)
                {
                    for(nByte = 0;  nByte < ctl.data;  nByte += rc)
                    {
                        rc = recv(client, cBuffer, min(TMAXSIZE, ctl.data - nByte), 0);
                        if( rc <= 0 )  break;
                        if( (nByte == 0) && (cBuffer[0] != 0) )  ed = 0;
                    }
                    if( rc <= 0 )   break;
                }
              //printf("[NetIO] CMD_C2S end: %lu \r\n", nData);
            }
            else if( ctl.cmd == CMD_S2C )
            {
                extern volatile uint32_t    os_time;        // only for Keil RTX
                extern const    uint32_t    os_clockrate;   // only for Keil RTX
              //printf("[NetIO] CMD_S2C start: %lu \r\n", ctl.data);
                for(nData = 0, ed = os_time;  (os_time - ed) < (INTERVAL * os_clockrate);  nData += ctl.data)
                {
                    for(cBuffer[0] = 0, nByte = 0;  nByte < ctl.data;  nByte += rc)
                    {
                        rc = send(client, cBuffer, min(TMAXSIZE, ctl.data - nByte), 0);
                        if( rc <= 0 )  break;
                    }
                    if( rc <= 0 )  break;
                }
                {
                    for(cBuffer[0] = 1, nByte = 0;  nByte < ctl.data;  nByte += rc)
                    {
                        rc = send(client, cBuffer, min(TMAXSIZE, ctl.data - nByte), 0);
                        if( rc <= 0 )  break;
                    }
                }
              //printf("[NetIO] CMD_S2C end: %lu \r\n", nData + ctl.data);
            }
            else  break;    /* quit */
        }

        closesocket(client);
      //printf("[NetIO] client close \r\n");
        if( rc <= 0 )  break;
    }

    return;
}


/*****************************  END OF FILE  **************************************************************/
/** @}
*** @}
*** @}
***********************************************************************************************************/

