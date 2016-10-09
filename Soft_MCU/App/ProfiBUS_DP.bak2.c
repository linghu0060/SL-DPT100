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

typedef struct {    /*------------- PBDP Information (Run-Time) ----------------------------*/
    osThreadId                              tx_sig;     /* OS Signal flags of transmit      */
    osMutexId                               tx_mut;     /* OS Mutex of transmit             */
    const uint8_t                          *tx_buf;     /* Buffer of transmit               */
    uint16_t                                tx_num;     /* Total number of transmit buffer  */
    uint16_t                                tx_cnt;     /* Number of data transmitted       */
    uint16_t                                tx_chk;     /* Number of data check             */

    osSemaphoreId                           rx_sem;     /* OS Semaphore of Received ED      */
    osMutexId                               rx_mut;     /* OS Mutex of Receive              */
    QUEUE_TYPE(uint8_t, PBDP_RX_BUF_LEN)    rx_que;     /* Receive Data Circular Queue      */

    uint32_t                                idl_cnt;    /* Counter of sequential idle char  */
    uint32_t                                rxd_cnt;    /* Counter of received data         */
    uint32_t                                txd_cnt;    /* Counter of transmit data         */
    uint32_t                                rxf_cnt;    /* Counter of received frame        */
    uint32_t                                txf_cnt;    /* Counter of transmit frame        */
    uint32_t                                err_ovr;    /* Counter of Queue Overrun error   */
    uint32_t                                err_chk;    /* Counter of transmit check error  */
} PBDP_INFO;

#define PBDP_IDLE_CNT_CLR()     if( PBDP_Info.tx_sig ) osSignalClear(PBDP_Info.tx_sig, PBDP_EVENT_IDLE);  PBDP_Info.idl_cnt = 0

#define PBDP_DBG_RXD_INIT()     ( PBDP_Info.rxd_cnt = 0 )
#define PBDP_DBG_RXD_INC()      ( PBDP_Info.rxd_cnt++   )
#define PBDP_DBG_TXD_INIT()     ( PBDP_Info.txd_cnt = 0 )
#define PBDP_DBG_TXD_INC()      ( PBDP_Info.txd_cnt++   )
#define PBDP_DBG_RXF_INIT()     ( PBDP_Info.rxf_cnt = 0 )
#define PBDP_DBG_RXF_INC()      ( PBDP_Info.rxf_cnt++   )
#define PBDP_DBG_TXF_INIT()     ( PBDP_Info.txf_cnt = 0 )
#define PBDP_DBG_TXF_INC()      ( PBDP_Info.txf_cnt++   )
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

int PBDP_Statc(char* buff, int size)
{
    int n, m;

    n = snprintf( buff, size,
                  "ProfiBUS DP Rx: %u(Frame) %u(Byte)\r\n"
                  "ProfiBUS DP Tx: %u(Frame) %u(Byte)\r\n"
                  "ProfiBUS DP Over Run ERR: %u\r\n"
                  "ProfiBUS DP Check(Tx) ERR: %u\r\n",
                  PBDP_Info.rxf_cnt, PBDP_Info.rxd_cnt,
                  PBDP_Info.txf_cnt, PBDP_Info.txd_cnt,
                  PBDP_Info.err_ovr,
                  PBDP_Info.err_chk
                );
    if( n < 0 )  { /*******/ return( n );        }
    if( n >= size )  { /***/ return( size - 1 ); }

    m = PBDP_UART_Statc(&(buff[n]), size - n);
    if( m < 0 )  { /*******/ return( n );        }
    if( m >= (size - n) )  { return( size - 1 ); }
    else  { /**************/ return( m + n );    }
}


/**********************************************************************************************************/
/** @brief      PorfiBUS_DP Initialize
***
*** @param[in]  config  A pointer of band rate.
***********************************************************************************************************/

