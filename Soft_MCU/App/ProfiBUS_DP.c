/**********************************************************************************************************/
/** @file     ProfiBUS_DP.c
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

#include    <stddef.h>
#include    <stdbool.h>
#include    <stdint.h>
#include    <string.h>
#include    <stdio.h>
#include    "cmsis_os.h"
#include    "ProfiBUS_DP.h"

/**********************************************************************************************************/
/** @addtogroup PROFIBUS_DP
*** @{
*** @addtogroup PROFIBUS_DP_Pravate
*** @{
*** @addtogroup                 PROFIBUS_DP_Private_Constants
*** @{
***********************************************************************************************************/

#define PBDP_EVENT_ERR      (0x1u << 0)             /* PBDP Event error                 */
#define PBDP_EVENT_IDLE     (0x1u << 1)             /* PBDP Event Recv Idle char        */
#define PBDP_EVENT_CPLT     (0x1u << 2)             /* PBDP Event Transmission complete */

#define PBDP_FRAME_SD1      (0x10)
#define PBDP_FRAME_SD1L     (1 + (3) + 1 + 1)
#define PBDP_FRAME_SD2      (0x68)
#define PBDP_FRAME_SD2L     (1 + 2 + 1 + (0) + 1 + 1)
#define PBDP_FRAME_SD3      (0xA2)
#define PBDP_FRAME_SD3L     (1 + (11) + 1 + 1)
#define PBDP_FRAME_SD4      (0xDC)
#define PBDP_FRAME_SD4L     (1 + 2)
#define PBDP_FRAME_SC       (0xE5)
#define PBDP_FRAME_SCL      (1)
#define PBDP_FRAME_ED       (0x16)

#define PBDP_RX_BUF_LEN     (1024)


/**********************************************************************************************************/
/** @}
*** @addtogroup                 PROFIBUS_DP_Private_Macros
*** @{
***********************************************************************************************************/

#define QUEUE_TYPE(type, len)   /* 队列定义, (type)队列元素类型, (len)队列元素数组长度,必须为(2 ^ n)    */ \
                                struct                        /* (len)最大值不得超过(2 ^ 15)            */ \
                                {   ##type      elem[##len];  /* (type)不能为数组类型, 如<char[8]>      */ \
                                    uint16_t    head, tail;   /* head --- 头进, tail --- 尾出           */ \
                                }
#define QUEUE_INIT(queue)       /* 初始化                                                               */ \
                                ( (queue).head = (queue).tail = 0                                          \
                                )
#define QUEUE_SIZE(queue)       /* 返回元素个数                                                         */ \
                                ( ((queue).head - (queue).tail) % __Q_LEN(queue)                           \
                                )
#define QUEUE_EMPTY(queue)      /* 判断是否为空, (TRUE)为空, (FALSE)非空                                */ \
                                ( ((queue).tail % __Q_LEN(queue)) == ((queue).head % __Q_LEN(queue))       \
                                )
#define QUEUE_FULL(queue)       /* 判断是否为满, (TRUE)已满, (FALSE)非满                                */ \
                                ( ((queue).tail % __Q_LEN(queue)) == (((queue).head + 1) % __Q_LEN(queue)) \
                                )
#define QUEUE_PUSH(queue)       /* 插入(队头), 需要右值来赋值  ---注意必须保证队列非满---               */ \
                                ( (queue).elem[ ((queue).head++) % __Q_LEN(queue) ]                        \
                                )
#define QUEUE_POP(queue)        /* 弹出(队尾), 返回删除的元素  ---注意必须保证队列非空---               */ \
                                ( (queue).elem[ ((queue).tail++) % __Q_LEN(queue) ]                        \
                                )
#define QUEUE_DEL(queue, num)   /* 删除, (num)删除个数   ---注意删除前必须保证队列有(num)个元素---      */ \
                                ( (queue).tail += (num)                                                    \
                                )
#define QUEUE_GET(queue, pos)   /* 获取队尾(pos)位置元素 ---注意删除前必须保证队列有(pos)个元素---      */ \
                                ( (queue).elem[ ((queue).tail + (pos)) % __Q_LEN(queue) ]                  \
                                )
#define __Q_LEN(queue)          ( sizeof((queue).elem) / sizeof((queue).elem[0]) )

/**********************************************************************************************************/
/** @}
*** @addtogroup                 PROFIBUS_DP_Private_Types
*** @{
***********************************************************************************************************/

typedef struct {    /*------------- PBDP Information (Run-Time) ------------------------------
                    -- (tx_num)(tx_cnt)(tx_chk)(tx_sig) -- must guarantee Atomic-Access ------
                    -- (rx_que.head)(rx_que.tail) -------- must guarantee Atomic-Access ----*/
    uint16_t                                idl_cnt;    /* Counter of sequential idle char  */

    uint16_t                                tx_num;     /* Total number of transmit buffer  */
    uint16_t                                tx_cnt;     /* Number of data transmitted       */
    uint16_t                                tx_chk;     /* Number of data check             */
    const uint8_t                          *tx_buf;     /* Buffer of transmit               */
    osThreadId                              tx_sig;     /* OS Signal flags of transmit      */
    osMutexId                               tx_mut;     /* OS Mutex of transmit             */

    osSemaphoreId                           rx_sem;     /* OS Semaphore of Received ED      */
    osMutexId                               rx_mut;     /* OS Mutex of Receive              */
    int                                     rx_enb;     /* Receive Enable(1)/Disable(0)     */
    QUEUE_TYPE(uint8_t, PBDP_RX_BUF_LEN)    rx_que;     /* Receive Data Circular Queue      */

    uint32_t                                rxd_cnt;    /* Counter of UART received data    */
    uint32_t                                txd_cnt;    /* Counter of UART transmit data    */
    uint32_t                                err_evt;    /* Counter of UART Error event      */
    uint32_t                                err_ovr;    /* Counter of Queue Overrun error   */
    uint32_t                                err_chk;    /* Counter of transmit check error  */
} PBDP_INFO;

#define PBDP_IDLE_CNT_CLR()     { /*if( PBDP_Info.tx_sig ) {                             */ \
                                  /*    osSignalClear(PBDP_Info.tx_sig, PBDP_EVENT_IDLE);*/ \
                                  /*}                                                    */ \
                                    PBDP_Info.idl_cnt = 0;                                  \
                                }
