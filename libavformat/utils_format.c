#include "../berrno.h"
#include "avformat.h"
#include <assert.h>

#define UINT_MAX  (0xffffffff)

#define PROBE_BUF_MIN 2048
#define PROBE_BUF_MAX 131072

// ʶ���ļ���ʽ��ý���ʽ����ʹ�õ�һЩ�����ຯ����

AVInputFormat *first_iformat = NULL;


void av_register_input_format(AVInputFormat *format)
{
    AVInputFormat **p;
    p = &first_iformat;
    // ѭ���ƶ��ڵ�ָ�뵽���һ���ļ�������ʽ��
    while (*p != NULL)
	p = &(*p)->next;
    // ֱ�ӹҽ�Ҫע����ļ�������ʽ��
    *p = format;
    format->next = NULL;
}

// �Ƚ��ļ�����չ����ʶ���ļ����͡�
int match_ext(const char *filename, const char *extensions)
{
    const char *ext, *p;
    char ext1[32], *q;

    if (!filename)
	return 0;

    // ��'.'����Ϊ��չ���ָ�������ļ���������չ���ָ����
    ext = strrchr(filename, '.');
    if (ext)
    {
	ext++;
	p = extensions;
	for (;;)
	{
	    // �ļ����п����ж�������ţ�ȡ���������ż��һ������һ������������ַ�������չ���Ƚ����ж��ļ����ͣ����Կ���Ҫ��αȽϣ�����������һ��ѭ����
	    q = ext1;
	    // ��λ��һ�������Ż��ַ���������������֮����ַ���������չ���ַ������С�
	    while (*p != '\0' &&  *p != ',' && q - ext1 < sizeof(ext1) - 1)
		*q++ = *p++;
	    // �����չ���ַ����������0��
	    *q = '\0';
	    // �Ƚ�ʶ�����չ���Ƿ���������չ����ͬ�������ͬ�ͷ���1���������
	    if (!strcasecmp(ext1, ext))
		return 1;
	    if (*p == '\0')
		break;
	    p++;
	}
    }
    // �����ǰ���ѭ����û��ƥ�䵽��չ�������ǲ�ʶ����ļ����ͷ���0
    return 0;
}

// ̽��������ļ�������ʽ������ʶ��������ļ���ʽ�����û��ʶ��������ͷ��س�ʼֵNULL��
AVInputFormat *av_probe_input_format(AVProbeData *pd, int is_opened)
{
    AVInputFormat *fmt1, *fmt;
    // score��score_max �������ʶ���ļ�������ʽ����ȷ����
    // �ļ�������ʽʶ�����������ȫ��ȷ�����趨Ϊ100�����������ȷ�����趨Ϊ50��ûʶ������趨Ϊ0��ʶ�𷽷���ͬ���µȼ���ͬ��
    int score, score_max;

    fmt = NULL;
    score_max = 0;
    for (fmt1 = first_iformat; fmt1 != NULL; fmt1 = fmt1->next)
    {
	if (!is_opened)
	    continue;

	score = 0;
	if (fmt1->read_probe)
	{
	    // ��ȡ�ļ�ͷ���ж��ļ�ͷ��������ʶ���ļ�������ʽ������ʶ�𷽷��ǳ��ɿ����趨score Ϊ100��
	    score = fmt1->read_probe(pd);
	}
	else if (fmt1->extensions)
	{
	    // ͨ���ļ���չ����ʶ���ļ�������ʽ����Ϊ�ļ���չ���κ��˶����Ըģ�����ı���չ�������ַ����ʹ���������ı���չ��������ʶ�𷽷��е�ɿ����ۺϵȼ�Ϊ50��
	    if (match_ext(pd->filename, fmt1->extensions))
		score = 50;
	}
	// ���ʶ������ĵȼ��������Ҫ��ĵȼ�������Ϊ��ȷʶ����ز�����ֵ�󣬽���һ��ѭ������󷵻���߼����Ӧ���ļ�������ʽ��
	if (score > score_max)
	{
	    score_max = score;
	    fmt = fmt1;
	}
    }
    return fmt;
}

