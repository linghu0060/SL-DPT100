/**********************************************************************************************************/
/** @file     ProfiBUS_DP.h
*** @author   Linghu
*** @version  V1.0.0
*** @date     2015/7/30
*** @brief    ProfiBUS DP
***********************************************************************************************************
*** @par Last Commit:
***      \$Author$ \n
***      \$Date$ \n
***      \$Rev$ \n
***      \$URL$ \n
***
*** @par Change Logs:
***      2015/7/30 -- Linghu -- the first version
***********************************************************************************************************/
#ifndef __PROFIBUS_DP_H___20150730_112501
#define __PROFIBUS_DP_H___20150730_112501
#ifdef  __cplusplus
extern  "C"
{
#endif
/**********************************************************************************************************/
/** @addtogroup PROFIBUS_DP
*** @{
*** @addtogroup                 PROFIBUS_DP_Exported_Constants
*** @{
***********************************************************************************************************/

#define PBDP_EVENT_ERR      (0x1u << 0)             /* PBDP Event error                 */
#define PBDP_EVENT_IDLE     (0x1u << 1)             /* PBDP Event Recv Idle char        */
#define PBDP_EVENT_TRCP     (0x1u << 2)             /* PBDP Event Transmission complete */


/**********************************************************************************************************/
/** @}
*** @addtogroup                 PROFIBUS_DP_Exported_Functions
*** @{
***********************************************************************************************************/

/* ProfiBUS DP User function */
extern int  PBDP_Statc(char* buff, int size);
extern void PBDP_Init(uint32_t baud);
extern int  PBDP_Recv(uint8_t buff[260]);
extern int  PBDP_Send(const uint8_t *buff, int len);

/* ProfiBUS DP Uart callback function */
extern void PBDP_UART_RecvCB(int ch);
extern int  PBDP_UART_SendCB(void);
extern void PBDP_UART_EventCB(int event);

/* ProfiBUS DP Uart driver function */
extern void PBDP_UART_Init(uint32_t BaudRate);
extern void PBDP_UART_EnDEN(void);
extern void PBDP_UART_DsDEN(void);
extern void PBDP_UART_EnTXE(void);
extern void PBDP_UART_DsTXE(void);
extern void PBDP_UART_EnIRQ(void);
extern void PBDP_UART_DsIRQ(void);

/*****************************  END OF FILE  **************************************************************/
/** @}
*** @}
*****/
#ifdef  __cplusplus
}
#endif
#endif
/**********************************************************************************************************/

