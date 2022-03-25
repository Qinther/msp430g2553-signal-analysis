#include <msp430.h>
#include "lcd.h"

unsigned long fontaddr=0;
extern unsigned int adcbuff[128];

// ��ʼ��LCD
void LCD_Init()
{
    RESET_L;  // �͵�ƽ��λ
    delay(100);
    RESET_H;  // ��λ���
    delay(100);
    transfer_command_lcd(0xe2);  // ��λ
    delay(5);
    transfer_command_lcd(0x2c);  // ��ѹ���� 1
    delay(50);
    transfer_command_lcd(0x2e);  // ��ѹ���� 2
    delay(50);
    transfer_command_lcd(0x2f);  // ��ѹ���� 3
    delay(5);
    transfer_command_lcd(0x23);  // �ֵ��Աȶȣ������÷�Χ 0x20��0x27
    transfer_command_lcd(0x81);  // ΢���Աȶ�
    transfer_command_lcd(0x28);  // ΢���Աȶȵ�ֵ�������÷�Χ 0x00��0x3f
    transfer_command_lcd(0xa2);  // 1/9 ƫѹ�ȣ�bias��
    transfer_command_lcd(0xc8);  // ��ɨ��˳�򣺴��ϵ���
    transfer_command_lcd(0xa0);  // ��ɨ��˳�򣺴�����
    transfer_command_lcd(0x40);  // ��ʼ�У���һ�п�ʼ
    transfer_command_lcd(0xaf);  // ����ʾ
}

// ���ӳ�
void delay(int n_ms)
{
    int j,k;
    for(j=0;j<n_ms;j++)
    for(k=0;k<110;k++);
}

// ���ӳ�
void delay_us(int n_us)
{
    int j,k;
    for(j=0;j<n_us;j++)
    for(k=0;k<1;k++);
}

// ����ָ��
void transfer_command_lcd(int data1)
{
    char i;
    CS_L;
    RS_L;
    for(i=0;i<8;i++)
    {
        SCLK_L;
        if(data1&0x80)
        {
            SDA_H;
        }
        else
        {
            SDA_L;
        }
        SCLK_H;
        data1=data1<<=1;
    }
    CS_H;
}

// ��������
void transfer_data_lcd(int data1)
{
    char i;
    CS_L;
    RS_H;
    for(i=0;i<8;i++)
    {
        SCLK_L;
        if(data1&0x80)
        {
            SDA_H;
        }
        else
        {
            SDA_L;
        }
        SCLK_H;
        data1=data1<<=1;
    }
    CS_H;
}

// ����
void clear_screen()
{
    unsigned char i,j;
    for(i=0;i<9;i++)
    {
        transfer_command_lcd(0xb0+i);
        transfer_command_lcd(0x10);
        transfer_command_lcd(0x00);
        for(j=0;j<132;j++)
        {
            transfer_data_lcd(0x00);
        }
    }
}

// ���ж�λ
void lcd_address(unsigned int page,unsigned int column)
{
    column=column-0x01;
    transfer_command_lcd(0xb0+page-1);  // ����ҳ��ַ��ÿ 8 ��Ϊһҳ��ȫ���� 64 �У����ֳ� 8 ҳ
    transfer_command_lcd(0x10+(column>>4 & 0x0f));  // �����е�ַ�ĸ� 4 λ
    transfer_command_lcd(column&0x0f);  // �����е�ַ�ĵ� 4 λ
}