// ��������������AVFormatParameters *ap ������������ffplay ��û���õ�������Ϊ�˲��ı�ӿ�
int av_open_input_stream(AVFormatContext **ic_ptr, ByteIOContext *pb, const char *filename,
    AVInputFormat *fmt, AVFormatParameters *ap)
{
    int err;
    AVFormatContext *ic;
    AVFormatParameters default_ap;

    if (!ap)
    {
	ap = &default_ap;
	memset(ap, 0, sizeof(default_ap));
    }
    //����AVFormatContext �ڴ棬���ֳ�Ա�����ڽ������ĳ�������и�ֵ�����ֳ�Ա������������õ�ic->iformat->read_header(ic, ap)�����и�ֵ��
    ic = av_mallocz(sizeof(AVFormatContext));
    if (!ic)
    {
	err = AVERROR_NOMEM;
	goto fail;
    }
    // ����AVFormatContext��AVInputFormat
    ic->iformat = fmt;
    // ����AVFormatContext�͹����ļ�ByteIOContext
    if (pb)
	ic->pb = *pb;

    if (fmt->priv_data_size > 0)
    {
	// ����priv_data ָ����ڴ档
	ic->priv_data = av_mallocz(fmt->priv_data_size);
	if (!ic->priv_data)
	{
	    err = AVERROR_NOMEM;
	    goto fail;
	}
    }
    else
    {
	ic->priv_data = NULL;
    }
    // ��ȡ�ļ�ͷ��ʶ��ý������ʽ��
    err = ic->iformat->read_header(ic, ap);
    if (err < 0)
	goto fail;

    *ic_ptr = ic;
    return 0;

    // �򵥳���Ĵ�����
fail:
    if (ic)
	av_freep(&ic->priv_data);

    av_free(ic);
    *ic_ptr = NULL;
    return err;
}

// �������ļ�����ʶ���ļ���ʽ��Ȼ����ú���ʶ��ý������ʽ��
int av_open_input_file(AVFormatContext **ic_ptr, const char *filename, AVInputFormat *fmt,
    int buf_size, AVFormatParameters *ap)
{
    int err, must_open_file, file_opened, probe_size;
    AVProbeData probe_data, *pd = &probe_data;
    ByteIOContext pb1, *pb = &pb1;

    file_opened = 0;
    pd->filename = "";
    if (filename)
	pd->filename = filename;
    pd->buf = NULL;
    pd->buf_size = 0;

    must_open_file = 1;

    if (!fmt || must_open_file)
    {
	// �������ļ�������ByteIOContext��������ת���κ��ʵ�ʵ����ļ�ϵͳopen()����ʵ�ʴ��ļ���
	if (url_fopen(pb, filename, URL_RDONLY) < 0)
	{
	    err = AVERROR_IO;
	    goto fail;
	}
	file_opened = 1;
	// �������ָ��ByteIOContext �ڲ�ʹ�õĻ����С�������������ڲ������С��ͨ����ָ����С��
	if (buf_size > 0)
	    url_setbufsize(pb, buf_size);

	// �ȶ�PROBE_BUF_MIN(2048)�ֽ��ļ���ʼ����ʶ���ļ���ʽ��
	// �������ʶ���ļ���ʽ���Ͱ�ʶ���ļ�������2 �������������ٶ��ļ���ʼ����ʶ��ֱ��ʶ����ļ���ʽ���߳���131072 �ֽڻ��档
	for (probe_size = PROBE_BUF_MIN; probe_size <= PROBE_BUF_MAX && !fmt; probe_size <<= 1)
	{
	    // ���·��仺�棬���¶��ļ���ʼ���ݡ�
	    pd->buf = av_realloc(pd->buf, probe_size);
	    pd->buf_size = url_fread(pb, pd->buf, probe_size);
	    // ���ļ���ָ��seek ���ļ���ʼ����������һ�ζ���
	    if (url_fseek(pb, 0, SEEK_SET) == (offset_t)-EPIPE)
	    {
		// ���seek���󣬹ر��ļ��������´򿪡�
		url_fclose(pb);
		// ���´��ļ��������ô����룬����������
		if (url_fopen(pb, filename, URL_RDONLY) < 0)
		{
		    file_opened = 0;
		    err = AVERROR_IO;
		    goto fail;
		}
	    }

	    // ����ʶ���ļ���ʽ����Ϊһ�α�һ�����ݶ࣬�����ٵ�ʱ�����ʶ�𲻳������ݶ��˿��ܾͿ����ˡ�
	    fmt = av_probe_input_format(pd, 1);
	}
	av_freep(&pd->buf);
    }

    if (!fmt)
    {
	err = AVERROR_NOFMT;
	goto fail;
    }

    // ʶ����ļ���ʽ�󣬵��ú���ʶ����av_open_input_stream ��ʽ��
    err = av_open_input_stream(ic_ptr, pb, filename, fmt, ap);
    if (err)
	goto fail;
    return 0;

fail:
    // �򵥵��쳣������
    av_freep(&pd->buf);
    if (file_opened)
	url_fclose(pb);
    *ic_ptr = NULL;
    return err;
}