#define PBDP_IDLE_CNT_INC()     {   if( PBDP_Info.idl_cnt < 0xFFF0 ) {                      \
                                        PBDP_Info.idl_cnt++;                                \
                                    }                                                       \
                                }
#define PBDP_TX_SIG_SEND(evt)   {   if( PBDP_Info.tx_sig ) {                                \
                                        osSignalSet(PBDP_Info.tx_sig, evt);                 \
                                    }                                                       \
                                }
#define PBDP_RX_QUE_PUSH(val)   {   if( PBDP_Info.rx_enb ) {                                \
                                        if(!QUEUE_FULL(PBDP_Info.rx_que) ) {                \
                                            QUEUE_PUSH(PBDP_Info.rx_que) = (val);           \
                                        } else {                                            \
                                            PBDP_DBG_OVR_INC();                             \
                                        }                                                   \
                                    }                                                       \
                                }
#define PBDP_ENTER_RECV_STA()   (PBDP_UART_DsDEN(), PBDP_Info.tx_num = 0, PBDP_Info.tx_buf = NULL)
                                //{ if(PBDP_Info.tx_num != 0) PBDP_Info.tx_buf = NULL; }

#define PBDP_DBG_RXD_INIT()     ( PBDP_Info.rxd_cnt = 0 )
#define PBDP_DBG_RXD_INC()      ( PBDP_Info.rxd_cnt++   )
#define PBDP_DBG_TXD_INIT()     ( PBDP_Info.txd_cnt = 0 )
#define PBDP_DBG_TXD_INC()      ( PBDP_Info.txd_cnt++   )
#define PBDP_DBG_ERR_INIT()     ( PBDP_Info.err_evt = 0 )
#define PBDP_DBG_ERR_INC()      ( PBDP_Info.err_evt++   )
#define PBDP_DBG_OVR_INIT()     ( PBDP_Info.err_ovr = 0 )
#define PBDP_DBG_OVR_INC()      ( PBDP_Info.err_ovr++   )
#define PBDP_DBG_CHK_INIT()     ( PBDP_Info.err_chk = 0 )
#define PBDP_DBG_CHK_INC()      ( PBDP_Info.err_chk++   )

/**********************************************************************************************************/
/** @}
*** @addtogroup                 PROFIBUS_DP_Private_Variables
*** @{
***********************************************************************************************************/

static PBDP_INFO      PBDP_Info;                        /* PBDP Information (Run-Time)      */
static osMutexDef    (PBDP_tx_mut);                     /* PBDP Mutex definition            */
static osMutexDef    (PBDP_rx_mut);                     /* PBDP Mutex definition            */
static osSemaphoreDef(PBDP_rx_sem);                     /* PBDP Semaphore definition        */

/**********************************************************************************************************/
/** @}
*** @addtogroup                 PROFIBUS_DP_Private_Functions
*** @{
***********************************************************************************************************/

// static PBDP_IsAck(const uint8_t *buff)
// {
//     switch(  ) {
//     }
// }


/**********************************************************************************************************/
/** @brief      Get PorfiBUS_DP statistic information
***
*** @param[out] buff    Output statistic information string
*** @param[in]  size    size of buff(in bytes)
***
*** @return     (< 0)Error. (other)number of output char, not counting the terminating null char
***********************************************************************************************************/
#if 0
int PBDP_Statc(char* buff, int size)
{
    int n, m;

    if( (buff == NULL) || (size == 0) )  { return( 0 );        }

    n = snprintf( buff, size,
                  "DP Rx: %u(Frame) %u(Byte)\r\n"
                  "DP Tx: %u(Frame) %u(Byte)\r\n"
                  "DP Over Run ERR: %u\r\n"
                  "DP CHK(TxD) ERR: %u\r\n",
                  PBDP_Info.rxf_cnt, PBDP_Info.rxd_cnt,
                  PBDP_Info.txf_cnt, PBDP_Info.txd_cnt,
                  PBDP_Info.err_ovr,
                  PBDP_Info.err_chk
                );
    if( n < 0 ) /**********************/ { return( n );        }
    if( n >= size ) /******************/ { return( size - 1 ); }

    m = PBDP_UART_Statc(&(buff[n]), size - n);
    if( m < 0 ) /**********************/ { return( n );        }
    if( m >= (size - n) ) /************/ { return( size - 1 ); }
    else /*****************************/ { return( m + n );    }
}
#endif

/**********************************************************************************************************/
/** @brief      PorfiBUS_DP Initialize
***
*** @param[in]  config  A pointer of band rate.
***********************************************************************************************************/

void PBDP_Init(uint32_t baud)
{
    PBDP_Info.idl_cnt = 0;      /* PBDP Information Initialize  */
    PBDP_Info.tx_sig  = NULL;
    PBDP_Info.tx_mut  = osMutexCreate(osMutex(PBDP_tx_mut));
    PBDP_Info.tx_buf  = NULL;
    PBDP_Info.tx_num  = 0;
    PBDP_Info.tx_cnt  = 0;
    PBDP_Info.tx_chk  = 0;
    PBDP_Info.rx_sem  = osSemaphoreCreate(osSemaphore(PBDP_rx_sem), 0);
    PBDP_Info.rx_mut  = osMutexCreate(osMutex(PBDP_rx_mut));
    PBDP_Info.rx_enb  = 0;
    QUEUE_INIT(PBDP_Info.rx_que);

    PBDP_DBG_RXD_INIT();
    PBDP_DBG_TXD_INIT();
    PBDP_DBG_ERR_INIT();
    PBDP_DBG_OVR_INIT();
    PBDP_DBG_CHK_INIT();

    if( (PBDP_Info.tx_mut == NULL) || (PBDP_Info.rx_sem == NULL) || (PBDP_Info.rx_mut == NULL) ) {
        printf("[ProfiBUS DP] Initialize Failed!\r\n");
    }

    PBDP_UART_Init(baud);       /* UART Initialize              */
    printf("[ProfiBUS DP] Initialize Succeed! Baud rate: %u.\r\n", baud);
}


/**********************************************************************************************************/
/** @brief      PorfiBUS_DP data Receive
***
*** @param[out] buff    Buffer of Recv data
***
*** @return     Length of Received data
***********************************************************************************************************/

int PBDP_Recv(uint8_t buff[260])
{
#   define  PBDP_FRAME_COPY_TO_OUT_BUF(size)    for( rx_len = 0;  rx_len < size;  rx_len++ ) {               \
                                                    buff[rx_len]     = QUEUE_GET(PBDP_Info.rx_que, rx_len);  \
                                                }
