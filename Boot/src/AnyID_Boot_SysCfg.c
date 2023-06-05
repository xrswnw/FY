#include "AnyID_Boot_SysCfg.h"

u32 g_nSysState = 0;
u32 g_nDeviceNxtEraseAddr = 0;
u32 g_nDeviceNxtDownloadAddr = 0;
u8  g_nDeviceComType = 0;

#define SYS_BOOT_VER_SIZE               50
const u8 SYS_BOOT_VERSION[SYS_BOOT_VER_SIZE] = "Boot V3.1_22032600 GD32F3xx";

void Sys_Delayms(u32 n)             //ϵͳ��ʱn����
{
    n *= 0x6000;
    n++;
    while(n--);
}

void Sys_CfgClock(void)
{
    ErrorStatus HSEStartUpStatus;

    RCC_DeInit();
    //Enable HSE
    RCC_HSEConfig(RCC_HSE_ON);

    //Wait till HSE is ready
    HSEStartUpStatus = RCC_WaitForHSEStartUp();

    if(HSEStartUpStatus == SUCCESS)
    {
        //HCLK = SYSCLK = 72.0M
        RCC_HCLKConfig(RCC_SYSCLK_Div1);

        //PCLK2 = HCLK = 72.0M
        RCC_PCLK2Config(RCC_HCLK_Div1);

        //PCLK1 = HCLK/2 = 33.9M
        RCC_PCLK1Config(RCC_HCLK_Div2);

        //ADCCLK = PCLK2/2
        RCC_ADCCLKConfig(RCC_PCLK2_Div8);

        // Select USBCLK source 72 / 1.5 = 48M
        RCC_USBCLKConfig(RCC_USBCLKSource_PLLCLK_1Div5);

        //Flash 2 wait state
        FLASH_SetLatency(FLASH_Latency_2);

        //Enable Prefetch Buffer
        FLASH_PrefetchBufferCmd(FLASH_PrefetchBuffer_Enable);

        //PLLCLK = 12.00MHz * 10 = 120 MHz
        RCC_PLLConfig(RCC_PLLSource_HSE_Div1, RCC_PLLMul_10);    //PLL���������

        //Enable PLL
        RCC_PLLCmd(ENABLE);

        //Wait till PLL is ready
        while(RCC_GetFlagStatus(RCC_FLAG_PLLRDY) == RESET)
        {
        }

        //Select PLL as system clock source
        RCC_SYSCLKConfig(RCC_SYSCLKSource_PLLCLK);

        //Wait till PLL is used as system clock source
        while(RCC_GetSYSCLKSource() != 0x08)
        {
        }
    }
}

void Sys_CfgPeriphClk(FunctionalState state)
{
   RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA |
                          RCC_APB2Periph_GPIOB |
                          RCC_APB2Periph_GPIOC |
                          RCC_APB2Periph_GPIOD |
                          RCC_APB2Periph_AFIO, state);
    
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_USART3 |
                           RCC_APB1Periph_UART5  , state);

}

void Sys_CfgNVIC(void)
{
    //NVIC_InitTypeDef NVIC_InitStructure;
#ifdef  VECT_TAB_RAM
    //Set the Vector Table base location at 0x20000000
    NVIC_SetVectorTable(NVIC_VectTab_RAM, 0x0);
#else  //VECT_TAB_FLASH
    //Set the Vector Table base location at 0x08000000
#ifdef _ANYID_BOOT_STM32_
    NVIC_SetVectorTable(NVIC_VectTab_FLASH, 0x4000);
#else
    NVIC_SetVectorTable(NVIC_VectTab_FLASH, 0x0000);
#endif
#endif

    //Configure the Priority Group to 2 bits
    NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);
}

const PORT_INF SYS_RUNLED_COM = {GPIOB, GPIO_Pin_1};