// ��ʾ����16*16���ַ�8*16
void display_GB2312_string(unsigned char page,unsigned char column,unsigned char *text)
{
    unsigned char i= 0;
    while((text[i]>0x00))
    {
        if(((text[i]>=0xb0) && (text[i]<=0xf7)) && (text[i+1]>=0xa1))
        {
            fontaddr = (text[i]- 0xb0)*94;
            fontaddr += (text[i+1]-0xa1)+846;
            fontaddr = (unsigned long)(fontaddr*32);
            get_and_write_16x16(fontaddr,page,column);  // ��ָ����ַ��������д��Һ����ָ����page,column)������
            i+=2;
            column+=16;
        }
        else if(((text[i]>=0xa1) &&(text[i]<=0xa3))&&(text[i+1]>=0xa1))
        {
            fontaddr = (text[i]- 0xa1)*94;
            fontaddr += (text[i+1]-0xa1);
            fontaddr = (unsigned long)(fontaddr*32);

            get_and_write_16x16(fontaddr,page,column);  // ��ָ����ַ��������д��Һ����ָ����page,column)������
            i+=2;
            column+=16;
        }
        else if((text[i]>=0x20) &&(text[i]<=0x7e))
        {
            fontaddr = (text[i]- 0x20);
            fontaddr = (unsigned long)(fontaddr*16);
            fontaddr = (unsigned long)(fontaddr+0x3cf80);
            get_and_write_8x16(fontaddr,page,column);  // ��ָ����ַ��������д��Һ����ָ����page,column)������
            i+=1;
            column+=8;
        }
        else
            i++;
    }
}

// ����д������
void get_and_write_16x16(unsigned long fontaddr,unsigned char page,unsigned char column)
{
    unsigned char i,j,disp_data;
    ROM_CS_L;
    send_command_to_ROM(0x03);
    send_command_to_ROM((fontaddr&0xff0000)>>16);  // ��ַ�ĸ� 8 λ,�� 24 λ
    send_command_to_ROM((fontaddr&0xff00)>>8);  // ��ַ���� 8 λ,�� 24 λ
    send_command_to_ROM(fontaddr&0xff);  // ��ַ�ĵ� 8 λ,�� 24 λ
    for(j=0;j<2;j++)
    {
        lcd_address(page+j,column);
        for(i=0; i<16; i++ )
        {
            disp_data=get_data_from_ROM();
            transfer_data_lcd(disp_data);  // д���ݵ� LCD,ÿд�� 1 �ֽڵ����ݺ��е�ַ�Զ��� 1
        }
    }
    ROM_CS_H;
}

// ����д������
void get_and_write_8x16(unsigned long fontaddr,unsigned char page,unsigned char column)
{
    unsigned char i,j,disp_data;
    ROM_CS_L;
    send_command_to_ROM(0x03);
    send_command_to_ROM((fontaddr&0xff0000)>>16);  // ��ַ�ĸ� 8 λ,�� 24 λ
    send_command_to_ROM((fontaddr&0xff00)>>8);  // ��ַ���� 8 λ,�� 24 λ
    send_command_to_ROM(fontaddr&0xff);  // ��ַ�ĵ� 8 λ,�� 24 λ
    for(j=0;j<2;j++)
    {
        lcd_address(page+j,column);
        for(i=0; i<8; i++ )
        {
            disp_data=get_data_from_ROM();
            transfer_data_lcd(disp_data);  // д���ݵ� LCD,ÿд�� 1 �ֽڵ����ݺ��е�ַ�Զ��� 1
        }
    }
    ROM_CS_H;
}

// ���ֿ��л�ȡ����
static unsigned char get_data_from_ROM()
{
    unsigned char i;
    unsigned char ret_data=0;
    for(i=0;i<8;i++)
    {
        ROM_OUT_H;
        ROM_SCK_L;
        //delay_us(1);
        ret_data=ret_data<<1;
        if(P2IN&0x10) ret_data=ret_data+1;
        else ret_data=ret_data+0;
        ROM_SCK_H;
        //delay_us(1);
    }
    return(ret_data);
}

// ���ֿⷢ��ָ��
void send_command_to_ROM( unsigned char datu )
{
    unsigned char i;
    for(i=0;i<8;i++ )
    {
        ROM_SCK_L;
        delay_us(10);
        if(datu&0x80){ROM_IN_H;}
        else{ROM_IN_L;}
        datu = datu<<1;
        ROM_SCK_H;
        delay_us(10);
    }
}