void PBDP_Init(const char* config)
{
    (void)config;

    PBDP_Info.idl_cnt = 0;      /* PBDP Information Initialize  */
    PBDP_Info.tx_sig  = NULL;
    PBDP_Info.tx_mut  = osMutexCreate(osMutex(PBDP_tx_mut));
    PBDP_Info.tx_buf  = NULL;
    PBDP_Info.tx_num  = 0;
    PBDP_Info.tx_cnt  = 0;
    PBDP_Info.tx_chk  = 0;
    PBDP_Info.rx_sem  = osSemaphoreCreate(osSemaphore(PBDP_rx_sem), 0);
    PBDP_Info.rx_mut  = osMutexCreate(osMutex(PBDP_rx_mut));
    QUEUE_INIT(PBDP_Info.rx_que);
    PBDP_DBG_RXD_INIT();
    PBDP_DBG_TXD_INIT();
    PBDP_DBG_RXF_INIT();
    PBDP_DBG_TXF_INIT();
    PBDP_DBG_OVR_INIT();
    PBDP_DBG_CHK_INIT();

    if( (PBDP_Info.tx_mut == NULL) || (PBDP_Info.rx_sem == NULL) || (PBDP_Info.rx_mut == NULL) ) {
        printf("[ProfiBUS DP] Initialize Failed!\r\n");
    }

    PBDP_UART_Init(187500);     /* UART Initialize              */
    printf("[ProfiBUS DP] Initialize Succeed! Baud rate: 19200.\r\n");
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

    if( osOK != osMutexWait(PBDP_Info.rx_mut, osWaitForever) ) { return( -1 ); }/* osMutexWait Error        */
    if( osSemaphoreWait(PBDP_Info.rx_sem, osWaitForever) <= 0) { GOTO_RET(-2); }/* Waiting for Received ED  */

    while(1) {
        PBDP_UART_DsIntr();

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

        default:   RECV_DEL:    /*--------------------- End_Delimiter and Other Character ------------------*/
            QUEUE_DEL(PBDP_Info.rx_que, 1); /*****************/  break;         /* Queue Delete             */
//          if( PBDP_FRAME_ED != QUEUE_POP(PBDP_Info.rx_que) ) {                /* Queue Delete             */
//              printf(   "[ProfiBUS DP] Recv invalid data: "
//                        "0x%02X 0x%02X 0x%02X 0x%02X *0x%02X* 0x%02X 0x%02X 0x%02X 0x%02X\r\n"
//                      , (int)(QUEUE_GET(PBDP_Info.rx_que, -5) & 0xFF)
//                      , (int)(QUEUE_GET(PBDP_Info.rx_que, -4) & 0xFF)
//                      , (int)(QUEUE_GET(PBDP_Info.rx_que, -3) & 0xFF)
//                      , (int)(QUEUE_GET(PBDP_Info.rx_que, -2) & 0xFF)
//                      , (int)(QUEUE_GET(PBDP_Info.rx_que, -1) & 0xFF)
//                      , (int)(QUEUE_GET(PBDP_Info.rx_que,  0) & 0xFF)
//                      , (int)(QUEUE_GET(PBDP_Info.rx_que,  1) & 0xFF)
//                      , (int)(QUEUE_GET(PBDP_Info.rx_que,  2) & 0xFF)
//                      , (int)(QUEUE_GET(PBDP_Info.rx_que,  3) & 0xFF)
//                    );
//          }
//          break;
        }   /* End switch   */
        PBDP_UART_EnIntr();
    }       /* End while(1) */

  RECV_RET:
    PBDP_UART_EnIntr();
    if( result > 0 )  { PBDP_DBG_RXF_INC(); }
    if( osOK != osMutexRelease(PBDP_Info.rx_mut) ) { return( -1 ); }            /* osMutexRelease Error     */
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
static  QUEUE_TYPE(uint8_t, 1024)     g_EvtIRQ;
int PBDP_Send(const uint8_t *buff, int len)
{
    int         cnt;
    osEvent     evt;        uint32_t    idle[2];

    switch( buff[0] ) {
    case PBDP_FRAME_SD1:  cnt = (buff[3] & 0x40) ? (3) : (1);   break;
    case PBDP_FRAME_SD2:  cnt = (buff[6] & 0x40) ? (3) : (1);   break;
    case PBDP_FRAME_SD3:  cnt = (buff[3] & 0x40) ? (3) : (1);   break;
    case PBDP_FRAME_SD4:  cnt = 3; /************************/   break;
    case PBDP_FRAME_SC:   cnt = 1; /************************/   break;
    default: /**********************************************/   return( -1 );
    }

    if( osOK != osMutexWait(PBDP_Info.tx_mut, osWaitForever) )  return( -2 ); /* osMutexWait Error  */
    PBDP_Info.tx_sig = osThreadGetId();
    {
//      osSignalClear(PBDP_Info.tx_sig, PBDP_EVENT_IDLE);
        while( 1 ) {
            if( osEventSignal != (evt = osSignalWait(0, osWaitForever)).status )  continue;
            if( PBDP_EVENT_IDLE != evt.value.signals )  continue;   /* Waiting for PBDP_EVENT_IDLE  */
                                idle[0] = PBDP_Info.idl_cnt;
            if( PBDP_Info.idl_cnt < cnt )  continue;
            break;
        }
        PBDP_Info.tx_buf = buff;                                    /* Pointer to transmit buffer   */
        PBDP_Info.tx_num = len;                                     /* Total  of transmit buffer    */
        PBDP_Info.tx_cnt = 0;                                       /* Number of data transmitted   */
        PBDP_Info.tx_chk = 0;                                       /* Number of data check         */
        PBDP_UART_EnDE();                                           /* Enable PIN of UART RS485_DE  */
                                idle[1] = PBDP_Info.idl_cnt;
//      osSignalClear(PBDP_Info.tx_sig, PBDP_EVENT_IDLE);
        PBDP_UART_EnTXE();                                          /* Enable interrupt of UART TXE */
//      osSignalClear(PBDP_Info.tx_sig, PBDP_EVENT_IDLE);
        evt = osSignalWait(0, osWaitForever);                       /* Waiting for UART Event       */
        PBDP_UART_DsTXE();                                          /* Disable interrupt of UART TXE*/
        PBDP_UART_DsDE();                                           /* Disable PIN of UART RS485_DE */
        PBDP_Info.tx_chk = 0;
        PBDP_Info.tx_cnt = 0;
        PBDP_Info.tx_num = 0;
        PBDP_Info.tx_buf = NULL;
    }
    PBDP_Info.tx_sig = NULL;
    if( osOK != osMutexRelease(PBDP_Info.tx_mut) )  return( -3 );   /* osMutexRelease Error         */

    if( osEventSignal != evt.status )/***********/  return( -4 );   /* osSignalWait Error           */
//  if( evt.value.signals & PBDP_EVENT_ERR )/****/  return( -5 );
    if( evt.value.signals & PBDP_EVENT_ERR )/****/ {
        printf(   "[ProfiBUS DP] Send invalid data: "
                  "*0x%04X* 0x%02X 0x%02X 0x%02X 0x%02X 0x%02X 0x%02X 0x%02X 0x%02X\r\n"
                  "              idl_cnt1: %u, idl_cnt2: %u\r\n"
                  "              buff: 0x%02X 0x%02X 0x%02X 0x%02X 0x%02X 0x%02X 0x%02X\r\n"
                , (int)(g_EvtIRQ.head % 1024)
                , (int)(g_EvtIRQ.elem[ (g_EvtIRQ.head + (-8)) % __Q_LEN(g_EvtIRQ) ] & 0xFF)
                , (int)(g_EvtIRQ.elem[ (g_EvtIRQ.head + (-7)) % __Q_LEN(g_EvtIRQ) ] & 0xFF)
                , (int)(g_EvtIRQ.elem[ (g_EvtIRQ.head + (-6)) % __Q_LEN(g_EvtIRQ) ] & 0xFF)
                , (int)(g_EvtIRQ.elem[ (g_EvtIRQ.head + (-5)) % __Q_LEN(g_EvtIRQ) ] & 0xFF)
                , (int)(g_EvtIRQ.elem[ (g_EvtIRQ.head + (-4)) % __Q_LEN(g_EvtIRQ) ] & 0xFF)
                , (int)(g_EvtIRQ.elem[ (g_EvtIRQ.head + (-3)) % __Q_LEN(g_EvtIRQ) ] & 0xFF)
                , (int)(g_EvtIRQ.elem[ (g_EvtIRQ.head + (-2)) % __Q_LEN(g_EvtIRQ) ] & 0xFF)
                , (int)(g_EvtIRQ.elem[ (g_EvtIRQ.head + (-1)) % __Q_LEN(g_EvtIRQ) ] & 0xFF)
                , idle[0], idle[1]
                , (int)buff[0], (int)buff[1], (int)buff[2], (int)buff[3], (int)buff[4], (int)buff[5], (int)buff[6]
              );
        return( -5 );
    }

    if( evt.value.signals & PBDP_EVENT_IDLE )/***/  return( -6 );
    if( evt.value.signals != PBDP_EVENT_CPLT )/**/  return( -7 );
    PBDP_DBG_TXF_INC();
    return( len );
}