void Sys_CtrlIOInit(void)
{
    GPIO_InitTypeDef GPIO_InitStructure;
    
    GPIO_PinRemapConfig(GPIO_Remap_SWJ_JTAGDisable, ENABLE);

    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;

    GPIO_InitStructure.GPIO_Pin = SYS_RUNLED_COM.Pin;
    GPIO_Init(SYS_RUNLED_COM.Port, &GPIO_InitStructure);

}



void Sys_Init(void)
{
#if SYS_ENABLE_WDT
    WDG_InitIWDG();
#endif
    //
    Sys_CfgClock();

    Sys_CfgNVIC();
    Sys_CfgPeriphClk(ENABLE);

    //��ֹ�ж�
    Sys_DisableInt();
    Sys_CtrlIOInit();
    Sys_RunLedOn();
    Sys_Delayms(10);        //һ����10ms
    FRam_InitInterface();
    Fram_ReadBootParamenter();
    
    
    
    //g_sFramBootParamenter.appState = FRAM_BOOT_APP_FAIL;//����
    
    
    
    //���appState״̬���������ǰ汾��ϢУ����󣬻ָ�Ĭ��״̬
    if(g_sFramBootParamenter.appState == FRAM_BOOT_APP_OK)
    {
        if(Sys_CheckVersion() == FALSE)
        {
            g_sFramBootParamenter.appState = FRAM_BOOT_APP_FAIL;
        }
    }

    Uart_InitInterface(UART_BAUDRARE);
    Uart_ConfigInt();
    Uart_EnableInt(ENABLE, DISABLE);
    
    
    EC20_Init();
    memcpy(g_nSoftWare, (u8 *)(SYS_BOOT_VER_ADDR + 8), EC20_SOFTVERSON_LEN);
    //SysTick ��ʼ�� 5ms
    STick_InitSysTick();
    Sys_RunLedOff();

    //ϵͳ����״̬
    
    if(g_sFramBootParamenter.appState == FRAM_BOOT_APP_OK)
    {
        a_SetState(g_nSysState, SYS_STAT_IDLE);
    }
    else
    {
        a_SetState(g_nSysState, SYS_STAT_DOWNLOAD);
    }
    

    EC20_ConnectInit(&g_sEC20Connect, EC20_CNT_CMD_PWRON, &g_sEC20Params);
    a_SetState(g_sEC20Connect.state, EC20_CNT_OP_STAT_TX);

    
    //ʹ���ж�
    Sys_EnableInt();
}

void Sys_LedTask(void)
{
    if(a_CheckStateBit(g_nSysState, SYS_STAT_RUNLED))
    {
        static u32 ledTimes = 0;

        a_ClearStateBit(g_nSysState, SYS_STAT_RUNLED);

        ledTimes++;

        if(ledTimes & 0x01)
        {
            Sys_RunLedOff();

        }
        else
        {
            Sys_RunLedOn();

        }

    #if SYS_ENABLE_WDT
        WDG_FeedIWDog();
    #endif
    }
}

typedef  void (*pFunction)(void);
pFunction Jump_To_Application;
uint32_t JumpAddress;
void Sys_Jump(u32 address)
{
    u32 stackAddr = 0;
    Sys_DisableInt();
    stackAddr = *((vu32 *)address);
    //�鿴ջ��ַ�Ƿ���RAM������CCRAM��
    if((stackAddr & 0x2FFE0000) == 0x20000000)
    {
        JumpAddress = *(vu32 *)(address + 4);
        Jump_To_Application = (pFunction) JumpAddress;

        __set_MSP(*(vu32 *)address);
        Jump_To_Application();
    }
    else
    {
        a_SetState(g_nSysState, SYS_STAT_IDLE);
        g_nSysTick = 0;
    }
    //while(1)
    {
    #if SYS_ENABLE_WDT
        WDG_FeedIWDog();
    #endif
    }
    Sys_EnableInt();
}