// ��ʾ������xx.xx
void display_float(unsigned int page, unsigned int column, float num)
{
    int temp,i;
    for(i=0;i<5;i++)
    {
        if(i == 2)
        {
            display_GB2312_string(page,i*8 + 1 + column,".");
        }
        else
        {
            temp = num / 10;
            num = (num - temp*10) * 10;
            if(i != 0 || temp != 0)  // ��λΪ0����ʾ
            {
                switch (temp)
                {
                case 0: display_GB2312_string(page,i*8 + 1 + column,"0");break;
                case 1: display_GB2312_string(page,i*8 + 1 + column,"1");break;
                case 2: display_GB2312_string(page,i*8 + 1 + column,"2");break;
                case 3: display_GB2312_string(page,i*8 + 1 + column,"3");break;
                case 4: display_GB2312_string(page,i*8 + 1 + column,"4");break;
                case 5: display_GB2312_string(page,i*8 + 1 + column,"5");break;
                case 6: display_GB2312_string(page,i*8 + 1 + column,"6");break;
                case 7: display_GB2312_string(page,i*8 + 1 + column,"7");break;
                case 8: display_GB2312_string(page,i*8 + 1 + column,"8");break;
                case 9: display_GB2312_string(page,i*8 + 1 + column,"9");break;
                }
            }
        }

    }
}

// ��ʾ���Σ�ֻ��ʾadcbuffǰһ�������
void display_wave(int res)
{
    unsigned int page, i, j, pagea, pageb;
    unsigned char data, data1, data2;
    for(i=0; i<64; i++) adcbuff[i] = adcbuff[i] * 40 / res;
    for(i=0; i<64; i++)
    {
        page = 8 - (int)(adcbuff[i] / 8);
        data = 0x80 >> (adcbuff[i] % 8);
        lcd_address(page, 2*i+1);
        transfer_data_lcd(data);
        if(i == 63)
        {
            break;
        }
        if(adcbuff[i] < adcbuff[i + 1])
        {
            pagea = page;
            pageb = 8 - (int)(adcbuff[i + 1] / 8);
            data1 = (0x80 >> (adcbuff[i] % 8 - 1)) - 0x01;
            data2 = 0x80 - (0x80 >> (adcbuff[i + 1] % 8 - 1)) + 0x80;
            if(pagea == pageb)
            {
                lcd_address(pagea, 2*i+2);
                transfer_data_lcd(data1 & data2);
            }
            else
            {
                lcd_address(pagea, 2*i+2);
                transfer_data_lcd(data1);
                lcd_address(pageb, 2*i+2);
                transfer_data_lcd(data2);
                for(j=pagea-1; j>pageb; j--)
                {
                    lcd_address(j, 2*i+2);
                    transfer_data_lcd(0xFF);
                }
            }
        }
        else if(adcbuff[i] > adcbuff[i + 1])
        {
            pagea = 8 - (int)(adcbuff[i + 1] / 8);
            pageb = page;
            data1 = (0x80 >> (adcbuff[i + 1] % 8 - 1)) - 0x01;
            data2 = 0x80 - (0x80 >> (adcbuff[i] % 8 - 1)) + 0x80;
            if(pagea == pageb)
            {
                lcd_address(pagea, 2*i+2);
                transfer_data_lcd(data1 & data2);
            }
            else
            {
                lcd_address(pagea, 2*i+2);
                transfer_data_lcd(data1);
                lcd_address(pageb, 2*i+2);
                transfer_data_lcd(data2);
                for(j=pagea-1; j>pageb; j--)
                {
                    lcd_address(j, 2*i+2);
                    transfer_data_lcd(0xFF);
                }
            }
        }
        else
        {
            lcd_address(page, 2*i + 2);
            transfer_data_lcd(data);
        }
    }
}





























