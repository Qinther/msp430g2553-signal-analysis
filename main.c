#include <msp430.h>
#include <math.h>
#include "stdint.h"
#include "msp.h"
#include "lcd.h"
#include "fft.h"


unsigned int adcbuff[128] = {};  // 采样结果
float vpp = 0;  // 峰峰值
float vrms = 0;  // 有效值
unsigned int new, old, diff, button_new, button_old, state;  // 记录按键状态
uint32_t timestamp = 0;  // 时间戳
uint16_t capvalue_1 = 0;  // 第一次捕捉值
uint16_t capvalue_2 = 0;  // 第二次捕捉值
uint16_t capvalue_3 = 0;  // 第三次捕捉值
uint32_t timestamp_1 = 0;  // 第一次时间戳
uint32_t timestamp_2 = 0;  // 第二次时间戳
uint32_t timestamp_3 = 0;  // 第三次时间戳
uint32_t totaltime = 0;  // 周期
uint32_t a = 0;  // 周期内"1"的长度
uint8_t flag = 0;
float freq = 0;  // 频率
float duty = 0;  // 占空比
float thd = 0;  // 失真度
unsigned int cnt = 0;


// 主函数
int main(void)
{
    WDTCTL = WDTPW | WDTHOLD;
    ADC_Init();  // 初始化ADC采样模块
    SystemClock_Init();  // 初始化时钟
    Pin_Init();  // 引脚配置，P1.3设为按键输入
    LCD_Init();  // 初始化LCD屏
    Timer0_Init();  // 初始化计时器
    Uart_Init();  // 初始化串口通信
    int i;
    // 事件循环
    while(1)
    {
        // 记录按键的状态——一共4个状态
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
            state %= 4;
        }

        // 根据按键状态确定显示内容
        switch(state)
        {
        case 0: // 频率 占空比
            clear_screen();  // 清屏
            __bis_SR_register(GIE);  //开启中断
            // LCD屏显示
            display_GB2312_string(1, 1, "频率为：");
            display_GB2312_string(3, 41, "kHz");
            display_GB2312_string(5, 1, "占空比为：");
            display_GB2312_string(7, 41, "%");
            flag = 1;
            cnt = 0;
            while(flag);  // 捕捉上升沿和下降沿
            __bic_SR_register(GIE);  // 关闭中断
            freq = ((float)(8000000.0) / totaltime) * 2.0;  // 计算频率
            duty = (float) a / totaltime;  // 计算占空比
            display_float(3,1,freq / 1000.0);
            display_float(7, 1, duty * 100.0);
            // 串口传输数据
            uart_sendstring("freq:", 5);
            uart_sendfloat(freq / 1000.0);
            uart_sendstring("kHz\t", 4);
            uart_sendstring("duty:", 5);
            uart_sendfloat(duty * 100.0);
            uart_sendstring("%\r", 2);
            delay(20000);
            break;
                
        case 1: // 峰峰值 有效值 失真度
            clear_screen();  // 清屏
            // LCD显示
            display_GB2312_string(1,1,"Vpp:");
            display_GB2312_string(1,87,"V");
            display_GB2312_string(3,1,"Vrms:");
            display_GB2312_string(3,87,"V");
            display_GB2312_string(5,1,"THD:");
            display_GB2312_string(5,87,"%");
            // 采样
            for(i=0;i<128;i++) StartADCConvert();
            get_vpp();  // 计算峰峰值和有效值
            // FFT变换
            FFTR_SEQ();
            FFTR();
            // 计算失真度
            THD();
            display_float(1, 45, vpp);
            display_float(3, 45, vrms);
            display_float(5,45,thd);
            // 根据峰峰值和有效值的比值判断波的类型，依实际情况而定
            if(vrms/vpp > 0.9)
            {
                uart_sendstring("正弦波", 6);
            }
            else if(vrms/vpp <= 0.65)
            {
                uart_sendstring("三角波", 6);
            }
            else
            {
                uart_sendstring("方波", 4);
            }
            uart_sendstring("Vpp:",5);
            uart_sendfloat(vpp);
            uart_sendstring("V Vrms:", 7);
            uart_sendfloat(vrms);
            uart_sendstring("V     \n\r",8);
            delay(20000);
            break;
                
        case 2: // 波形
            clear_screen();  // 清屏
            display_GB2312_string(1,1,"波形：");
            for(i=0;i<128;i++) StartADCConvert();  // 采样
            display_wave(1024);  // 画出波形
            delay(20000);
            break;
                
        case 3: // 频谱
            clear_screen();  // 清屏
            display_GB2312_string(1,1,"频谱：");
            for(i=0;i<128;i++) StartADCConvert();  // 采样
            // FFT变换
            FFTR_SEQ();
            FFTR();
            display_wave(adcbuff[0]);  // 画出频谱
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
    case 0x02:  // 捕捉比较中断1
        break;
    case 0x04:  // 捕捉比较中断2
        if(flag)
        {
            if(cnt == 0 && (TA1CCTL2 & CM_1))
            {
                capvalue_1 = TA1CCR2;  // 保存第一次捕捉值（上升沿）
                timestamp_1 = timestamp;  // 保存第一次时间戳
                cnt = 1;
                TA1CCTL2 |= CM_2;  // 下一次捕捉下降沿
            }
            else if(cnt == 1 && (TA1CCTL2 & CM_2))
            {
                capvalue_2 = TA1CCR2;  // 保存第二次捕捉值（下降沿）
                timestamp_2 = timestamp;  // 保存第二次时间戳
                cnt = 2;
                TA1CCTL2 |= CM_1;  // 下一次捕捉上升沿
            }
            else if(cnt == 2 && (TA1CCTL2 & CM_1))
            {
                capvalue_3 = TA1CCR2;  // 保存第三次捕捉值（上升沿）
                timestamp_3 = timestamp;  // 保存第三次时间戳
                cnt = 0;
                flag = 0;
                totaltime = (timestamp_3 - timestamp_1) * 50000 + capvalue_3 - capvalue_1;  // 计算总时间
                a = (timestamp_2 - timestamp_1) * 50000 +  capvalue_2 - capvalue_1;  // 计算非空时间
            }
        }
        break;
    case 0x0A:  // 溢出中断
        timestamp ++;
        break;
    default:
        break;
    }
}




