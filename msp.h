#ifndef MSP_H_
#define MSP_H_

void ADC_Init(void);
void StartADCConvert(void);
void SystemClock_Init(void);
void Timer0_Init();
void Uart_Init();
void Pin_Init();
void uart_sendstring(uint8_t *pbuff,uint8_t num);
void uart_sendfloat(float num);

#endif /* MSP_H_ */