/**********************************************************************************************************/
/** @brief       UART callbacks of Receive data register not empty
***
*** @param[in]   ch     UART Receive data
***********************************************************************************************************/

void PBDP_UART_RecvCB(int ch)
{
    PBDP_IDLE_CNT_CLR();    /*-------------------------- Clear Counter of Received IDLE ----------------*/

    if( PBDP_Info.tx_buf == NULL ) {    /*-------------- PBDP is Recv status ---------------------------*/
        if(!QUEUE_FULL(PBDP_Info.rx_que) ) {
            QUEUE_PUSH(PBDP_Info.rx_que) = (ch & 0xFF);         /* Insert to Receive queue              */
        }
        else { PBDP_DBG_OVR_INC(); }
        if( PBDP_FRAME_ED == (ch & 0xFF) ) {
            osSemaphoreRelease(PBDP_Info.rx_sem);               /* Increase count of Received ED        */
        }
        PBDP_DBG_RXD_INC();
    }
    else {  /*------------------------------------------ PBDP is Send status ---------------------------*/
        if( (ch & 0xFF) != PBDP_Info.tx_buf[PBDP_Info.tx_chk++] ) {
            if( PBDP_Info.tx_sig ) {
                osSignalSet(PBDP_Info.tx_sig, PBDP_EVENT_ERR);  /* Transmit Data check the error        */
            }
            PBDP_DBG_CHK_INC();
        }
        if( PBDP_Info.tx_chk == PBDP_Info.tx_num ) {
            if( PBDP_Info.tx_sig ) {
                osSignalSet(PBDP_Info.tx_sig, PBDP_EVENT_CPLT); /* Transmission complete                */
            }
        }
    }
}


