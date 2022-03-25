#include <msp430.h>
#include "stdint.h"
#include "msp.h"

extern unsigned int adcbuff[128];

// ADC转换
void StartADCConvert(void)
{
      ADC10CTL0 |= ADC10SC|ENC;  //*开始转换
      while(ADC10CTL1&ADC10BUSY);  // 等待转换完成

}

// ADC配置
void ADC_Init(void)
{
      ADC10CTL1 |= ADC10SSEL_3;  // 设置ADC时钟MCLK
      ADC10CTL1 |= ADC10DIV_0;  // ADC 不分频
      ADC10CTL0 |= SREF_0;  // 设置ADC基准源为系统电压
      ADC10CTL0 |= ADC10SHT_0;  // 设置ADC采样保持时间4CLK
      ADC10CTL0 |= ADC10SR;  // 设置ADC采样率200k
      ADC10CTL1 |= INCH_0;  // 选择ADC输入通道A0
      ADC10AE0 |= 0;  // 允许A0模拟输入
      ADC10DTC0 |= ADC10CT;  // DTC传输模式
      ADC10DTC1 = 128;  // 传输次数
      ADC10SA = (int)(adcbuff);  // 起始地址
      ADC10CTL0 |= ADC10ON;  // 开启ADC
      ADC10CTL0 |= ENC; // 允许转换
}

// 系统时钟配置
void SystemClock_Init(void)
{
    DCOCTL = CALDCO_16MHZ;  // 配置DCO为1MHz
    BCSCTL1 = CALBC1_16MHZ;
    BCSCTL2 &= ~SELS;  // 配置SMCLK的时钟源为DCO
    BCSCTL2 &= ~(DIVS0 | DIVS1);  // SMCLK的分频系数置为1
}

// 定时器配置
void Timer0_Init()
{
    TA1CTL |= TASSEL_2;  // 设置时钟源为SMCLK
    TA1CTL |= MC_1;  // 设置工作模式为上升沿
    TA1CCR0 = 49999;  // 设置定时间隔 50000*SMCLK
    TA1CTL |= TAIE;  // 开启TAIFG中断
    TA1CCTL2 |= CAP;  // TA1CCR2用于捕捉功能
    TA1CCTL2 |= CM_1;  // 上升沿捕捉
    TA1CCTL2 |= CCIS0;  // P2.5作为捕捉输入(CCI2B)
    TA1CCTL2 |= SCS;
    P2SEL |= BIT5;
    TA1CCTL2 |= CCIE;  // 允许捕捉比较中断
}

// 配置引脚
void Pin_Init()
{
    // LCD屏引脚
    P1DIR |= BIT4+BIT5+BIT6+BIT7;
    P2DIR |= BIT0+BIT1+BIT2+BIT3;
    // P1.3设为按键输入
    P1DIR &=~ BIT3;
    P1REN |= BIT3;
    P1OUT |= BIT3;
}

// 配置串口通信
void Uart_Init()
{
    UCA0CTL1 |= UCSWRST;  // 复位USCI_Ax*/
    UCA0CTL0 &= ~UCSYNC;  // 选择USCI_Ax为UART模式*/
    UCA0CTL1 |= UCSSEL1;  // 配置UART时钟源为SMCLK*/
    // 配置波特率为9600@1MHz
    UCA0BR0 = 0x82;
    UCA0BR1 = 0x06;
    UCA0MCTL |= UCBRS0;
    // 使能端口复用
    P1SEL |= BIT1 + BIT2;
    P1SEL2 |= BIT1 + BIT2;
    UCA0CTL1 &= ~UCSWRST;  // 清除复位位，使能UART
}

// 串口发送字符串
void uart_sendstring(uint8_t *pbuff,uint8_t num)
{
    uint8_t cnt = 0;
    for(cnt = 0;cnt < num;cnt ++)
    {
        while(UCA0STAT & UCBUSY);
        UCA0TXBUF = *(pbuff + cnt);
    }
}

// 串口发送浮点数 xx.xx
void uart_sendfloat(float num)
{
    uint8_t buff[5] = {};
    uint8_t i = 0;
    for(i = 0; i < 5; i ++)
    {
        if(i == 2)
        {
            buff[i] = 0x2E;
        }
        else
        {
            if(i != 0 || (uint8_t)(num / 10) != 0)
            {
                buff[i] = (uint8_t)(num / 10) + 0x30;
            }
            num = (num - (int)(num / 10) * 10) * 10;

        }
    }
    uart_sendstring(buff,5);
}