void Sys_BootTask(void)
{
    if(!a_CheckStateBit(g_nSysState, SYS_STAT_DOWNLOAD))
    {
        if(g_nSysTick > 40 && g_sFramBootParamenter.appState == FRAM_BOOT_APP_OK)                //TICK = STICK_TIME_MS = 5MS, ��ʱ200ms�ȴ�bootѡ��
        {
            a_SetStateBit(g_nSysState, SYS_STAT_JMP);
        }
    }

    if(a_CheckStateBit(g_nSysState, SYS_STAT_JMP))
    {
       Sys_RunLedOff();
       Sys_Jump(SYS_APP_START_ADDR);
    }
}

void Sys_UartTask(void)
{
    //���ڴ�����:���³�ʼ��
    if(USART_GetFlagStatus(UART_PORT, USART_FLAG_NE | USART_FLAG_FE | USART_FLAG_PE))
    {
        USART_ClearFlag(UART_PORT, USART_FLAG_NE | USART_FLAG_FE | USART_FLAG_PE);
        Uart_InitInterface(UART_BAUDRARE);
        Uart_ConfigInt();
        Uart_EnableInt(ENABLE, DISABLE);
    }
   
    //��������֡����
    /*
    if(Uart_IsRcvFrame(g_sUartRcvFrame))
    {
        Sys_ProcessBootFrame(&g_sUartRcvFrame, g_nDeviceComType);
        Uart_ResetFrame(g_sUartRcvFrame);
    }
*/
}

BOOL Sys_CheckVersion(void)
{
    BOOL b = FALSE;
    u8 *p = (u8 *)SYS_BOOT_VER_ADDR;
    u8 i = 0, c = 0;
        
    if(memcmp(p, SYS_VER_HEAD, SYS_VER_HEAD_SIZE) == 0) //�豸�ͺ���ȷ
    {
        for(i = SYS_VER_HEAD_SIZE; i < SYS_VERSION_SIZE; i++)
        {
            c = *p++;
            if((c < ' ' || c > 127) && (c != 0x00))
            {
                break;
            }
        }
        if(i == SYS_VERSION_SIZE)
        {
            b = TRUE;
        }
    }
    
    
    
    if(b == FALSE)
    {
        if(memcmp((u8 *)SYS_BOOT_HARDTYPE_ADDR, SYS_VER_HARD_TYPE, sizeof(SYS_VER_HARD_TYPE) - 1) == 0) //�豸Ӳ���ͺ���ȷҲ����
        {
            b = TRUE;
        }
    }
    
    return b;
}