/**********************************************************************************************************/
/** @brief      UART callbacks of Transmit data register empty
***
*** @return     (>= 0)Transmit Data, (< 0)No Transmit Data or Error.
***********************************************************************************************************/

int PBDP_UART_SendCB(void)
{
    PBDP_IDLE_CNT_CLR();    /*-------------------------- Clear Counter of Received IDLE ----------------*/

    if( PBDP_Info.tx_buf == NULL ) {    /*-------------- PBDP is Recv status ---------------------------*/
        PBDP_UART_DsTXE();                                      /* Disable interrupt of UART TXE        */
        return( -2 );                                           /* Return Error                         */
    }
    else {  /*------------------------------------------ PBDP is Send status ---------------------------*/
        if( PBDP_Info.tx_cnt < PBDP_Info.tx_num ) {
            PBDP_DBG_TXD_INC();
            return( PBDP_Info.tx_buf[PBDP_Info.tx_cnt++] );     /* Return the Send data                 */
        }
        PBDP_UART_DsTXE();                                      /* Disable interrupt of UART TXE        */
        return( -1 );                                           /* Return no Send data                  */
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
        if( PBDP_Info.tx_sig ) {
            osSignalSet(PBDP_Info.tx_sig, PBDP_EVENT_ERR);      /* Set the signal flags             */
        }
        if( PBDP_Info.tx_buf == NULL ) {
            if(!QUEUE_FULL(PBDP_Info.rx_que) ) {
                QUEUE_PUSH(PBDP_Info.rx_que)  =  PBDP_FRAME_ED; /* Insert to Receive queue          */
            }
            else { PBDP_DBG_OVR_INC(); }
            osSemaphoreRelease(PBDP_Info.rx_sem);               /* Increase Counter of Received ED  */
        }
        PBDP_IDLE_CNT_CLR();                                    /* Clear Counter of Received IDLE   */
    }

    if( event & PBDP_EVENT_IDLE ) { /*-------------- PBDP Event Recv Idle char ---------------------*/
        if( PBDP_Info.tx_sig ) {
            osSignalSet(PBDP_Info.tx_sig, PBDP_EVENT_IDLE);     /* Set the signal flags             */
        }
        if( (PBDP_Info.tx_buf == NULL) && (PBDP_Info.idl_cnt == 0) ) {
            if(!QUEUE_FULL(PBDP_Info.rx_que) ) {
                QUEUE_PUSH(PBDP_Info.rx_que)  =  PBDP_FRAME_ED; /* Insert to Receive queue          */
            }
            else { PBDP_DBG_OVR_INC(); }
            osSemaphoreRelease(PBDP_Info.rx_sem);               /* Increase Counter of Received ED  */
        }
        if( PBDP_Info.idl_cnt < 65536 ) { PBDP_Info.idl_cnt++; }/* Increase Counter of Received IDLE*/
    }

    if( event & PBDP_EVENT_CPLT ) { /*-------------- PBDP Event Send Complete ----------------------*/
        PBDP_UART_DsDE();                                       /* Disable PIN of UART RS485_DE     */
      //PBDP_IDLE_CNT_CLR();                                    /* Clear Counter of Received IDLE   */
    }
}


