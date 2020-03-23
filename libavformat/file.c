#include "../berrno.h"

#include "avformat.h"
#include <fcntl.h>

#ifndef CONFIG_WIN32
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/time.h>
#else
#include <io.h>
#define open(fname,oflag,pmode) _open(fname,oflag,pmode)
#endif

// ffplay��file����������rtsp��rtp��tcp ��Э���һ��Э�飬��file:ǰ׺��ʾfileЭ�顣
// URLContext�ṹ����ͳһ��ʾ��Щ�����ϵ�Э�飬�����ṩͳһ�ĳ���ӿڡ�
// ������Ĺ���Э��ʵ���ļ�ʵ��URLContext �ӿڡ����ļ�ʵ����file ����Э���URLContext �ӿڡ�

// �򿪱���ý���ļ����ѱ����ļ������Ϊ�����ļ���������priv_data�С�
static int file_open(URLContext *h, const char *filename, int flags)
{
    int access;
    int fd;
    // ��������·���ļ�����ȥ��ǰ����ܵ�"file:"�ַ���
    strstart(filename, "file:", &filename);
    // ���ñ����ļ���ȡ���ԡ�
    if (flags &	URL_RDWR)
	access = O_CREAT | O_TRUNC | O_RDWR;
    else if (flags & URL_WRONLY)
	access = O_CREAT | O_TRUNC | O_WRONLY;
    else
	access = O_RDONLY;
#if defined(CONFIG_WIN32) || defined(CONFIG_OS2) || defined(__CYGWIN__)
    access |= O_BINARY;
#endif
    // ����open()�򿪱����ļ������ѱ����ļ������Ϊ�����URL��������priv_data�����С�
    fd = open(filename, access, 0666);
    if (fd < 0)
	return  -ENOENT;
    h->priv_data = (void*)(size_t)fd;
    return 0;
}

// ת������URL���Ϊ�����ļ����������read()�����������ļ���
static int file_read(URLContext *h, unsigned char *buf, int size)
{
    int fd = (size_t)h->priv_data;
    return read(fd, buf, size);
}
// ת������URL���Ϊ�����ļ����������wite()����д�����ļ�����������ûʵ��ʹ�ô˺�����
static int file_write(URLContext *h, unsigned char *buf, int size)
{
    int fd = (size_t)h->priv_data;
    return write(fd, buf, size);
}
// ת������URL���Ϊ�����ļ����������lseek()�������ñ����ļ���ָ�롣
static offset_t file_seek(URLContext *h, offset_t pos, int whence)
{
    int fd = (size_t)h->priv_data;
    return lseek(fd, pos, whence);
}
// ת������URL ���Ϊ�����ļ����������close()�����رձ����ļ���
static int file_close(URLContext *h)
{
    int fd = (size_t)h->priv_data;
    return close(fd);
}

// ��fileЭ����Ӧ������ʼ��URLProtocol �ṹ��
URLProtocol file_protocol =
{
	"file",
	file_open,
	file_read,
	file_write,
	file_seek,
	file_close,
};

// https://github.com/feixiao/ffmpeg-2.8.11/blob/master/libavformat/file.c

// ����Э����RTMP
// https://github.com/feixiao/ffmpeg-2.8.11/blob/master/libavformat/librtmp.c