void Sys_ProcessBootFrame(UART_RCVFRAME *pRcvFrame, u8 com)
{
  
    u16 crc1 = 0, crc2 = 0;
    memcpy(&g_sUartTempRcvFrame, pRcvFrame, sizeof(UART_RCVFRAME));


    crc1 = Uart_GetFrameCrc(g_sUartTempRcvFrame.buffer, g_sUartTempRcvFrame.index);
    crc2 = a_GetCrc(g_sUartTempRcvFrame.buffer + UART_FRAME_POS_LEN, g_sUartTempRcvFrame.index - 4);

    if(crc1 == crc2)
    {
        u8 cmd = g_sUartTempRcvFrame.buffer[UART_FRAME_POS_CMD];
        u16 destAddr = 0;
        
        destAddr = *((u16 *)(pRcvFrame->buffer + UART_FRAME_POS_DESTADDR));
        if((destAddr != SYS_FRAME_BROADCAST_ADDR) && (destAddr != g_sFramBootParamenter.addr))
        {
            return;
        }
        switch(cmd)
        {
            case UART_FRAME_CMD_RESET:
                g_nSysTick = 0;
                g_sUartTxFrame.len = Uart_RspReset();
                break;
            case UART_FRAME_CMD_ERASE:
                if(a_CheckStateBit(g_nSysState, SYS_STAT_DOWNLOAD))
                {
                    BOOL bOk = FALSE;
                    u32 addr = 0;
                    u8 sector = 0;
                    
                    sector = g_sUartTempRcvFrame.buffer[UART_FRAME_POS_PAR];
                    addr = SYS_APP_START_ADDR + (sector << 10);         //ÿ������1K
                    
                    if(addr >= SYS_APP_START_ADDR)
                    {
                        if(g_nDeviceNxtEraseAddr == addr)               //������ַ�����������ģ������������δ����
                        {
                            g_nDeviceNxtEraseAddr = addr + (1 << 10);   //ÿ������1K
                            
                            bOk = Uart_EraseFlash(addr); 
                            g_sUartTxFrame.len = Uart_RspErase(bOk);
                        }
                    }
                }
                break;
            case UART_FRAME_CMD_DL:
                if(a_CheckStateBit(g_nSysState, SYS_STAT_DOWNLOAD))
                {
                    BOOL bCheck = FALSE;
                    u32 addr = 0;
                    u32 size = 0;

                    if(g_sUartTempRcvFrame.buffer[UART_FRAME_POS_LEN] == 0x00)
                    {
                        bCheck = (BOOL)(g_sUartTempRcvFrame.buffer[UART_FRAME_POS_PAR + 0]);
                        addr = *((u32 *)(g_sUartTempRcvFrame.buffer + UART_FRAME_POS_PAR + 1));
                        size = *((u32 *)(g_sUartTempRcvFrame.buffer + UART_FRAME_POS_PAR + 5));
                        if(addr >= SYS_APP_START_ADDR)
                        {
                            //��һ�β���Ҫ�ж���ַ�������⣬��Ϊboot�����ǴӺ���ǰд���ݣ���һ�β�֪����ַ��ʲô
                            if(addr + size == g_nDeviceNxtDownloadAddr || g_nDeviceNxtDownloadAddr == 0)
                            {
                                g_nDeviceNxtDownloadAddr = addr;
                                //֡����֮ǰ���� + ��������(1 + 4 + 4) + size + crclen;
                                //frameLen = UART_FRAME_POS_PAR + 9 + size + 2;
                                Sys_RunLedOn();
                                if(BL_WriteImagePage(addr, g_sUartTempRcvFrame.buffer + UART_FRAME_POS_PAR + 9, size))
                                {
                                    g_sUartTxFrame.len = Uart_RspDownload(bCheck, addr, size);
                                }

                                Sys_RunLedOff();

                            }
                        }
                    }
                }                       
                break;
            case UART_FRAME_CMD_BOOT:
                g_sUartTxFrame.len = Uart_RspBoot();
                if(g_sUartTxFrame.len)      //���������������ʱ��ϳ�
                {

                        Uart_WriteBuffer(g_sUartTxFrame.frame, g_sUartTxFrame.len);

                   Sys_Delayms(2);         //�ȴ����һ���ֽڷ������
                   g_sUartTxFrame.len = 0;
                }
                a_ClearStateBit(g_nSysState, SYS_STAT_IDLE);
                a_SetStateBit(g_nSysState, SYS_STAT_DOWNLOAD);
                g_sFramBootParamenter.appState = FRAM_BOOT_APP_FAIL;
                Fram_WriteBootParamenter();
                Fram_WriteBackupBootParamenter();
                FLASH_Unlock();
                
                Uart_EraseFlash(SYS_BOOT_VER_ADDR);          //�汾��Ϣ�������
                g_nDeviceNxtEraseAddr = SYS_APP_START_ADDR;
                g_nDeviceNxtDownloadAddr = 0;                   //boot���ɺ���ǰд������
                
                break;
            case UART_FRAME_CMD_JMP:
                if(Sys_CheckVersion() == TRUE)
                {
                    g_sUartTxFrame.len = Uart_RspJmp();
                    a_SetStateBit(g_nSysState, SYS_STAT_JMP);
                    g_sFramBootParamenter.appState = FRAM_BOOT_APP_OK;
                    Fram_WriteBootParamenter();
                    Fram_WriteBackupBootParamenter();
                    FLASH_Lock();
                }
                break;
            case UART_FRAME_CMD_VER:
                g_sUartTxFrame.len = Uart_RspFrame(g_sUartTxFrame.frame, cmd, (u8 *)SYS_BOOT_VERSION, SYS_BOOT_VER_SIZE, UART_FRAME_FLAG_OK, UART_FRAME_RSP_NOERR);
                break;
            default:
                break;
        }
    }
    if(g_sUartTxFrame.len)
    {
        Uart_WriteBuffer(g_sUartTxFrame.frame, g_sUartTxFrame.len);
        Sys_Delayms(2);         //�ȴ����һ���ֽڷ������
        g_sUartTxFrame.len = 0;
    }

}