/**********************************************************************************************************/
#include    <stdint.h>
#include    "stm32f4xx.h"
#include    "ProfiBUS_DP.h"

#define _hUART                            (&UART_Handle)
static struct{ USART_TypeDef *Instance; }   UART_Handle;              /* UART handle (Run-Time)             */
static uint32_t                             g_statistic[4];           /* Counter of UART driver error       */

#ifdef  NDEBUG
#define PBDP_DBG_EVT_INIT()
#define PBDP_DBG_EVT_PUSH(m)
#else
// static  QUEUE_TYPE(uint8_t, 1024)     g_EvtIRQ;
#define PBDP_DBG_EVT_INIT()         ( QUEUE_INIT(g_EvtIRQ) )
#define PBDP_DBG_EVT_PUSH(m)        ( QUEUE_PUSH(g_EvtIRQ) = m )
#endif

#define PBDP_DBG_ERR_PE_INIT()      ( g_statistic[0] = 0 )
#define PBDP_DBG_ERR_PE_INC()       ( g_statistic[0]++   )
#define PBDP_DBG_ERR_FE_INIT()      ( g_statistic[1] = 0 )
#define PBDP_DBG_ERR_FE_INC()       ( g_statistic[1]++   )
#define PBDP_DBG_ERR_NE_INIT()      ( g_statistic[2] = 0 )
#define PBDP_DBG_ERR_NE_INC()       ( g_statistic[2]++   )
#define PBPD_DBG_ERR_ORE_INIT()     ( g_statistic[3] = 0 )
#define PBDP_DBG_ERR_ORE_INC()      ( g_statistic[3]++   )

#define PBDP_UART_IDEL_CHK_START()  ( TIM3->SR   = ~TIM_SR_UIF,       /* Timer Update interrupt flag clear  */ \
                                      TIM3->CNT  =  0,                /* Timer Counter clear                */ \
                                      TIM3->CR1 |= TIM_CR1_CEN,       /* Timer Counter enable               */ \
                                      NVIC_ClearPendingIRQ(TIM3_IRQn) /* Timer Pending Interrupt clear      */ \
                                    )
#define PBDP_UART_IDEL_CHK_STOP()   ( TIM3->SR   = ~TIM_SR_UIF,       /* Timer Update interrupt flag clear  */ \
                                      TIM3->CR1 &= ~TIM_CR1_CEN,      /* Timer Counter disable              */ \
                                      NVIC_ClearPendingIRQ(TIM3_IRQn) /* Timer Pending Interrupt clear      */ \
                                    )

/**
  * @brief      Get UART driver statistic information
  * @param[out] buff    Output statistic information string
  * @param[in]  size    size of buff(in bytes)
  * @return     (< 0)Error. (>= size)string was truncated. (other)number of output char
  */
int PBDP_UART_Statc(char* buff, int size)
{
    return( snprintf( buff, size,
                      "USART Parity ERR: %u\r\n"
                      "USART Frame ERR: %u\r\n"
                      "USART Noise ERR: %u\r\n"
                      "USART Over ERR: %u\r\n",
                      g_statistic[0],
                      g_statistic[1],
                      g_statistic[2],
                      g_statistic[3]
                    )
          );
}

/**
  * @brief      UART Initialize
  * @param[in]  BaudRate    UART BaudRate
  */