#   define  PBDP_FRAME_SD1_FORMAT_CHECK(len)    (   (PBDP_FRAME_ED  != QUEUE_GET(PBDP_Info.rx_que, len))     \
                                                 || (PBDP_FRAME_ED  != QUEUE_GET(PBDP_Info.rx_que, len - 1)) \
                                                )
#   define  PBDP_FRAME_SD2_FORMAT_CHECK(len)    (   (PBDP_FRAME_ED  != QUEUE_GET(PBDP_Info.rx_que, len))     \
                                                 || (PBDP_FRAME_ED  != QUEUE_GET(PBDP_Info.rx_que, len - 1)) \
                                                 || (PBDP_FRAME_SD2 != QUEUE_GET(PBDP_Info.rx_que, 3))       \
                                                )
#   define  PBDP_FRAME_SD3_FORMAT_CHECK(len)    (   (PBDP_FRAME_ED  != QUEUE_GET(PBDP_Info.rx_que, len))     \
                                                 || (PBDP_FRAME_ED  != QUEUE_GET(PBDP_Info.rx_que, len - 1)) \
                                                )
#   define  PBDP_FRAME_SD4_FORMAT_CHECK(len)    (   (PBDP_FRAME_ED  != QUEUE_GET(PBDP_Info.rx_que, len)) )
#   define  PBDP_FRAME_SC_FORMAT_CHECK(len)     (   (PBDP_FRAME_ED  != QUEUE_GET(PBDP_Info.rx_que, len)) )
#   define  GOTO_RET(ret)   { result = ret;  goto RECV_RET; }
#   define  GOTO_DEL(ret)   { result = ret;  goto RECV_DEL; }

    int         rx_len,  result;

    if( buff == NULL ) /*************************************/ { return( -1 ); }
    if( osOK != osMutexWait(PBDP_Info.rx_mut, osWaitForever) ) { return( -1 ); }/* osMutexWait Error        */
    PBDP_Info.rx_enb = 1;                                                       /* Receive Enable           */
    if( osSemaphoreWait(PBDP_Info.rx_sem, osWaitForever) <= 0) { GOTO_RET(-2); }/* Waiting for Received ED  */

    while(1) {
        if(  (rx_len = QUEUE_SIZE(PBDP_Info.rx_que)) <= 0 ) {    GOTO_RET(-3); }/* Receiver Queue is empty  */

        switch( QUEUE_GET(PBDP_Info.rx_que, 0) ) {
        case PBDP_FRAME_SD1:    /*--------------------- DLPDUs of fixed length with no data field ----------*/
            if( rx_len <= PBDP_FRAME_SD1L ) { /* Return FALSE */ GOTO_RET(-4); }/* Less the Frame data Len  */
            if( PBDP_FRAME_SD1_FORMAT_CHECK(PBDP_FRAME_SD1L) ) { GOTO_DEL(-4); }/* Frame format invalid     */
            PBDP_FRAME_COPY_TO_OUT_BUF(PBDP_FRAME_SD1L);                        /* Copy data to out buffer  */
            QUEUE_DEL(PBDP_Info.rx_que, PBDP_FRAME_SD1L + 1);    GOTO_RET(PBDP_FRAME_SD1L);

        case PBDP_FRAME_SD2:    /*--------------------- DLPDUs with variable data field length -------------*/
            result = PBDP_FRAME_SD2L + QUEUE_GET(PBDP_Info.rx_que, 1);
            if( rx_len <= result ) { /*--------- Return FALSE */ GOTO_RET(-5); }/* Less the Frame data Len  */
            if( PBDP_FRAME_SD2_FORMAT_CHECK(result) ) {          GOTO_DEL(-5); }/* Frame format invalid     */
            PBDP_FRAME_COPY_TO_OUT_BUF(result);                                 /* Copy data to out buffer  */
            QUEUE_DEL(PBDP_Info.rx_que, result + 1);             GOTO_RET(result);

        case PBDP_FRAME_SD3:    /*--------------------- DLPDUs of fixed length with data field -------------*/
            if( rx_len <= PBDP_FRAME_SD3L ) { /* Return FALSE */ GOTO_RET(-6); }/* Less the Frame data Len  */
            if( PBDP_FRAME_SD3_FORMAT_CHECK(PBDP_FRAME_SD3L) ) { GOTO_DEL(-6); }/* Frame format invalid     */
            PBDP_FRAME_COPY_TO_OUT_BUF(PBDP_FRAME_SD3L);                        /* Copy data to out buffer  */
            QUEUE_DEL(PBDP_Info.rx_que, PBDP_FRAME_SD3L + 1);    GOTO_RET(PBDP_FRAME_SD3L);

        case PBDP_FRAME_SD4:    /*--------------------- token DLPDU ----------------------------------------*/
            if( rx_len <= PBDP_FRAME_SD4L ) { /* Return FALSE */ GOTO_RET(-7); }/* Less the Frame data Len  */
            if( PBDP_FRAME_SD4_FORMAT_CHECK(PBDP_FRAME_SD4L) ) { GOTO_DEL(-7); }/* Frame format invalid     */
            PBDP_FRAME_COPY_TO_OUT_BUF(PBDP_FRAME_SD4L);                        /* Copy data to out buffer  */
            QUEUE_DEL(PBDP_Info.rx_que, PBDP_FRAME_SD4L + 1);    GOTO_RET(PBDP_FRAME_SD4L);

        case PBDP_FRAME_SC:     /*--------------------- Short Acknowledgement ------------------------------*/
            if( rx_len <= PBDP_FRAME_SCL ) { /* Return FALSE  */ GOTO_RET(-8); }/* Less the Frame data Len  */
            if( PBDP_FRAME_SC_FORMAT_CHECK(PBDP_FRAME_SCL) ) {   GOTO_DEL(-8); }/* Frame format invalid     */
            PBDP_FRAME_COPY_TO_OUT_BUF(PBDP_FRAME_SCL);                         /* Copy data to out buffer  */
            QUEUE_DEL(PBDP_Info.rx_que, PBDP_FRAME_SCL + 1);     GOTO_RET(PBDP_FRAME_SCL);

        default:  RECV_DEL:     /*--------------------- End_Delimiter and Other Character ------------------*/
        //  if( PBDP_FRAME_ED != QUEUE_POP(PBDP_Info.rx_que) ) {                /* Queue Delete             */
        //      printf(   "[ProfiBUS DP] Recv invalid data: "
        //                "0x%02X 0x%02X 0x%02X 0x%02X *0x%02X* 0x%02X 0x%02X 0x%02X 0x%02X\r\n"
        //              , (int)(QUEUE_GET(PBDP_Info.rx_que, -5) & 0xFF)
        //              , (int)(QUEUE_GET(PBDP_Info.rx_que, -4) & 0xFF)
        //              , (int)(QUEUE_GET(PBDP_Info.rx_que, -3) & 0xFF)
        //              , (int)(QUEUE_GET(PBDP_Info.rx_que, -2) & 0xFF)
        //              , (int)(QUEUE_GET(PBDP_Info.rx_que, -1) & 0xFF)
        //              , (int)(QUEUE_GET(PBDP_Info.rx_que,  0) & 0xFF)
        //              , (int)(QUEUE_GET(PBDP_Info.rx_que,  1) & 0xFF)
        //              , (int)(QUEUE_GET(PBDP_Info.rx_que,  2) & 0xFF)
        //              , (int)(QUEUE_GET(PBDP_Info.rx_que,  3) & 0xFF)
        //            );
        //  }
        //  break;
            QUEUE_DEL(PBDP_Info.rx_que, 1); /*****************/  break;         /* Queue Delete             */
        }   /* End switch   */
    }       /* End while(1) */

    RECV_RET:  if( osOK != osMutexRelease(PBDP_Info.rx_mut) ) { return( -1 ); } /* osMutexRelease Error     */
    return( result );
}


