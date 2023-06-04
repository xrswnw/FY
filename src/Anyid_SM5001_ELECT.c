#include "AnyID_SM5001_Elect.h"

ELECT_INFO g_sElectInfo = {0};
ELECT_RX_BUF g_sElectTempRcv = {0};

void Elect_Init()
{	
    Elect_InitInterface(ELECT_BAUDRARE);
    Elect_ConfigInt(ENABLE);
    Elect_InitTxDma(g_sElectInfo.txBuf.buffer, g_sElectInfo.txBuf.len);
    Elect_InitRxDma(g_sElectInfo.rxBuf.buffer, g_sElectInfo.rxBuf.len);
    Elect_EnableRxDma();
    g_sElectInfo.state =ELECT_STAT_IDLE;
}



u8 Elect_FormatFrame(u8 cmd,u16 regAdd,u16 regNum,u8 *pFrame)
{

    u8 pos = 0;
    u16 crc = 0;
    pFrame[pos++] = 0x01;
    pFrame[pos++] = cmd;
    pFrame[pos++] = (regAdd >> 8) & 0xFF;
    pFrame[pos++] = (regAdd >> 0) & 0xFF;
    pFrame[pos++] = (regNum >> 8) & 0xFF;
    pFrame[pos++] = (regNum >> 0) & 0xFF;
	//校验
    crc = Elect_GetCrc16(pFrame, pos);
    pFrame[pos++] = (crc >> 0) & 0xFF;
    pFrame[pos++] = (crc >> 8) & 0xFF;
    return pos;

}


#define POLYNOMIAL                      0xA001   //x^16 + x^12 + x^5 + 1
#define PRESET_VALUE                    0xFFFF
u16 Elect_GetCrc16(u8 *pBuffer, u8 len)
{
    u16 i = 0;
    u16 crc = 0;
    u8 j = 0;

    crc = PRESET_VALUE;
    for(i = 0; i < len; i++)
    {
        crc = crc ^ pBuffer[i];
        for(j = 0; j < 8; j++)
        {
            if(crc & 0x0001)
            {
                crc = (crc >> 1) ^ POLYNOMIAL;
            }
            else
            {
                crc = (crc >> 1);
            }
        }
    }

    return crc;
}

void Elect_TransmitCmd(ELECT_TX_BUF *pBuffer)
{	
    g_sElectInfo.state =ELECT_STAT_IDLE;
    g_sElectInfo.txBuf.cmd = ELECT_MODBUS_READ_REG;
    g_sElectInfo.txBuf.regAdd = ELECT_REG_ADDR_ELECTVALUE;
    g_sElectInfo.txBuf.regNum = 0x0002;
    g_sElectInfo.txBuf.len = Elect_FormatFrame(pBuffer->cmd, pBuffer->regAdd, pBuffer->regNum,  pBuffer->buffer);
    Elect_WriteBuffer(g_sElectInfo.txBuf.buffer, g_sElectInfo.txBuf.len);
}


BOOL Elect_CheckFrame(ELECT_RX_BUF *pRcvFrame)
{
    BOOL b = FALSE;

    if(pRcvFrame->len >= ELECT_BUF_MIN_LEN)
    {
        u16 crc1 = 0, crc2 = 0;
        crc1 = Elect_GetCrc16(pRcvFrame->buffer, pRcvFrame->len - 2);
        crc2 = pRcvFrame->buffer[pRcvFrame->len - 1] ;
        crc2 <<= 8;
        crc2 |= pRcvFrame->buffer[pRcvFrame->len - 2];		
        if(crc1 == crc2)
        {
            b = TRUE;
        }
    }
    return b;
}


u32 Float32(u32 NumFloat32)
{
	
    unsigned char  sbit,ebits;
    unsigned int mbits,result;
 
    sbit = NumFloat32 >> 31;
    ebits =  ( NumFloat32 >> 23 ) & 0xff ;
    mbits =  NumFloat32 & 0x7fffff ; 
 
    if(sbit)    //若需要负数部分，sbit为1时，result*-1,去掉判断；
    {
        return -1 ;
    }
    ebits -= 127;    //得到阶码<0，即结果小于1，返回0；
    if(ebits < 0)
    {
        return 0;
    }
    result = (1 << (ebits + 10))* 1000 / 1024 + (mbits >> (13 - ebits)) * 1000 / 1024;
    return result;	
}