void PBDP_UART_Init(uint32_t BaudRate)
{
    /*------------------------------------------ Init TxD(PC6) and RxD(PC7) --------------------------------*/
    MODIFY_REG(RCC->AHB1ENR,   RCC_AHB1ENR_GPIOCEN              /* Enable GPIOC clock                       */
                           ,   RCC_AHB1ENR_GPIOCEN);
    MODIFY_REG(GPIOC->MODER,   (0x3u << 2*6) | (0x3u << 2*7)    /* PC6 PC7 to AF8                           */
                           ,   (0x2u << 2*6) | (0x2u << 2*7));
    MODIFY_REG(GPIOC->AFR[0],  (0xFu << 4*6) | (0xFu << 4*7)
                            ,  (0x8u << 4*6) | (0x8u << 4*7));
    MODIFY_REG(GPIOC->OTYPER,  (0x1u << 1*6)                    /* PC6 Output push-pull                     */
                            ,  (0x0u << 1*6));
    MODIFY_REG(GPIOC->OSPEEDR, (0x3u << 2*6)                    /* PC6 Output High speed                    */
                             , (0x3u << 2*6));
    MODIFY_REG(GPIOC->PUPDR,   (0x3u << 2*6) | (0x3u << 2*7)    /* PC6 PC7 pull-up                          */
                           ,   (0x1u << 2*6) | (0x1u << 2*7));

    /*------------------------------------------ Init TxEN(PD7) --------------------------------------------*/
    MODIFY_REG(RCC->AHB1ENR,   RCC_AHB1ENR_GPIODEN              /* Enable GPIOD clock                       */
                           ,   RCC_AHB1ENR_GPIODEN);
    MODIFY_REG(GPIOD->MODER,   (0x3u << 2*7)                    /* PD7 Output mode                          */
                           ,   (0x1u << 2*7));
    WRITE_REG( GPIOD->BSRRH,   (0x1u << 1*7));                  /* PD7 Output Low                           */
    MODIFY_REG(GPIOD->OTYPER,  (0x1u << 1*7)                    /* PD7 Output push-pull                     */
                            ,  (0x0u << 1*7));
    MODIFY_REG(GPIOD->OSPEEDR, (0x3u << 2*7)                    /* PD7 Output High speed                    */
                             , (0x3u << 2*7));
    MODIFY_REG(GPIOD->PUPDR,   (0x3u << 2*7)                    /* PD7 Pull-down                            */
                           ,   (0x2u << 2*7));

    /*------------------------------------------ Init Serial Interface -------------------------------------*/
    MODIFY_REG(RCC->APB2ENR,   RCC_APB2ENR_USART6EN             /* Enable USART6 clock                      */
                           ,   RCC_APB2ENR_USART6EN);
    MODIFY_REG(USART6->BRR,    0xFFFFFFFF
                          ,    __USART_BRR(HAL_RCC_GetPCLK2Freq(), BaudRate));
    MODIFY_REG(USART6->CR3,    0xFFFFFFFF
                          ,    USART_CR3_EIE);                  /* Error Interrupt Enable                   */
    MODIFY_REG(USART6->CR2,    0xFFFFFFFF
                          ,    0);
    MODIFY_REG(USART6->CR1,    0xFFFFFFFF
                          ,    USART_CR1_UE                     /* USART Enable                             */
                         /*  | USART_CR1_SBK        */          /* USART Send Break                         */
                         /*  | USART_CR1_OVER8      */          /* USART Oversampling by 8 enable           */
                         /*  | USART_CR1_RWU        */          /* Receiver in active mode                  */
                         /*  | USART_CR1_WAKE       */          /* Wakeup method is Address Mark            */
                             | USART_CR1_M                      /* Word length of 9Bit                      */
                             | USART_CR1_PCE                    /* Parity Control Enable                    */
                         /*  | USART_CR1_PS         */          /* Parity Selection                         */
                             | USART_CR1_PEIE                   /* PE Interrupt Enable                      */
                             | USART_CR1_RE                     /* Receiver Enable                          */
                             | USART_CR1_TE                     /* Transmitter Enable                       */
                             | USART_CR1_TCIE                   /* Transmission Complete Interrupt Enable   */
                         /*  | USART_CR1_TXEIE      */          /* Transmission Interrupt Enable            */
                             | USART_CR1_IDLEIE                 /* IDLE Interrupt Enable                    */
                             | USART_CR1_RXNEIE);               /* RXNE Interrupt Enable                    */

    PBDP_DBG_EVT_INIT();
    PBDP_DBG_ERR_PE_INIT();
    PBDP_DBG_ERR_FE_INIT();
    PBDP_DBG_ERR_NE_INIT();
    PBPD_DBG_ERR_ORE_INIT();
    _hUART->Instance = USART6;
    NVIC_SetPriority(USART6_IRQn, 2);                           /* Set the USART6 priority                  */
    NVIC_EnableIRQ(USART6_IRQn);                                /* Enable the USART6 global Interrupt       */

    /*------------------------------------------ Init Timer 3 ----------------------------------------------*/
    MODIFY_REG(RCC->APB1ENR,   RCC_APB1ENR_TIM3EN               /* Enable Timer 3 clock                     */
                           ,   RCC_APB1ENR_TIM3EN);
    MODIFY_REG(TIM3->CR1,      TIM_CR1_DIR | TIM_CR1_CMS | TIM_CR1_CKD
                        ,      0                                /* Edge-aligned mode                        */
                             | 0                                /* Counter used as upcounter                */
                             | 0);                              /* Clock division                           */
    MODIFY_REG(TIM3->ARR,      0xFFFFFFFF                       /* Auto-reload value                        */
                        ,      (((HAL_RCC_GetPCLK1Freq() * 11 + (BaudRate / 2)) / BaudRate) - 1));
    MODIFY_REG(TIM3->PSC,      0xFFFFFFFF
                        ,      1);                              /* Prescaler value                          */
    MODIFY_REG(TIM3->EGR,      TIM_EGR_UG
                        ,      TIM_EGR_UG);                     /* Update generation                        */
  //MODIFY_REG(TIM3->SR,       TIM_SR_UIF, 0);                  /* Update interrupt flag clear              */
    MODIFY_REG(TIM3->DIER,     TIM_DIER_UIE
                         ,     TIM_DIER_UIE);                   /* Enable Update interrupt                  */
    MODIFY_REG(TIM3->CR1,      TIM_CR1_CEN
                        ,      TIM_CR1_CEN);                    /* Counter enable                           */

    NVIC_SetPriority(TIM3_IRQn, 2);                             /* Set the Timer 3 priority                 */
    NVIC_EnableIRQ(TIM3_IRQn);                                  /* Enable the Timer 3 global Interrupt      */
}