/**********************************************************************************************************/
/** @brief      PorfiBUS_DP data Send
***
*** @param[in]  buff    Pointer to Send data
*** @param[in]  len     Length  of Send data
***
*** @return     Length of Send data
***********************************************************************************************************/

int PBDP_Send(const uint8_t *buff, int len)
{
    static int  g_count = 0;
    int         cnt;
    osEvent     evt;

    if( (buff == NULL) || (len <= 0) ) /********************/   return( -1 );
    switch( buff[0] ) {
    case PBDP_FRAME_SD1:  cnt = (buff[3] & 0x40) ? (3) : (1);   break;
    case PBDP_FRAME_SD2:  cnt = (buff[6] & 0x40) ? (3) : (1);   break;
    case PBDP_FRAME_SD3:  cnt = (buff[3] & 0x40) ? (3) : (1);   break;
    case PBDP_FRAME_SD4:  cnt = 3; /************************/   break;
    case PBDP_FRAME_SC:   cnt = 1; /************************/   break;
    default: /**********************************************/   return( -1 );
    }
  //cnt += g_count;// + (osKernelSysTick() & 0x01);

    if( osOK != osMutexWait(PBDP_Info.tx_mut, osWaitForever) )  return( -2 );   /* osMutexWait Error    */
    {
        PBDP_UART_DsIRQ();                                          /* UART Interrupt Request Disable   */
        PBDP_Info.tx_sig = osThreadGetId();
        PBDP_Info.tx_num = 0;                                       /* Enable to Enter the Send status  */
        PBDP_Info.tx_buf = buff;                                    /* Pointer to transmit buffer       */
        PBDP_Info.tx_cnt = len;                                     /* Total of transmit buffer         */
        PBDP_Info.tx_chk = cnt;                                     /* PBDP synchronization period      */
        PBDP_UART_EnIRQ();                                          /* USART Interrupt Request Enable   */

//      while( 1 ) {
//          if( osEventSignal != (evt = osSignalWait(0, osWaitForever)).status )  continue;
//          if( PBDP_EVENT_IDLE != evt.value.signals )  continue;   /* Waiting for PBDP_EVENT_IDLE      */
//          if( PBDP_Info.tx_num != 0 )  break;
//      }
        evt = osSignalWait(0, osWaitForever);                       /* Waiting for UART Event           */

        PBDP_UART_DsIRQ();                                          /* UART Interrupt Request Disable   */
        PBDP_Info.tx_num = 0;
        PBDP_Info.tx_cnt = 0;
        PBDP_Info.tx_chk = 0;
        PBDP_Info.tx_buf = NULL;
        PBDP_Info.tx_sig = NULL;
        PBDP_UART_EnIRQ();                                          /* USART Interrupt Request Enable   */
    }
    if( osOK != osMutexRelease(PBDP_Info.tx_mut) )  return( -2 );               /* osMutexRelease Error */

    if( osEventSignal != evt.status )/***********/ { len = -3; goto TX_ERR; }   /* osSignalWait Error   */
    if( evt.value.signals & PBDP_EVENT_ERR )/****/ { len = -4; goto TX_ERR; }
    if( evt.value.signals & PBDP_EVENT_IDLE )/***/ { len = -5; goto TX_ERR; }
    if( evt.value.signals != PBDP_EVENT_CPLT )/**/ { len = -6; goto TX_ERR; }
    TX_ERR:  g_count = (len <= 0) ? (0) : ((g_count + 1) % 4);
    //  if( len <= 0 ) {
    //      printf(   "[ProfiBUS DP] Send invalid data: "
    //                "*0x%04X* 0x%02X 0x%02X 0x%02X 0x%02X 0x%02X 0x%02X 0x%02X 0x%02X\r\n"
    //                "              buff: 0x%02X 0x%02X 0x%02X 0x%02X 0x%02X 0x%02X 0x%02X\r\n"
    //                "              ERR: %d\r\n"
    //              , (int)(g_EvtIRQ.head % 1024)
    //              , (int)(g_EvtIRQ.elem[ (g_EvtIRQ.head + (-8)) % __Q_LEN(g_EvtIRQ) ] & 0xFF)
    //              , (int)(g_EvtIRQ.elem[ (g_EvtIRQ.head + (-7)) % __Q_LEN(g_EvtIRQ) ] & 0xFF)
    //              , (int)(g_EvtIRQ.elem[ (g_EvtIRQ.head + (-6)) % __Q_LEN(g_EvtIRQ) ] & 0xFF)
    //              , (int)(g_EvtIRQ.elem[ (g_EvtIRQ.head + (-5)) % __Q_LEN(g_EvtIRQ) ] & 0xFF)
    //              , (int)(g_EvtIRQ.elem[ (g_EvtIRQ.head + (-4)) % __Q_LEN(g_EvtIRQ) ] & 0xFF)
    //              , (int)(g_EvtIRQ.elem[ (g_EvtIRQ.head + (-3)) % __Q_LEN(g_EvtIRQ) ] & 0xFF)
    //              , (int)(g_EvtIRQ.elem[ (g_EvtIRQ.head + (-2)) % __Q_LEN(g_EvtIRQ) ] & 0xFF)
    //              , (int)(g_EvtIRQ.elem[ (g_EvtIRQ.head + (-1)) % __Q_LEN(g_EvtIRQ) ] & 0xFF)
    //              , (int)buff[0], (int)buff[1], (int)buff[2], (int)buff[3], (int)buff[4], (int)buff[5], (int)buff[6]
    //              , len
    //            );
    //  }
    return( len );
}


