#include "avformat.h"

// �򵥵�ע��/��ʼ������������Ӧ��Э�飬�ļ���ʽ��������������Ӧ�������������ڲ��ҡ�

extern URLProtocol file_protocol;

void av_register_all(void)
{
    // inited ����������static����һ�±Ƚ���Ϊ�˱���˺�����ε��á�
    static int inited = 0;

    if (inited != 0)
	return;
    inited = 1;
    // ffplay ��CPU ����һ�������DSP����Щ���������CPU �Դ��ļ���ָ�����Ż���
    // ffplay �����ຯ�����������ŵ�dsputil.h ��dsputil.c �ļ��У��ú���ָ��ķ���ӳ�䵽����CPU ����ļ����Ż�ʵ�ֺ������˴���ʼ����Щ����ָ�롣
    avcodec_init();

    // �����еĽ�����������ķ�ʽ����������������ͷָ����first_avcodec��
    avcodec_register_all();

    // �����е������ļ���ʽ������ķ�ʽ����������������ͷָ����first_iformat��
    avidec_init();
    // �����е�����Э��������ķ�ʽ����������������tcp/udp/file �ȣ�����ͷָ����first_protocol��
    register_protocol(&file_protocol);
}