/**
  * @brief  UART Transceiver(RS485) DE Enable
  */
void PBDP_UART_EnDE(void)
{
//  NVIC_DisableIRQ(TIM3_IRQn);                         /* Disable the Timer 3 global Interrupt             */
//  NVIC_DisableIRQ(USART6_IRQn);                       /* Disable the USART6 global Interrupt              */
    WRITE_REG(GPIOD->BSRRL, (0x1u << 1*7));             /* PD7 Output High                                  */
}

/**
  * @brief  UART Transceiver(RS485) DE Disable
  */
void PBDP_UART_DsDE(void)
{
    WRITE_REG(GPIOD->BSRRH, (0x1u << 1*7));             /* PD7 Output Low                                   */
}

/**
  * @brief  UART Transmit Data Register empty interrupt Enable
  */
void PBDP_UART_EnTXE(void)
{
    __USART_ENABLE_IT(_hUART, USART_IT_TXE);            /* Enable Transmit Data Register empty interrupt    */
    __nop(); __nop(); __nop(); __nop();
//  NVIC_EnableIRQ(TIM3_IRQn);                          /* Enable the Timer 3 global Interrupt              */
    __nop(); __nop(); __nop(); __nop();
//  NVIC_EnableIRQ(USART6_IRQn);                        /* Enable the USART6 global Interrupt               */
    __nop(); __nop(); __nop(); __nop();
    __nop(); __nop(); __nop(); __nop();
}

/**
  * @brief  UART Transmit Data Register empty interrupt Disable
  */
void PBDP_UART_DsTXE(void)
{
    __USART_DISABLE_IT(_hUART, USART_IT_TXE);           /* Disable Transmit Data Register empty interrupt   */
}

void PBDP_UART_EnIntr(void)
{
    NVIC_EnableIRQ(TIM3_IRQn);                          /* Enable the Timer 3 global Interrupt              */
    NVIC_EnableIRQ(USART6_IRQn);                        /* Enable the USART6 global Interrupt               */
}

void PBDP_UART_DsIntr(void)
{
    NVIC_DisableIRQ(TIM3_IRQn);                         /* Disable the Timer 3 global Interrupt             */
    NVIC_DisableIRQ(USART6_IRQn);                       /* Disable the USART6 global Interrupt              */
}

/**
  * @brief  USART interrupt handles function
  */