/**********************************************************************************************************/
/** @brief       UART callbacks of Receive data register not empty
***
*** @param[in]   ch     UART Receive data
***********************************************************************************************************/

void PBDP_UART_RecvCB(int ch)
{
    PBDP_IDLE_CNT_CLR();                                        /* Clear Counter of Received IDLE ------*/

    if( PBDP_Info.tx_buf == NULL ) {    /*------ Recving -----------------------------------------------*/
        goto RECV_RECV;
    } else if( PBDP_Info.tx_num != 0 ) {/*------ Sending -----------------------------------------------*/
        if( (ch & 0xFF) != PBDP_Info.tx_buf[PBDP_Info.tx_chk++] ) {
            PBDP_ENTER_RECV_STA();                              /* Enter the Receive status             */
            PBDP_TX_SIG_SEND(PBDP_EVENT_ERR);                   /* Transmit Data check the error        */
            PBDP_DBG_CHK_INC();
        }
        if( PBDP_Info.tx_chk == PBDP_Info.tx_num ) {
            PBDP_ENTER_RECV_STA();                              /* Enter the Receive status             */
            PBDP_TX_SIG_SEND(PBDP_EVENT_CPLT);                  /* Transmission complete                */
        }
    } else {                            /*------ PreSend(Recving) --------------------------------------*/
        RECV_RECV: PBDP_RX_QUE_PUSH(ch & 0xFF);                 /* Insert to Receiver queue             */
        if( PBDP_FRAME_ED == (ch & 0xFF) ) {
            osSemaphoreRelease(PBDP_Info.rx_sem);               /* Increase count of Received ED        */
        }
        PBDP_DBG_RXD_INC();
    }
}


/**********************************************************************************************************/
/** @brief      UART callbacks of Transmit data register empty
***
*** @return     (>= 0)Transmit Data, (< 0)No Transmit Data or Error.
***********************************************************************************************************/

int PBDP_UART_SendCB(void)
{
    PBDP_IDLE_CNT_CLR();                                        /* Clear Counter of Received IDLE ------*/

    if( PBDP_Info.tx_buf == NULL ) {    /*------ Recving -----------------------------------------------*/
        goto SEND_RECV;
    } else if( PBDP_Info.tx_num != 0 ) {/*------ Sending -----------------------------------------------*/
        if( PBDP_Info.tx_cnt < PBDP_Info.tx_num ) {
            PBDP_DBG_TXD_INC();
            return( PBDP_Info.tx_buf[PBDP_Info.tx_cnt++] );     /* Return the Send data                 */
        }
        PBDP_UART_DsTXE();                                      /* UART TXE interrupt Disable           */
        return( -1 );                                           /* Return no Send data                  */
    } else {                            /*------ PreSend(Recving) --------------------------------------*/
        SEND_RECV: PBDP_UART_DsTXE();                           /* UART TXE interrupt Disable           */
        return( -2 );                                           /* Return Error                         */
    }
}


/**********************************************************************************************************/
/** @brief      UART callbacks of Event.
***
*** @param[in]  event   Event type
***********************************************************************************************************/

void PBDP_UART_EventCB(int event)
{
    if( event & PBDP_EVENT_ERR ) {  /*-------------- PBDP Event error ------------------------------*/
        PBDP_IDLE_CNT_CLR();                                    /* Clear Counter of Received IDLE   */
        PBDP_DBG_ERR_INC();
        if( PBDP_Info.tx_buf == NULL ) {    /*------ Recving ---------------------------------------*/
            goto ERROR_RECV;
        } else if( PBDP_Info.tx_num != 0 ) {/*------ Sending ---------------------------------------*/
            PBDP_ENTER_RECV_STA();                              /* Enter the Receive status         */
            PBDP_TX_SIG_SEND(PBDP_EVENT_ERR);                   /* Set the signal flags             */
        } else {                            /*------ PreSend(Recving) ------------------------------*/
            ERROR_RECV: PBDP_RX_QUE_PUSH(PBDP_FRAME_ED);        /* Insert to Receiver queue         */
            osSemaphoreRelease(PBDP_Info.rx_sem);               /* Increase Counter of Received ED  */
        }
    }

    if( event & PBDP_EVENT_IDLE ) { /*-------------- PBDP Event Recv Idle char ---------------------*/
        PBDP_IDLE_CNT_INC();                                    /* Increase Counter of Received IDLE*/
        if( PBDP_Info.tx_buf == NULL ) {    /*------ Recving ---------------------------------------*/
            goto IDLE_RECV;
        } else if( PBDP_Info.tx_num != 0 ) {/*------ Sending ---------------------------------------*/
            PBDP_ENTER_RECV_STA();                              /* Enter the Receive status         */
//          PBDP_TX_SIG_SEND(PBDP_EVENT_IDLE);                  /* Set the signal flags             */
            PBDP_TX_SIG_SEND(PBDP_EVENT_ERR);                   /* Set the signal flags             */
        } else {                            /*------ PreSend(Recving) ------------------------------*/
            if( PBDP_Info.tx_cnt && PBDP_Info.tx_chk && (PBDP_Info.tx_chk <= PBDP_Info.idl_cnt) ) {
                PBDP_UART_EnDEN();                              /* UART RS485 DE-Pin Enable         */
                PBDP_Info.tx_num = PBDP_Info.tx_cnt;
                PBDP_Info.tx_cnt = 0;
                PBDP_Info.tx_chk = 0;
                PBDP_UART_EnTXE();                              /* UART TXE interrupt Enable        */
            }
            IDLE_RECV: if( PBDP_Info.idl_cnt == 1 ) {
                PBDP_RX_QUE_PUSH(PBDP_FRAME_ED);                /* Insert to Receiver queue         */
                osSemaphoreRelease(PBDP_Info.rx_sem);           /* Increase Counter of Received ED  */
            }
        }                                   /*------ End if( PBDP_Info.tx_buf == NULL ) ------------*/
    }

    if( event & PBDP_EVENT_CPLT ) { /*-------------- PBDP Event Send Complete ----------------------*/
//      PBDP_UART_DsDEN();                                      /* UART RS485 DE-Pin Disable        */
    }
}


