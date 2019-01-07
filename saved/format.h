#ifndef __SAVED_FORMAT_H__
#define __SAVED_FORMAT_H__

#include "saved.h"
#include "define.h"
#include <libavformat/avformat.h>

#define  MAX_STREAMS 10


typedef struct SAVEDFormat {
    AVFormatContext *fmt;
    AVStream *astream; //best_audio stream;
    AVStream *vstream; //best video stream;
    AVStream *sstream; //best sub stream
    enum AVMediaType allTypes[MAX_STREAMS];
    int best_audio_index;
    int best_video_index;
    int is_write_header;
    double start_pts;
}SAVEDFormat;

SAVEDFormat *saved_format_alloc();

int saved_format_free(SAVEDFormat *fmt);

int saved_format_open_input(SAVEDFormat* ctx,const char *path, const char *options);

int saved_format_open_output(SAVEDFormat* ctx, void *encoderContext,const char *path,const char *options);

int saved_format_get_pkt(SAVEDFormat *ctx, AVPacket *pkt);

int saved_format_send_pkt(SAVEDFormat *ctx, SAVEDPkt *pkt);

int saved_format_close(SAVEDFormat *fmt);

#endif