void Sys_EC20Task(void)
{
    if(a_CheckStateBit(g_nSysState, SYS_STAT_LTEDTU))
    {
        return; //ֻ�в���͸��ģʽ������Ҫ����ATָ�����
    }

    if(USART_GetFlagStatus(EC20_PORT, USART_FLAG_NE | USART_FLAG_FE | USART_FLAG_PE))
    {
        USART_ClearFlag(EC20_PORT, USART_FLAG_NE | USART_FLAG_FE | USART_FLAG_PE);
        EC20_InitInterface(EC20_BAUDRARE);
        EC20_ConfigInt();
        EC20_EnableInt(ENABLE, DISABLE);
    }

    if(Uart_IsRcvFrame(g_sEC20RcvFrame))
    {
        if(a_CheckStateBit(g_sEC20Connect.state, EC20_CNT_OP_STAT_RX))
        {
            //Uart_WriteBuffer(g_sEC20RcvFrame.buffer, g_sEC20RcvFrame.index);

            if(EC20_ConnectCheckRsp(&g_sEC20Connect, g_sEC20RcvFrame.buffer, g_sEC20RcvFrame.index))   //���У����Ӧ֡ʧ�ܣ��ͼ������գ�����λ���ջ�����
            {
                g_sEC20Connect.result = EC20_CNT_RESULT_OK;
                a_SetState(g_sEC20Connect.state, EC20_CNT_OP_STAT_STEP);
                
                EC20_ClearRxBuffer();
            }
            else
            {
                g_sEC20RcvFrame.state = UART_FLAG_RCV;                          //��������
                g_sEC20RcvFrame.idleTime = 0;
            }
        }
    }
    
    //����
    if(a_CheckStateBit(g_sEC20Connect.state, EC20_CNT_OP_STAT_TX))      //����ATָ��
    {
        a_ClearStateBit(g_sEC20Connect.state, EC20_CNT_OP_STAT_TX);

        if(g_sEC20Connect.index < g_sEC20Connect.num)
        {
            EC20_ClearRxBuffer();
            EC20_ConnectTxCmd(&g_sEC20Connect, g_nSysTick);
            a_SetState(g_sEC20Connect.state, EC20_CNT_OP_STAT_RX | EC20_CNT_OP_STAT_WAIT);
        }
    }

    //��ʱ
    if(a_CheckStateBit(g_sEC20Connect.state, EC20_CNT_OP_STAT_WAIT))    //�жϳ�ʱ��ÿ��ָ��ĳ�ʱʱ�䶼��һ��
    {
        if(g_sEC20Connect.tick + g_sEC20Connect.to[g_sEC20Connect.index] < g_nSysTick)
        {
            g_sEC20Connect.repeat[g_sEC20Connect.index]--;              //��Щָ�������β���
            if(g_sEC20Connect.repeat[g_sEC20Connect.index] == 0)
            {
                a_ClearStateBit(g_sEC20Connect.state, EC20_CNT_OP_STAT_WAIT);
                a_SetState(g_sEC20Connect.state, EC20_CNT_OP_STAT_STEP);
                g_sEC20Connect.result = EC20_CNT_RESULT_TO;             //��ǰ������ʱ��GPRSִ�����̴���
            }
            else
            {
                g_sEC20Connect.state = EC20_CNT_OP_STAT_TX;             //�ظ�����
            }
        }
    }

    if(a_CheckStateBit(g_sEC20Connect.state, EC20_CNT_OP_STAT_STEP))    //��һ���߼�����
    {
        a_ClearStateBit(g_sEC20Connect.state, EC20_CNT_OP_STAT_STEP);

        EC20_ConnectStep(&g_sEC20Connect);                              //�������账��
        if(g_sEC20Connect.result == EC20_CNT_RESULT_OK)                 //��������ִ����ɣ����ҳɹ�����ʾ����DTUģʽ
        {
            g_sEC20Connect.state = EC20_CNT_OP_STAT_TX;
            if(g_sEC20Connect.index == g_sEC20Connect.num)              //����ǿ�������ִ����ɣ���ʾ����DTU�����߹ر�
            {
                a_ClearStateBit(g_nSysState, SYS_STAT_HTTP_TEST);
                if(g_sEC20Connect.cmd == EC20_CNT_CMD_PWRON)            
                {
                    g_sEC20Connect.state = EC20_CNT_OP_STAT_DTU;
                    a_SetStateBit(g_nSysState, SYS_STAT_LTEDTU);        //����HTTP�������ɹ�
                    g_sEC20RcvBuffer.state = EC20_CNT_OP_STAT_TEST;
                }
                else if(g_sEC20Connect.cmd == EC20_CNT_CMD_PWROFF) 
                {

                    EC20_KeyLow();//�ػ�ָ��ִ����ɣ��ٴ�ȷ�Ϲر�
                    g_sEC20Connect.state = EC20_CNT_OP_STAT_IDLE;
                }
            }
        }
        else
        {
            if(g_sEC20Connect.cmd == EC20_CNT_CMD_PWRON)                //����ִ��ʧ�ܣ�����رգ����ﲻ��Ҫ�����ˣ���Ϊ�´λ��ǻ�������
            {
                EC20_ConnectInit(&g_sEC20Connect, EC20_CNT_CMD_PWROFF, &g_sEC20Params);
                a_SetState(g_sEC20Connect.state, EC20_CNT_OP_STAT_TX);
            }
            else                                                        //����ػ�ʧ�ܣ���ֱ�ӹرյ�Դ
            {
                g_sEC20Connect.state = EC20_CNT_OP_STAT_IDLE;
                EC20_KeyLow();
            }
        }
    }
   
}



