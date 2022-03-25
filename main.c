#include <msp430.h>
#include <math.h>
#include "stdint.h"
#include "msp.h"
#include "lcd.h"
#include "fft.h"


unsigned int adcbuff[128] = {};  // �������
float vpp = 0;  // ���ֵ
float vrms = 0;  // ��Чֵ
unsigned int new, old, diff, button_new, button_old, state;  // ��¼����״̬
uint32_t timestamp = 0;  // ʱ���
uint16_t capvalue_1 = 0;  // ��һ�β�׽ֵ
uint16_t capvalue_2 = 0;  // �ڶ��β�׽ֵ
uint16_t capvalue_3 = 0;  // �����β�׽ֵ
uint32_t timestamp_1 = 0;  // ��һ��ʱ���
uint32_t timestamp_2 = 0;  // �ڶ���ʱ���
uint32_t timestamp_3 = 0;  // ������ʱ���
uint32_t totaltime = 0;  // ����
uint32_t a = 0;  // ������"1"�ĳ���
uint8_t flag = 0;
float freq = 0;  // Ƶ��
float duty = 0;  // ռ�ձ�
float thd = 0;  // ʧ���
unsigned int cnt = 0;


// ������
int main(void)
{
    WDTCTL = WDTPW | WDTHOLD;
    ADC_Init();  // ��ʼ��ADC����ģ��
    SystemClock_Init();  // ��ʼ��ʱ��
    Pin_Init();  // �������ã�P1.3��Ϊ��������
    LCD_Init();  // ��ʼ��LCD��
    Timer0_Init();  // ��ʼ����ʱ��
    Uart_Init();  // ��ʼ������ͨ��
    int i;
    // �¼�ѭ��
    while(1)
    {
        // ��¼������״̬����һ��5��״̬
        new = 0;
        old = 0;
        diff = 0;
        button_old = button_new;
        if(P1IN & BIT3)
        {
            button_new = 1;
        }
        else
        {
            button_new = 0;
        }
        if(button_new < button_old)
        {
            state += 1;
            state %= 5;
        }

        // ���ݰ���״̬ȷ����ʾ����
        switch(state)
        {
        case 0: // �Ӻ� ����
            clear_screen();  // ����
            display_GB2312_string(2,1,"2022F137��");
            display_GB2312_string(5,1,"�����꣬��ï��");
            delay(2000);
            break;
        case 1: // Ƶ�� ռ�ձ�
            clear_screen();  // ����
            __bis_SR_register(GIE);  //�����ж�
            // LCD����ʾ
            display_GB2312_string(1, 1, "Ƶ��Ϊ��");
            display_GB2312_string(3, 41, "kHz");
            display_GB2312_string(5, 1, "ռ�ձ�Ϊ��");
            display_GB2312_string(7, 41, "%");
            flag = 1;
            cnt = 0;
            while(flag);  // ��׽�����غ��½���
            __bic_SR_register(GIE);  // �ر��ж�
            freq = ((float)(8000000.0) / totaltime) * 2.0;  // ����Ƶ��
            duty = (float) a / totaltime;  // ����ռ�ձ�
            display_float(3,1,freq / 1000.0);
            display_float(7, 1, duty * 100.0);
            // ���ڴ�������
            uart_sendstring("freq:", 5);
            uart_sendfloat(freq / 1000.0);
            uart_sendstring("kHz\t", 4);
            uart_sendstring("duty:", 5);
            uart_sendfloat(duty * 100.0);
            uart_sendstring("%\r", 2);
            delay(20000);
            break;
        case 2: // ���ֵ ��Чֵ ʧ���
            clear_screen();  // ����
            // LCD��ʾ
            display_GB2312_string(1,1,"Vpp:");
            display_GB2312_string(1,87,"V");
            display_GB2312_string(3,1,"Vrms:");
            display_GB2312_string(3,87,"V");
            display_GB2312_string(5,1,"THD:");
            display_GB2312_string(5,87,"%");
            // ����
            for(i=0;i<128;i++) StartADCConvert();
            get_vpp();  // ������ֵ����Чֵ
            // FFT�任
            FFTR_SEQ();
            FFTR();
            // ����ʧ���
            THD();
            display_float(1, 45, vpp);
            display_float(3, 45, vrms);
            display_float(5,45,thd);
            // ���ݷ��ֵ����Чֵ�ı�ֵ�жϲ������ͣ���ʵ���������
            if(vrms/vpp > 0.9)
            {
                uart_sendstring("���Ҳ�", 6);
            }
            else if(vrms/vpp <= 0.65)
            {
                uart_sendstring("���ǲ�", 6);
            }
            else
            {
                uart_sendstring("����", 4);
            }
            uart_sendstring("Vpp:",5);
            uart_sendfloat(vpp);
            uart_sendstring("V Vrms:", 7);
            uart_sendfloat(vrms);
            uart_sendstring("V     \n\r",8);
            delay(20000);
            break;
        case 3: // ����
            clear_screen();  // ����
            display_GB2312_string(1,1,"���Σ�");
            for(i=0;i<128;i++) StartADCConvert();  // ����
            display_wave(1024);  // ��������
            delay(20000);
            break;
        case 4: // Ƶ��
            clear_screen();  // ����
            display_GB2312_string(1,1,"Ƶ�ף�");
            for(i=0;i<128;i++) StartADCConvert();  // ����
            // FFT�任
            FFTR_SEQ();
            FFTR();
            display_wave(adcbuff[0]);  // ����Ƶ��
            delay(20000);
            break;
        default:
            break;
        }
    }
}

#pragma vector = TIMER1_A1_VECTOR
__interrupt void Time_Tick(void)
{
    switch(TA1IV)
    {
    case 0x02:  // ��׽�Ƚ��ж�1
        break;
    case 0x04:  // ��׽�Ƚ��ж�2
        if(flag)
        {
            if(cnt == 0 && (TA1CCTL2 & CM_1))
            {
                capvalue_1 = TA1CCR2;  // �����һ�β�׽ֵ�������أ�
                timestamp_1 = timestamp;  // �����һ��ʱ���
                cnt = 1;
                TA1CCTL2 |= CM_2;  // ��һ�β�׽�½���
            }
            else if(cnt == 1 && (TA1CCTL2 & CM_2))
            {
                capvalue_2 = TA1CCR2;  // ����ڶ��β�׽ֵ���½��أ�
                timestamp_2 = timestamp;  // ����ڶ���ʱ���
                cnt = 2;
                TA1CCTL2 |= CM_1;  // ��һ�β�׽������
            }
            else if(cnt == 2 && (TA1CCTL2 & CM_1))
            {
                capvalue_3 = TA1CCR2;  // ��������β�׽ֵ�������أ�
                timestamp_3 = timestamp;  // ���������ʱ���
                cnt = 0;
                flag = 0;
                totaltime = (timestamp_3 - timestamp_1) * 50000 + capvalue_3 - capvalue_1;  // ������ʱ��
                a = (timestamp_2 - timestamp_1) * 50000 +  capvalue_2 - capvalue_1;  // ����ǿ�ʱ��
            }
        }
        break;
    case 0x0A:  // ����ж�
        timestamp ++;
        break;
    default:
        break;
    }
}




