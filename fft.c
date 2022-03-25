#include <math.h>
#include "fft.h"

const unsigned int Pow2M[8]={1,2,4,8,16,32,64,128};
const float sin_fft_64[]={
                         0, 0.09801714, 0.19509032, 0.29028468, 0.38268343, 0.47139674, 0.55557023, 0.63439328, 0.70710678, 0.77301045,
                         0.83146961, 0.88192126, 0.92387953, 0.95694034, 0.98078528, 0.99518473, 1
};
const float sin_fft_128[]={0,    0.049068,   0.098017,   0.14673,    0.19509,    0.24298,    0.290285,   0.33689,    0.382683,   0.427555,
                          0.471397,   0.514103,   0.55557,    0.595699,   0.634393,   0.671559,   0.707107,   0.740951,   0.77301,    0.803208,
                          0.83147,    0.857729,   0.881921,   0.903989,   0.92388,    0.941544,   0.95694,    0.970031,   0.980785,   0.989177,
                          0.995185,   0.998795,   1
};
extern unsigned int adcbuff[128];
int max;
int min;
extern float vpp;
extern float vrms;
extern float thd;


float sin_fft64(unsigned char i)
{
    if(i<=16)
        return sin_fft_64[i];
    else if(i<=32)
        return sin_fft_64[32-i];
    else if(i<=48)
        return -sin_fft_64[i-32];
    else if(i<=64)
        return -sin_fft_64[64-i];
    else
        return 0;
}


float cos_fft64(unsigned char i)
{
    if(i<=16)
        return sin_fft_64[16-i];  // cos_fft_64[i];
    else if(i<=32)
        return -sin_fft_64[i-16];  // -cos_fft_64[32-i];
    else if(i<=48)
        return -sin_fft_64[48-i];  // -cos_fft_64[i-32];
    else if(i<=64)
        return sin_fft_64[i-48];  // cos_fft_64[64-i];
    else
        return 0;
}


float sin_fft128(unsigned char i)
{
    if(i<=32)
        return sin_fft_128[i];
    else if(i<=64)
        return sin_fft_128[64-i];
    else if(i<=96)
        return -sin_fft_128[i-64];
    else if(i<=128)
        return -sin_fft_128[128-i];
    else
        return 0;
}


float cos_fft128(unsigned char i)
{
    if(i<=32)
        return sin_fft_128[32-i];  // cos_fft_128[i];
    else if(i<=64)
        return -sin_fft_128[i-32];  // -cos_fft_128[64-i];
    else if(i<=96)
        return -sin_fft_128[96-i];  // -cos_fft_128[i-64];
    else if(i<=128)
        return sin_fft_128[i-96];  // cos_fft_128[128-i];
    else
        return 0;
}

// 复数FFT，执行64位
void FFT(int dataR[],int dataI[])
{
    // 从左到右进位法反序
    unsigned int B,L,p,r,i,j, k;
    int Temp;
    float Tr, Ti;
    j=FFTN/2;  // 最高位权值
    for(i=1;i<=FFTN-2;i++)
    {
        if(i<j)
        {
            Temp=dataR[i];
            dataR[i]=dataR[j];
            dataR[j]=Temp;

            Temp=dataI[i];
            dataI[i]=dataI[j];
            dataI[j]=Temp;
        }
        k=FFTN/2;  // 从最高位开始判断
        while(1)  // 从高到低位判断
        {
            if(j<k)  // 该位为0
            {
                j=j+k;  // 置1
                break;
            }
            else  // 该位为1
            {
                j=j-k;  // 置0
                k=k/2;
            }
        }
    }

    for(L=1; L<=M;L++)  // FFT蝶形级数L从1--M
    {
        // 第L级的运算
        // 先计算一下间隔 B = 2^(L-1);
        B = Pow2M[L-1];  // 7级时，B=64
        for(j=0; j<B;j++)
        {
            // 同种蝶形运算
            // 先计算增量k=2^(M-L)
            k = Pow2M[M-L];//7级时，k=1

            // 计算旋转指数p，增量为k时，则p=j*k
            p=j*k;
            for(i = 0; i <= k - 1; i++)
            {
                // 进行蝶形运算
                // 数组下标定为r
                r = j + 2 * B * i;
                Tr=dataR[r+B]*cos_fft64(p) + dataI[r+B]*sin_fft64(p);
                Ti=dataI[r+B]*cos_fft64(p) - dataR[r+B]*sin_fft64(p);

                dataR[r+B]=(dataR[r]-Tr)/1.1;
                dataI[r+B]=(dataI[r]-Ti)/1.1;
                dataR[r]  =(dataR[r]+Tr)/1.1;
                dataI[r]  =(dataI[r]+Ti)/1.1;
            }
        }
    }
}