void Sys_ServerTask(void)
{
    if(!a_CheckStateBit(g_nSysState, SYS_STAT_LTEDTU))  //ֻ��͸���ˣ�����Ҫ���������
    {
        return;
    }
    //���ڴ�����:���³�ʼ��

    if(USART_GetFlagStatus(EC20_PORT, USART_FLAG_NE | USART_FLAG_FE | USART_FLAG_PE))
    {
        USART_ClearFlag(EC20_PORT, USART_FLAG_NE | USART_FLAG_FE | USART_FLAG_PE);
        EC20_InitInterface(EC20_BAUDRARE);
        EC20_ConfigInt();
        EC20_EnableInt(ENABLE, DISABLE);
    }
    
    if(Uart_IsRcvFrame(g_sEC20RcvFrame))
    {
        
       if(a_CheckStateBit(g_sDeviceServerTxBuf.state, EC20_CNT_OP_STAT_RX_AT))
        {
          
            if(Device_CommunCheckRsp(&g_sDeviceServerTxBuf, g_sEC20RcvFrame.buffer))   
            {
                g_sDeviceServerTxBuf.index++;                                           //AT
                a_SetState(g_sDeviceServerTxBuf.state, DEVICE_SERVER_TXSTAT_WAIT);
                //EC20_ClearRxBuffer();
            }
            else
            {
                g_sEC20RcvFrame.state = UART_FLAG_RCV;                          //��������
                g_sEC20RcvFrame.idleTime = 0;
            }
        }
        //Uart_WriteBuffer(g_sEC20RcvFrame.buffer, g_sEC20RcvFrame.index);

        if(Device_CheckRsp(&g_sEC20RcvBuffer, g_sEC20RcvFrame.buffer, g_sEC20RcvFrame.index))
        {
            Device_ServerProcessRxInfo(&g_sEC20RcvBuffer, g_nSysTick);
        }
        if(EC20_ConnectCheckClose(g_sEC20RcvFrame.buffer))
        {
            EC20_ConnectInit(&g_sEC20Connect, EC20_CNT_CMD_PWROFF, &g_sEC20Params);
            a_SetState(g_sEC20Connect.state, EC20_CNT_OP_STAT_TX);
            a_ClearStateBit(g_nSysState, SYS_STAT_LTEDTU);
        }

        EC20_ClearRxBuffer();
        //memset(&g_sEC20RcvBuffer, 0, sizeof(EC20_RCVBUFFER));

    }
     
    if(a_CheckStateBit(g_sDeviceServerTxBuf.state, EC20_CNT_OP_STAT_TX_AT))      
    {
        a_ClearStateBit(g_sDeviceServerTxBuf.state, EC20_CNT_OP_STAT_TX_AT);

        if(g_sDeviceServerTxBuf.index < g_sDeviceServerTxBuf.num)
        {
            EC20_ClearRxBuffer();
            Device_CommunTxCmd(&g_sDeviceServerTxBuf, g_nSysTick);
            a_SetState(g_sDeviceServerTxBuf.state, EC20_CNT_OP_STAT_RX_AT | DEVICE_SERVER_TXSTAT_WAIT);
        }
    }
    
    if(a_CheckStateBit(g_sDeviceServerTxBuf.state, DEVICE_SERVER_TXSTAT_WAIT))    
     {
        if(g_sDeviceServerTxBuf.tick + g_sDeviceServerTxBuf.to[g_sDeviceServerTxBuf.index] < g_nSysTick)
        {
            g_sDeviceServerTxBuf.repeat[g_sDeviceServerTxBuf.index - 1]--;              
            if(g_sDeviceServerTxBuf.repeat[g_sDeviceServerTxBuf.index - 1] == 0)
            {
                a_ClearStateBit(g_sDeviceServerTxBuf.state, DEVICE_SERVER_TXSTAT_WAIT);
                a_SetState(g_sDeviceServerTxBuf.state, EC20_CNT_OP_STAT_STEP);
                g_sDeviceServerTxBuf.result = EC20_CNT_RESULT_TO;             
            }
            else
            {
                g_sDeviceServerTxBuf.state = EC20_CNT_OP_STAT_TX_AT;             
            }
        }
    }
   
   
     if(a_CheckStateBit(g_sDeviceServerTxBuf.state, EC20_CNT_OP_STAT_STEP))    //��һ���߼�����
    {
        a_ClearStateBit(g_sDeviceServerTxBuf.state, EC20_CNT_OP_STAT_STEP);
        Device_CommunStep(&g_sDeviceServerTxBuf);                              //�������账��
        if(g_sDeviceServerTxBuf.index >= g_sDeviceServerTxBuf.num)
        {
            g_sDeviceServerTxBuf.num = 0;   
            g_sDeviceServerTxBuf.index = 0;

        }
    }

    if(a_CheckStateBit(g_nSysState, SYS_STAT_HTTP_TEST))
    {
        a_ClearStateBit(g_nSysState, SYS_STAT_HTTP_TEST);
        
        Device_At_Rsp(EC20_CNT_TIME_1S, EC20_CNT_REPAT_NULL, DEVICE_HTTP_URL_LINK);
        Device_At_Rsp(EC20_CNT_TIME_1S, EC20_CNT_REPAT_NULL, DEVICE_HTTP_GET_REQUEST_CKECK);

    }

}