void USART6_IRQHandler(void)
{
    uint32_t    tmp1 = 0, tmp2 = 0;
    int         tmp3 = 0;

    tmp1 = __HAL_USART_GET_FLAG(_hUART, USART_FLAG_PE);
    tmp2 = __HAL_USART_GET_IT_SOURCE(_hUART, USART_IT_PE);
    if( (tmp1 != RESET) && (tmp2 != RESET) ) {  /* USART parity error interrupt occurred    */
        __HAL_USART_CLEAR_PEFLAG(_hUART);
        PBDP_UART_IDEL_CHK_STOP();
        PBDP_UART_EventCB(PBDP_EVENT_ERR);
        PBDP_DBG_ERR_PE_INC();
        PBDP_DBG_EVT_PUSH(1);
    }

    tmp1 = __HAL_USART_GET_FLAG(_hUART, USART_FLAG_FE);
    tmp2 = __HAL_USART_GET_IT_SOURCE(_hUART, USART_IT_ERR);
    if( (tmp1 != RESET) && (tmp2 != RESET) ) {  /* USART frame error interrupt occurred     */
        __HAL_USART_CLEAR_FEFLAG(_hUART);
        PBDP_UART_IDEL_CHK_STOP();
        PBDP_UART_EventCB(PBDP_EVENT_ERR);
        PBDP_DBG_ERR_FE_INC();
        PBDP_DBG_EVT_PUSH(2);
    }

    tmp1 = __HAL_USART_GET_FLAG(_hUART, USART_FLAG_NE);
    tmp2 = __HAL_USART_GET_IT_SOURCE(_hUART, USART_IT_ERR);
    if( (tmp1 != RESET) && (tmp2 != RESET) ) {  /* USART noise error interrupt occurred     */
        __HAL_USART_CLEAR_NEFLAG(_hUART);
        PBDP_UART_IDEL_CHK_STOP();
        PBDP_UART_EventCB(PBDP_EVENT_ERR);
        PBDP_DBG_ERR_NE_INC();
        PBDP_DBG_EVT_PUSH(3);
    }

    tmp1 = __HAL_USART_GET_FLAG(_hUART, USART_FLAG_ORE);
    tmp2 = __HAL_USART_GET_IT_SOURCE(_hUART, USART_IT_ERR);
    if( (tmp1 != RESET) && (tmp2 != RESET) ) {  /* USART Over-Run error interrupt occurred  */
        __HAL_USART_CLEAR_OREFLAG(_hUART);
        PBDP_UART_IDEL_CHK_STOP();
        PBDP_UART_EventCB(PBDP_EVENT_ERR);
        PBDP_DBG_ERR_ORE_INC();
        PBDP_DBG_EVT_PUSH(4);
    }

    tmp1 = __HAL_USART_GET_FLAG(_hUART, USART_FLAG_IDLE);
    tmp2 = __HAL_USART_GET_IT_SOURCE(_hUART, USART_IT_IDLE);
    if( (tmp1 != RESET) && (tmp2 != RESET) ) {  /* USART line IDLE occurred                 */
        __HAL_USART_CLEAR_IDLEFLAG(_hUART);
        PBDP_UART_IDEL_CHK_START();
        PBDP_UART_EventCB(PBDP_EVENT_IDLE);
        PBDP_DBG_EVT_PUSH(5);
    }

    tmp1 = __HAL_USART_GET_FLAG(_hUART, USART_FLAG_TC);
    tmp2 = __HAL_USART_GET_IT_SOURCE(_hUART, USART_IT_TC);
    if( (tmp1 != RESET) && (tmp2 != RESET) ) {  /* USART Transmission complete              */
        __HAL_USART_CLEAR_FLAG(_hUART, USART_FLAG_TC);
      //PBDP_UART_IDEL_CHK_STOP();
        PBDP_UART_EventCB(PBDP_EVENT_CPLT);
        PBDP_DBG_EVT_PUSH(6);
    }

    tmp1 = __HAL_USART_GET_FLAG(_hUART, USART_FLAG_TXE);
    tmp2 = __HAL_USART_GET_IT_SOURCE(_hUART, USART_IT_TXE);
    if( (tmp1 != RESET) && (tmp2 != RESET) ) {  /* USART Transmit data register empty       */
        PBDP_UART_IDEL_CHK_STOP();
        if( (tmp3 = PBDP_UART_SendCB()) >= 0 ) {
            _hUART->Instance->DR = tmp3;
        }
        PBDP_DBG_EVT_PUSH(7);
    }

    tmp1 = __HAL_USART_GET_FLAG(_hUART, USART_FLAG_RXNE);
    tmp2 = __HAL_USART_GET_IT_SOURCE(_hUART, USART_IT_RXNE);
    if( (tmp1 != RESET) && (tmp2 != RESET) ) {  /* USART Receive data register not empty    */
        PBDP_UART_IDEL_CHK_STOP();
        PBDP_UART_RecvCB(_hUART->Instance->DR);
        PBDP_DBG_EVT_PUSH(8);
    }
}

/**
  * @brief  Timer interrupt handles function
  */
void TIM3_IRQHandler(void)
{
    if( (TIM3->SR & TIM_SR_UIF) && (TIM3->DIER & TIM_DIER_UIE) ) {
        TIM3->SR = ~TIM_SR_UIF;                 /* Timer Update interrupt flag clear        */
        PBDP_UART_EventCB(PBDP_EVENT_IDLE);
        PBDP_DBG_EVT_PUSH(9);
    }
}


/*****************************  END OF FILE  **************************************************************/
/** @}
*** @}
*** @}
***********************************************************************************************************/