void FFTR_SEQ()  // 分奇偶，重复利用储存空间
{
    unsigned char i;
    int temp[FFTN];
    for (i = 0; i < FFTN; i++)
    {
        temp[i] = adcbuff[2*i+1];
    }
    for (i = 0; i < FFTN; i++)
    {
        adcbuff[i]= adcbuff[2 * i];
    }
    for (i = 0; i < FFTN; i++)
    {
        adcbuff[FFTN+i] = temp[i];
    }
}


void FFTR()
{
    unsigned int k;

    float Tr, Ti;
    int *dataR;
    dataR = (int*)adcbuff;
    int *dataI;
    dataI= (int*)&(adcbuff[64]);  // 重复利用数组
    // FFTR_SEQ();
    FFT(dataR, dataI);

    // 求X1(k)和X2(k)
    float x1R, x2R, x1I, x2I, x1I1, x2I1;
    for (k = 0; k <= FFTRN / 4; k++)  // 32个循环，计算64点,在32次以后，由于重复利用数组，会出现数据覆盖，损失dataR，dataI的前32位，因此需要在32次内将后32位计算完成
        {
            if (k == 0)
            {
                x1R = dataR[k];
                x1I = 0;
                x2R = dataI[k];
                x2I =  0;

                Tr = x1R + x2R;
                Ti = x1I + x2I;
                adcbuff[k] = sqrtf(Tr * Tr + Ti * Ti);

                Tr = x1R - x2R;
                Ti = x1I - x2I;
                adcbuff[FFTRN/2] = sqrtf(Tr * Tr + Ti * Ti);
            }
            else
            {
                // 前32位
                x1R = (dataR[k] + dataR[FFTRN / 2 - k]) / 2.0f;
                x1I = (dataI[k] - dataI[FFTRN / 2 - k]) / 2.0f;
                x2R = (dataI[k] + dataI[FFTRN / 2 - k]) / 2.0f;
                x2I = (dataR[FFTRN / 2 - k] - dataR[k]) / 2.0f;


                // 后32位
                // x1R1 = (dataR[FFTRN / 2 - k] + dataR[k]) / 2.0;
                x1I1 = (dataI[FFTRN / 2 - k] - dataI[k]) / 2.0f;
                // x2R1 = (dataI[FFTRN / 2 - k] + dataI[k]) / 2.0;
                x2I1 = (dataR[k] - dataR[FFTRN / 2 - k]) / 2.0f;


                Tr = x2R * cos_fft128(k) + x2I * sin_fft128(k);
                Ti = x2I * cos_fft128(k) - x2R * sin_fft128(k);
                Tr = x1R + Tr;
                Ti = x1I + Ti;
                adcbuff[k] = sqrtf(Tr * Tr + Ti * Ti);

                Tr = x1R - Tr;
                Ti = x1I - Ti;
                adcbuff[FFTRN/2+k] = sqrtf(Tr * Tr + Ti * Ti);

                // 因子中cos关于FFTRN / 2 并不对称！！
                Tr = x2R * cos_fft128(FFTRN / 2 -k) + x2I1 * sin_fft128(FFTRN / 2 -k);
                Ti = x2I1 * cos_fft128(FFTRN / 2 -k) - x2R * sin_fft128(FFTRN / 2 -k);
                Tr = x1R + Tr;
                Ti = x1I1 + Ti;
                adcbuff[FFTRN/2-k] = sqrtf(Tr * Tr + Ti * Ti);

                Tr = x1R - Tr;
                Ti = x1I1 - Ti;
                adcbuff[FFTRN-k] = sqrtf(Tr * Tr + Ti * Ti);
            }
        }

        for (k = 0; k < FFTRN; k++)  // 32个循环，计算64点
        {
            adcbuff[k] = adcbuff[k]/FFTRN;
        }
}


void HammingWindow()
{
    unsigned char i=0;
    for(i=0;i<128;i++)
    {
        adcbuff[i]=adcbuff[i]*(0.54f-0.46f*cos_fft128(i));
    }
}

// 计算失真度
void THD()
{
    int bas = adcbuff[1], i, item = 1;
    for(i=1; i<21; i++)
    {
        if(adcbuff[i] > bas)
        {
            bas = adcbuff[i];
            item = i;
        }
    }
    thd = sqrtf(adcbuff[item*2]*adcbuff[item*2] + adcbuff[item*3]*adcbuff[item*3] + adcbuff[item*4]*adcbuff[item*4]) * 100 / adcbuff[item]; // 五次谐波失真度
}

// 计算峰峰值和有效值
void get_vpp()
{
    int i = 0;
    float sum = 0;
    max = adcbuff[0];
    min = adcbuff[0];

    for(i=0;i<FFTRN;i++)
    {
        if(adcbuff[i] > max)
        {
            max = adcbuff[i];
        }
        if(adcbuff[i] < min)
        {
            min = adcbuff[i];
        }
    }
    vpp = (max - min) * 3.3 / 1024;
    for(i=0; i<128; i++)
    {
        sum = sum + (adcbuff[i] * 3.3 / 1024) * (adcbuff[i] * 3.3 / 1024);
    }
    vrms = sqrtf(sum / 128.0);

}