/**********************************************************************************************************/
#include    <stdint.h>
#include    "stm32f4xx.h"
#include    "ProfiBUS_DP.h"

#ifdef  NDEBUG
#define PBDP_DBG_EVT_INIT()
#define PBDP_DBG_EVT_PUSH(m)
#define PBDP_DBG_ERR_PE_INC()
#define PBDP_DBG_ERR_FE_INC()
#define PBDP_DBG_ERR_NE_INC()
#define PBDP_DBG_ERR_ORE_INC()
#else
static QUEUE_TYPE(uint8_t, 1024)      g_EvtIRQ;
static uint32_t                       g_errstats[4] = {0, 0, 0, 0};
#define PBDP_DBG_EVT_INIT()         ( QUEUE_INIT(g_EvtIRQ) )
#define PBDP_DBG_EVT_PUSH(m)        ( QUEUE_PUSH(g_EvtIRQ) = m )
#define PBDP_DBG_ERR_PE_INC()       ( g_errstats[0]++ )
#define PBDP_DBG_ERR_FE_INC()       ( g_errstats[1]++ )
#define PBDP_DBG_ERR_NE_INC()       ( g_errstats[2]++ )
#define PBDP_DBG_ERR_ORE_INC()      ( g_errstats[3]++ )
#endif

#define PBDP_UART_IDEL_LED_TURN()   (  (GPIOB->ODR   & (0x1u << 1*14))  /* Turn of UART Idel Status LED */ \
                                     ? (GPIOB->BSRRH = (0x1u << 1*14))  /* PB14                         */ \
                                     : (GPIOB->BSRRL = (0x1u << 1*14))                                     \
                                    )
#define PBDP_UART_IDEL_CHK_EN()     ( TIM3->CR1 |=  TIM_CR1_CEN,        /* Enable Timer3 Counter        */ \
                                      TIM3->SR   = ~TIM_SR_UIF,                                            \
                                      TIM3->EGR |=  TIM_EGR_UG                                             \
                                    )
#define PBDP_UART_IDEL_CHK_DS()     ( TIM3->CR1 &= ~TIM_CR1_CEN,        /* Disable Timer3 Counter       */ \
                                      TIM3->SR   = ~TIM_SR_UIF                                             \
                                    )

/**
  * @brief      UART Initialize
  * @param[in]  BaudRate    UART BaudRate
  */
