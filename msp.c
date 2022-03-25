#include <msp430.h>
#include "stdint.h"
#include "msp.h"

extern unsigned int adcbuff[128];

// ADCת��
void StartADCConvert(void)
{
      ADC10CTL0 |= ADC10SC|ENC;  //*��ʼת��
      while(ADC10CTL1&ADC10BUSY);  // �ȴ�ת�����

}

// ADC����
void ADC_Init(void)
{
      ADC10CTL1 |= ADC10SSEL_3;  // ����ADCʱ��MCLK
      ADC10CTL1 |= ADC10DIV_0;  // ADC ����Ƶ
      ADC10CTL0 |= SREF_0;  // ����ADC��׼ԴΪϵͳ��ѹ
      ADC10CTL0 |= ADC10SHT_0;  // ����ADC��������ʱ��4CLK
      ADC10CTL0 |= ADC10SR;  // ����ADC������200k
      ADC10CTL1 |= INCH_0;  // ѡ��ADC����ͨ��A0
      ADC10AE0 |= 0;  // ����A0ģ������
      ADC10DTC0 |= ADC10CT;  // DTC����ģʽ
      ADC10DTC1 = 128;  // �������
      ADC10SA = (int)(adcbuff);  // ��ʼ��ַ
      ADC10CTL0 |= ADC10ON;  // ����ADC
      ADC10CTL0 |= ENC; // ����ת��
}

// ϵͳʱ������
void SystemClock_Init(void)
{
    DCOCTL = CALDCO_16MHZ;  // ����DCOΪ1MHz
    BCSCTL1 = CALBC1_16MHZ;
    BCSCTL2 &= ~SELS;  // ����SMCLK��ʱ��ԴΪDCO
    BCSCTL2 &= ~(DIVS0 | DIVS1);  // SMCLK�ķ�Ƶϵ����Ϊ1
}

// ��ʱ������
void Timer0_Init()
{
    TA1CTL |= TASSEL_2;  // ����ʱ��ԴΪSMCLK
    TA1CTL |= MC_1;  // ���ù���ģʽΪ������
    TA1CCR0 = 49999;  // ���ö�ʱ��� 50000*SMCLK
    TA1CTL |= TAIE;  // ����TAIFG�ж�
    TA1CCTL2 |= CAP;  // TA1CCR2���ڲ�׽����
    TA1CCTL2 |= CM_1;  // �����ز�׽
    TA1CCTL2 |= CCIS0;  // P2.5��Ϊ��׽����(CCI2B)
    TA1CCTL2 |= SCS;
    P2SEL |= BIT5;
    TA1CCTL2 |= CCIE;  // ����׽�Ƚ��ж�
}

// ��������
void Pin_Init()
{
    // LCD������
    P1DIR |= BIT4+BIT5+BIT6+BIT7;
    P2DIR |= BIT0+BIT1+BIT2+BIT3;
    // P1.3��Ϊ��������
    P1DIR &=~ BIT3;
    P1REN |= BIT3;
    P1OUT |= BIT3;
}

// ���ô���ͨ��
void Uart_Init()
{
    UCA0CTL1 |= UCSWRST;  // ��λUSCI_Ax*/
    UCA0CTL0 &= ~UCSYNC;  // ѡ��USCI_AxΪUARTģʽ*/
    UCA0CTL1 |= UCSSEL1;  // ����UARTʱ��ԴΪSMCLK*/
    // ���ò�����Ϊ9600@1MHz
    UCA0BR0 = 0x82;
    UCA0BR1 = 0x06;
    UCA0MCTL |= UCBRS0;
    // ʹ�ܶ˿ڸ���
    P1SEL |= BIT1 + BIT2;
    P1SEL2 |= BIT1 + BIT2;
    UCA0CTL1 &= ~UCSWRST;  // �����λλ��ʹ��UART
}

// ���ڷ����ַ���
void uart_sendstring(uint8_t *pbuff,uint8_t num)
{
    uint8_t cnt = 0;
    for(cnt = 0;cnt < num;cnt ++)
    {
        while(UCA0STAT & UCBUSY);
        UCA0TXBUF = *(pbuff + cnt);
    }
}

// ���ڷ��͸����� xx.xx
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

