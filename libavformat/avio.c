#include "../berrno.h"
#include "avformat.h"

URLProtocol *first_protocol = NULL;
// ��URLProtocol �������������������ڲ��ҡ�
// register_protocol ʵ�ʾ��Ǵ����ĸ���URLProtocol��ȫ�ֱ�ͷΪfirst_protocol��
int register_protocol(URLProtocol *protocol)
{
    URLProtocol **p;
    p = &first_protocol;
    while (*p != NULL)
	p = &(*p)->next;
    *p = protocol;
    protocol->next = NULL;
    return 0;
}
// �򿪹��������ļ����˺�����Ҫ���������߼������ȴ��ļ�·�����з����Э���ַ�����proto_str�ַ������У�
// ���ű���URLProtocol �������ƥ��proto_str�ַ������е��ַ�����ȷ��ʹ�õ�Э�飬��������Ӧ���ļ�Э��Ĵ򿪺����������ļ���
int url_open(URLContext **puc, const char *filename, int flags)
{
    URLContext *uc;
    URLProtocol *up;
    const char *p;
    char proto_str[128], *q;
    int err;

    // ��ð�źͽ�������Ϊ�߽���ļ����з������Э���ַ�����proto_str�ַ�����
    p = filename;
    q = proto_str;
    while (*p != '\0' &&  *p != ':')
    {
	if (!isalpha(*p))  // protocols can only contain alphabetic chars
	    goto file_proto;
	if ((q - proto_str) < sizeof(proto_str) - 1)
	    *q++ = *p;
	p++;
    }
    // if the protocol has length 1, we consider it is a dos drive
    // ���Э���ַ���ֻ��һ���ַ������Ǿ���Ϊ��windows �µ��߼��̷����϶���file��
    if (*p == '\0' || (q - proto_str) <= 1)
    {
    file_proto:
	strcpy(proto_str, "file");
    }
    else
    {
	*q = '\0';
    }

    // ����URLProtocol ����ƥ��ʹ�õ�Э�飬���û���ҵ��ͷ��ش����롣
    up = first_protocol;
    while (up != NULL)
    {
	if (!strcmp(proto_str, up->name))
	    goto found;
	up = up->next;
    }
    err = -ENOENT;
    goto fail;
found:
    // ����ҵ��ͷ���URLContext �ṹ�ڴ棬�ر�ע���ڴ��СҪ�����ļ������ȣ��ļ����ַ����������0 
    // ҲҪԤ�ȷ���1���ֽ��ڴ棬��1 ���ֽھ���URLContext�ṹ�е�char filename[1]
    uc = av_malloc(sizeof(URLContext) + strlen(filename));
    if (!uc)
    {
	err = -ENOMEM;
	goto fail;
    }
    strcpy(uc->filename, filename);
    uc->prot = up;
    uc->flags = flags;
    uc->max_packet_size = 0; // default: stream file
    // ���ŵ�����ӦЭ����ļ��򿪺���
    err = up->url_open(uc, filename, flags);
    if (err < 0)
    {
	av_free(uc);
	*puc = NULL;
	return err;
    }
    *puc = uc;
    return 0;
fail:
    *puc = NULL;
    return err;
}
// �򵥵���ת���������ײ�Э��Ķ���������ɶ�������
int url_read(URLContext *h, unsigned char *buf, int size)
{
    int ret;
    if (h->flags &URL_WRONLY)
	return AVERROR_IO;
    ret = h->prot->url_read(h, buf, size);
    return ret;
}

// �򵥵���תseek �������ײ�Э���seek���������seek������
offset_t url_seek(URLContext *h, offset_t pos, int whence)
{
    offset_t ret;

    if (!h->prot->url_seek)
	return  -EPIPE;
    ret = h->prot->url_seek(h, pos, whence);
    return ret;
}
// �򵥵���ת�رղ������ײ�Э��Ĺرպ�������ɹرղ��������ͷ���url_open()������malloc�������ڴ档
int url_close(URLContext *h)
{
    int ret;

    ret = h->prot->url_close(h);
    av_free(h);
    return ret;
}
// ȡ������ݰ���С�������0��������ʵ����Ч�ġ�
int url_get_max_packet_size(URLContext *h)
{
    return h->max_packet_size;
}
