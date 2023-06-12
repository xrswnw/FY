#ifndef ANYID_BOOT_DEVICE_H
#define ANYID_BOOT_DEVICE_H

#include "AnyID_Boot_EC20.h"
#include "AnyID_Boot_Uart.h"

#define DEVICE_SERVER_TXSTAT_IDLE       0
#define DEVICE_SERVER_TXSTAT_WAIT       1
#define DEVICE_SERVER_TXSTAT_OVER       2
#define DEVICE_SERVER_TXSTAT_OK         3
#define DEVICE_SERVER_TXSTAT_RX         4


#define DEVICE_FLAG_MAST                5
#define DEVICE_GATE_LEN                 9
#define DEVICE_VERSION_LEN              4
#define DEVICE_SERVER_RSP_LEN           256
#define DEVICE_SERVER_MIN_LEN           50

#define DEVICE_SERVERRSP_NUM            20


#define DEVICE_UPDATA_FLAG_RQ           1
#define DEVICE_UPDATA_FLAG_DOWN         2


#define DEVICE_UPDATA_CHK_TIME          100

#define DEVICE_UPDATA_SM5001            1
#define DEVICE_UPDATA_SM5002            2

#define DEVICE_UPDATA_BUFFER_NAME_LEN           286  
#define DEVICE_UPDATA_BUFFER_TID_LEN            311 
#define DEVICE_UPDATA_BUFFER_SIZE_LEN           325
#define DEVICE_UPDATA_BUFFER_MD5_LEN            338

#define DEVICE_HTTP_URL_LINK                     1
#define DEVICE_HTTP_GET_REQUEST                  2
#define DEVICE_HTTP_GET_REQUEST_CKECK            3
#define DEVICE_HTTP_GET_REQUEST_DOWNLOAD         4
#define DEVICE_HTTP_GET_RONSPOND                 5

#define Device_At_Rsp(opt,repat,cmd)      do{g_sDeviceServerTxBuf.to[g_sDeviceServerTxBuf.num] = opt;g_sDeviceServerTxBuf.repeat[g_sDeviceServerTxBuf.num] = repat;g_sDeviceServerTxBuf.op[g_sDeviceServerTxBuf.num++] = cmd;g_sDeviceServerTxBuf.state = EC20_CNT_OP_STAT_TX_AT;}while(0)


typedef struct deviceSenverTxBuff{
    u8 num;
    u8 index;
    u8 result;
    u16 state;
    u16 len;
    u8 id[EC20_CNT_OP_NUM];
    u8 op[EC20_CNT_OP_NUM];
    u8 txBuffer[DEVICE_SERVER_RSP_LEN];
    u8 repeat[EC20_CNT_OP_NUM];
    u32 to[EC20_CNT_OP_NUM];
    u32 tick;
}DEVICE_SENVER_TXBUFFER;

#define DEVICE_SOFTVERSION_NAME_LEN             8+8+1
#define DEVICE_SOFTVERSION_TID_LEN              6
#define DEVICE_SOFTVERSION_BUFFER_SIZE          6
#define DEVICE_SOFTVERSION_MD5                  32
typedef struct deviceUpDataInfo{
    u8 state;
    u8 flag;
    char name[DEVICE_SOFTVERSION_NAME_LEN];
    char tid[DEVICE_SOFTVERSION_TID_LEN];
    char bufferSize[DEVICE_SOFTVERSION_BUFFER_SIZE];
    char md5[DEVICE_SOFTVERSION_MD5];
    u32 tick;
}DEVICE_UPDATA_INFO;

extern DEVICE_UPDATA_INFO g_sDeviceUpDataInfo;
extern DEVICE_SENVER_TXBUFFER g_sDeviceServerTxBuf;

BOOL Device_CommunCheckRsp(DEVICE_SENVER_TXBUFFER *pCntOp, u8 *pRxBuf);
BOOL Device_CheckRsp(EC20_RCVBUFFER *pCntOp, u8 *pRxBuf, u16 len);
BOOL Device_Chk_VersionFrame(u8 *pBuffer, DEVICE_UPDATA_INFO *pDataInfo);
void Device_CommunStep(DEVICE_SENVER_TXBUFFER *pCntOp);
void Device_CommunTxCmd(DEVICE_SENVER_TXBUFFER *pCntOp, u32 sysTick);
void Device_ServerProcessRxInfo(EC20_RCVBUFFER *pRcvBuffer, u32 tick); 

void Device_Format_LinkToken();
#endif