void PBDP_UART_Init(uint32_t BaudRate)
{
    PBDP_DBG_EVT_INIT();

    /*------------------------------------------ Init TxD(PC6) and RxD(PC7) --------------------------------*/
    MODIFY_REG(RCC->AHB1ENR,   0                                /* Enable GPIOC clock                       */
                           ,   RCC_AHB1ENR_GPIOCEN);
    MODIFY_REG(GPIOC->MODER,   (0x3u << 2*6) | (0x3u << 2*7)    /* PC6 PC7 to AF8(USART6_TX, USART6_Rx)     */
                           ,   (0x2u << 2*6) | (0x2u << 2*7));
    MODIFY_REG(GPIOC->AFR[0],  (0xFu << 4*6) | (0xFu << 4*7)
                            ,  (0x8u << 4*6) | (0x8u << 4*7));
    MODIFY_REG(GPIOC->OTYPER,  (0x1u << 1*6)                    /* PC6 Output push-pull                     */
                            ,  (0x0u << 1*6));
    MODIFY_REG(GPIOC->OSPEEDR, (0x3u << 2*6)                    /* PC6 Output High speed                    */
                             , (0x3u << 2*6));
    MODIFY_REG(GPIOC->PUPDR,   (0x3u << 2*6) | (0x3u << 2*7)    /* PC6 PC7 pull-up                          */
                           ,   (0x1u << 2*6) | (0x1u << 2*7));

    /*------------------------------------------ Init TxEN(PC9) --------------------------------------------*/
    MODIFY_REG(RCC->AHB1ENR,   0                                /* Enable GPIOC clock                       */
                           ,   RCC_AHB1ENR_GPIOCEN);
    MODIFY_REG(GPIOC->MODER,   (0x3u << 2*9)                    /* PC9 Output mode                          */
                           ,   (0x1u << 2*9));
    WRITE_REG( GPIOC->BSRRH,   (0x1u << 1*9));                  /* PC9 Output Low                           */
    MODIFY_REG(GPIOC->OTYPER,  (0x1u << 1*9)                    /* PC9 Output push-pull                     */
                            ,  (0x0u << 1*9));
    MODIFY_REG(GPIOC->OSPEEDR, (0x3u << 2*9)                    /* PC9 Output High speed                    */
                             , (0x3u << 2*9));
    MODIFY_REG(GPIOC->PUPDR,   (0x3u << 2*9)                    /* PC9 Pull-down                            */
                           ,   (0x2u << 2*9));

    /*------------------------------------------ Init Serial Interface -------------------------------------*/
    MODIFY_REG(RCC->APB2ENR,   0,  0                            /* Enable USART6 clock                      */
                             | RCC_APB2ENR_USART6EN);
    MODIFY_REG(USART6->BRR,    0xFFFFFFFF,  0
                             | __USART_BRR(HAL_RCC_GetPCLK2Freq(), BaudRate));
    MODIFY_REG(USART6->CR3,    0xFFFFFFFF,  0
                         /*  | USART_CR3_EIE        */ );       /* Error Interrupt Enable                   */
    MODIFY_REG(USART6->CR2,    0xFFFFFFFF,  0);
    MODIFY_REG(USART6->CR1,    0xFFFFFFFF,  0
                             | USART_CR1_UE                     /* USART Enable                             */
                         /*  | USART_CR1_SBK        */          /* USART Send Break                         */
                         /*  | USART_CR1_OVER8      */          /* USART Oversampling by 8 enable           */
                         /*  | USART_CR1_RWU        */          /* Receiver in active mode                  */
                         /*  | USART_CR1_WAKE       */          /* Wakeup method is Address Mark            */
                             | USART_CR1_M                      /* Word length of 9Bit                      */
                             | USART_CR1_PCE                    /* Parity Control Enable                    */
                         /*  | USART_CR1_PS         */          /* Parity Selection                         */
                         /*  | USART_CR1_PEIE       */          /* PE Interrupt Enable                      */
                             | USART_CR1_RE                     /* Receiver Enable                          */
                             | USART_CR1_TE                     /* Transmitter Enable                       */
                             | USART_CR1_TCIE                   /* Transmission Complete Interrupt Enable   */
                         /*  | USART_CR1_TXEIE      */          /* Transmission Interrupt Enable            */
                             | USART_CR1_IDLEIE                 /* IDLE Interrupt Enable                    */
                             | USART_CR1_RXNEIE);               /* RXNE Interrupt Enable                    */

    NVIC_SetPriority(USART6_IRQn, 2);                           /* Set the USART6 priority                  */
    NVIC_EnableIRQ(USART6_IRQn);                                /* Enable the USART6 global Interrupt       */

    /*------------------------------------- Init IDLE_STAT(PB14) -------------------------------------------*/
    MODIFY_REG(RCC->AHB1ENR,   0                                /* Enable GPIOB clock                       */
                           ,   RCC_AHB1ENR_GPIOBEN);
    MODIFY_REG(GPIOB->MODER,   (0x3u << 2*14)                   /* PB14 Output mode                         */
                           ,   (0x1u << 2*14));
    WRITE_REG( GPIOB->BSRRL,   (0x1u << 1*14));                 /* PB14 Output High                         */
    MODIFY_REG(GPIOB->OTYPER,  (0x1u << 1*14)                   /* PB14 Output push-pull                    */
                            ,  (0x0u << 1*14));
    MODIFY_REG(GPIOB->OSPEEDR, (0x3u << 2*14)                   /* PB14 Output High speed                   */
                             , (0x3u << 2*14));
    MODIFY_REG(GPIOB->PUPDR,   (0x3u << 2*14)                   /* PB14 No pull-up, pull-down               */
                           ,   (0x0u << 2*14));

    /*------------------------------------------ Init TIM3_CH3(PC8) ----------------------------------------*/
    MODIFY_REG(RCC->AHB1ENR,   0                                /* Enable GPIOC clock                       */
                           ,   RCC_AHB1ENR_GPIOCEN);
    MODIFY_REG(GPIOC->MODER,   (0x3u << 2*(8))                  /* PC8 to AF2(TIM3_CH3)                     */
                           ,   (0x2u << 2*(8)));
    MODIFY_REG(GPIOC->AFR[1],  (0xFu << 4*(8-8))
                            ,  (0x2u << 4*(8-8)));
    MODIFY_REG(GPIOC->PUPDR,   (0x3u << 2*(8))                  /* PC8 Pull-Up                              */
                           ,   (0x1u << 2*(8)));

    /*------------------------------------------ Init Timer 3 ----------------------------------------------*/
    MODIFY_REG(RCC->APB1ENR,   0                                /* Enable Timer 3 clock                     */
                           ,   RCC_APB1ENR_TIM3EN);
    MODIFY_REG(TIM3->ARR,      0xFFFF                           /* Auto-reload value                        */
                        ,      (((HAL_RCC_GetPCLK1Freq() * (11 + 1) + (BaudRate / 2)) / BaudRate) - 1));
    MODIFY_REG(TIM3->PSC,      0xFFFF
                        ,      1);                              /* Prescaler value                          */
    MODIFY_REG(TIM3->CR1,      0xFFFF, 0
                          /*   TIM_CR1_CKD   */                 /* Clock division: 00                       */
                          /* | TIM_CR1_ARPE  */                 /* Auto-reload preload disable              */
                          /* | TIM_CR1_CMS   */                 /* Edge-aligned mode                        */
                          /* | TIM_CR1_DIR   */                 /* Counter used as upcounter                */
                          /* | TIM_CR1_OPM   */                 /* Counter not stopped at update event      */
                             | TIM_CR1_URS                      /* Only counter overflow/underflow          */
                          /* | TIM_CR1_UDIS  */                 /* UEV enabled                              */
                          /* | TIM_CR1_CEN   */ );              /* Counter disabled                         */
    MODIFY_REG(TIM3->CR2,      0xFFFF, 0
                             | TIM_CR2_TI1S     );              /* TIMx_CH1 pin is connected to TI1 input   */
    MODIFY_REG(TIM3->SMCR,     0xFFFF, 0
                          /* | TIM_SMCR_ETP  */                 /* active at high level or rising edge      */
                          /* | TIM_SMCR_ECE  */                 /* External clock mode 2 disabled           */
                          /* | TIM_SMCR_ETPS */                 /* External trigger prescaler: OFF          */
                          /* | TIM_SMCR_ETF  */                 /* External trigger filter: No filter       */
                          /* | TIM_SMCR_MSM  */                 /* Master/Slave mode: No action             */
                             | (0x5u << 4)                      /* TS: Filtered Timer Input 1 (TI1FP1)      */
                             | (0x4u << 0)      );              /* SMS: Reset Mode                          */
    MODIFY_REG(TIM3->CCMR1,     0xFFFF, 0
                             | (0x0u << 4)                      /* IC1F: No filter                          */
                             | (0x0u << 2)                      /* IC1PSC: no prescaler                     */
                             | (0x1u << 0)      );              /* CC1S: Input channel, IC1 is mapped on TI1*/
    MODIFY_REG(TIM3->CCER,     0xFFFF, 0
                             | TIM_CCER_CC1NP                   /* 11: noninverted/both edges               */
                             | TIM_CCER_CC1P                    /* 11: noninverted/both edges               */
                          /* | TIM_CCER_CC1E */ );              /* Capture disabled                         */
    MODIFY_REG(TIM3->DIER,     0xFFFF, 0
                          /* | TIM_DIER_TIE  */                 /* Disable Trigger interrupt                */
                             | TIM_DIER_UIE);                   /* Enable Update interrupt                  */
    MODIFY_REG(TIM3->CR1,      0
                        ,      TIM_CR1_CEN);                    /* Counter enable                           */

    NVIC_SetPriority(TIM3_IRQn, 2);                             /* Set the Timer 3 priority                 */
    NVIC_EnableIRQ(TIM3_IRQn);                                  /* Enable the Timer 3 global Interrupt      */
}