// һ�ζ�ȡһ�����ݰ�����������ffplay �У�һ�ζ�ȡһ������������֡�����ݰ���
int av_read_packet(AVFormatContext *s, AVPacket *pkt)
{
    return s->iformat->read_packet(s, pkt);
}

// �����������������Щý���ļ�Ϊ����seek��������Ƶ����֡��������ffplay ����Щ������ʱ������ŵ�һ�������С�����ֵ������������
int av_add_index_entry(AVStream *st, int64_t pos, int64_t timestamp, int size, int distance, int flags)
{
    AVIndexEntry *entries, *ie;
    int index;

    if ((unsigned)st->nb_index_entries + 1 >= UINT_MAX / sizeof(AVIndexEntry)) // Խ���ж�
	return  -1;

    entries = av_fast_realloc(st->index_entries, &st->index_entries_allocated_size,
	(st->nb_index_entries + 1) * sizeof(AVIndexEntry));
    if (!entries)
	return  -1;

    st->index_entries = entries;

    index = av_index_search_timestamp(st, timestamp, AVSEEK_FLAG_ANY);

    if (index < 0)	// ����
    {
	index = st->nb_index_entries++;
	ie = &entries[index];
	assert(index == 0 || ie[-1].timestamp < timestamp);
    }
    else			// �в�
    {
	ie = &entries[index];
	if (ie->timestamp != timestamp)
	{
	    if (ie->timestamp <= timestamp)
		return  -1;

	    memmove(entries + index + 1, entries + index,
		sizeof(AVIndexEntry)*(st->nb_index_entries - index));

	    st->nb_index_entries++;
	}
    }

    ie->pos = pos;
    ie->timestamp = timestamp;
    ie->size = size;
    ie->flags = flags;

    return index;
}

int av_index_search_timestamp(AVStream *st, int64_t wanted_timestamp, int flags)
{
    AVIndexEntry *entries = st->index_entries;
    int nb_entries = st->nb_index_entries;
    int a, b, m;
    int64_t timestamp;

    a = -1;
    b = nb_entries;

    while (b - a > 1) //��û�м�¼idxֵ�����õ����۰����
    {
	m = (a + b) >> 1;
	timestamp = entries[m].timestamp;
	if (timestamp >= wanted_timestamp)
	    b = m;
	if (timestamp <= wanted_timestamp)
	    a = m;
    }

    m = (flags &AVSEEK_FLAG_BACKWARD) ? a : b;

    if (!(flags &AVSEEK_FLAG_ANY))
    {
	while (m >= 0 && m < nb_entries && !(entries[m].flags &AVINDEX_KEYFRAME))
	{
	    m += (flags &AVSEEK_FLAG_BACKWARD) ? -1 : 1;
	}
    }

    if (m == nb_entries)
	return  -1;

    return m;
}

// �ر�����ý���ļ���һ��ѵĹر��ͷŲ�����
void av_close_input_file(AVFormatContext *s)
{
    int i;
    AVStream *st;

    if (s->iformat->read_close)
	s->iformat->read_close(s);

    for (i = 0; i < s->nb_streams; i++)
    {
	st = s->streams[i];
	av_free(st->index_entries);
	av_free(st->actx);
	av_free(st);
    }

    url_fclose(&s->pb);

    av_freep(&s->priv_data);
    av_free(s);
}
// new һ���µ�ý����������AVStream ָ��
AVStream *av_new_stream(AVFormatContext *s, int id)
{
    AVStream *st;

    if (s->nb_streams >= MAX_STREAMS)
	return NULL;

    st = av_mallocz(sizeof(AVStream));
    if (!st)
	return NULL;

    st->actx = avcodec_alloc_context();

    s->streams[s->nb_streams++] = st;
    return st;
}

// ���ü���pts ʱ�ӵ���ز�����
void av_set_pts_info(AVStream *s, int pts_wrap_bits, int pts_num, int pts_den)
{
    s->time_base.num = pts_num;
    s->time_base.den = pts_den;
}
