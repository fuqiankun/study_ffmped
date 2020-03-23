#include "avcodec.h"
#include "dsputil.h"

// ����dsp �Ż��޷�����ʹ�õĲ��ұ�ʵ�����ʼ��������

uint8_t cropTbl[256 + 2 * MAX_NEG_CROP] = { 0, };

void dsputil_static_init(void)
{
    int i;
    // ��ʼ���޷�������ұ����Ľ���ǣ�ǰMAX_NEG_CROP ��������Ϊ0�����ŵ�256 ��������ֱ�Ϊ0 ��255��
    // ����MAX_NEG_CROP ��������Ϊ255���ò�����Ƚ�ʵ���޷����㡣
    for (i = 0; i < 256; i++)
	cropTbl[i + MAX_NEG_CROP] = i;

    for (i = 0; i < MAX_NEG_CROP; i++)
    {
	cropTbl[i] = 0;
	cropTbl[i + MAX_NEG_CROP + 256] = 255;
    }
}