/**
  * @brief  UART RS485 DE-Pin Enable
  */
void PBDP_UART_EnDEN(void)
{
    WRITE_REG(GPIOC->BSRRL, (0x1u << 1*9));     /* PC9 Output High                                  */
    PBDP_UART_IDEL_CHK_DS();
}

/**
  * @brief  UART RS485 DE-Pin Disable
  */
void PBDP_UART_DsDEN(void)
{
    WRITE_REG(GPIOC->BSRRH, (0x1u << 1*9));     /* PC9 Output Low                                   */
}

/**
  * @brief  UART Transmit Data Register empty interrupt Enable
  */
void PBDP_UART_EnTXE(void)
{
    SET_BIT(USART6->CR1, USART_CR1_TXEIE);      /* Enable Transmit Data Register empty interrupt    */
}

/**
  * @brief  UART Transmit Data Register empty interrupt Disable
  */
void PBDP_UART_DsTXE(void)
{
    CLEAR_BIT(USART6->CR1, USART_CR1_TXEIE);    /* Disable Transmit Data Register empty interrupt   */
}

/**
  * @brief  USART Interrupt Request Enable
  */
void PBDP_UART_EnIRQ(void)
{
    NVIC_EnableIRQ(TIM3_IRQn);                  /* Enable the Timer 3 global Interrupt              */
    NVIC_EnableIRQ(USART6_IRQn);                /* Enable the USART6 global Interrupt               */
}

/**
  * @brief  UART Interrupt Request Disable
  */
void PBDP_UART_DsIRQ(void)
{
    NVIC_DisableIRQ(TIM3_IRQn);                 /* Disable the Timer 3 global Interrupt             */
    NVIC_DisableIRQ(USART6_IRQn);               /* Disable the USART6 global Interrupt              */
}

/**
  * @brief  USART interrupt handles function
  */
void USART6_IRQHandler(void)
{
    uint32_t    tmp1;
    int32_t     tmp3;

    if( ((tmp3 = 0, tmp1 = USART6->SR) & USART_SR_RXNE) && (USART6->CR1 & USART_CR1_RXNEIE) ) {
        PBDP_UART_IDEL_CHK_DS();
        if( tmp1 & USART_SR_PE ) {              /* USART parity error interrupt occurred    */
            PBDP_DBG_EVT_PUSH(1);
            PBDP_DBG_ERR_PE_INC();
            PBDP_UART_EventCB(PBDP_EVENT_ERR);
            tmp3 = USART6->SR;  tmp3 = USART6->DR;  // Clear flag
            tmp3 = 1;
        }
        if( tmp1 & USART_SR_FE ) {              /* USART frame error interrupt occurred     */
            PBDP_DBG_EVT_PUSH(2);
            PBDP_DBG_ERR_FE_INC();
            PBDP_UART_EventCB(PBDP_EVENT_ERR);
            tmp3 = USART6->SR;  tmp3 = USART6->DR;  // Clear flag
            tmp3 = 1;
        }
        if( tmp1 & USART_SR_NE ) {              /* USART noise error interrupt occurred     */
            PBDP_DBG_EVT_PUSH(3);
            PBDP_DBG_ERR_NE_INC();
            PBDP_UART_EventCB(PBDP_EVENT_ERR);
            tmp3 = USART6->SR;  tmp3 = USART6->DR;  // Clear flag
            tmp3 = 1;
        }
        if( tmp1 & USART_SR_ORE ) {             /* USART Over-Run error interrupt occurred  */
            PBDP_DBG_EVT_PUSH(4);
            PBDP_DBG_ERR_ORE_INC();
            PBDP_UART_EventCB(PBDP_EVENT_ERR);
            tmp3 = USART6->SR;  tmp3 = USART6->DR;  // Clear flag
            tmp3 = 1;
        }
        if( tmp3 == 0 ) {                       /* USART Receive data register not empty    */
            PBDP_DBG_EVT_PUSH(8);
            PBDP_UART_RecvCB(USART6->DR);
        }
    }

    if( (tmp1 & USART_SR_TXE) && (USART6->CR1 & USART_CR1_TXEIE) ) {
        PBDP_UART_IDEL_CHK_DS();                /* USART Transmit data register empty       */
        PBDP_DBG_EVT_PUSH(7);
        if( ((tmp3 = PBDP_UART_SendCB()) & (~0xFFu)) == 0 ) { USART6->DR = tmp3 & 0xFF; }
    }

    if( (tmp1 & USART_SR_TC) && (USART6->CR1 & USART_CR1_TCIE) ) {
      //PBDP_UART_IDEL_CHK_EN();                /* USART Transmission complete              */
        PBDP_DBG_EVT_PUSH(6);
        PBDP_UART_EventCB(PBDP_EVENT_CPLT);
        WRITE_REG(USART6->SR, ~USART_SR_TC);    // Clear flag
    }

    if( (tmp1 & USART_SR_IDLE) && (USART6->CR1 & USART_CR1_IDLEIE) ) {
        PBDP_UART_IDEL_CHK_EN();                /* USART line IDLE occurred    (Clear flag) */
        PBDP_UART_IDEL_LED_TURN();
        PBDP_DBG_EVT_PUSH(5);
        PBDP_UART_EventCB(PBDP_EVENT_IDLE);
        tmp3 = USART6->SR;  tmp3 = USART6->DR;  // Clear flag
    }
}

/**
  * @brief  Timer interrupt handles function
  */
void TIM3_IRQHandler(void)
{
    if( READ_BIT(TIM3->SR, TIM_SR_UIF) && READ_BIT(TIM3->DIER, TIM_DIER_UIE) ) {
        if( READ_BIT(GPIOC->IDR, 0x1u << (1*8)) ) {
            PBDP_UART_IDEL_LED_TURN();      // TIM3_CH3(PC8)
            PBDP_DBG_EVT_PUSH(9);
            PBDP_UART_EventCB(PBDP_EVENT_IDLE);
        }
        WRITE_REG(TIM3->SR, ~TIM_SR_UIF);   // Clear interrupt flag
    }
}

/*****************************  END OF FILE  **************************************************************/
/** @}
*** @}
*** @}
***********************************************************************************************************/

