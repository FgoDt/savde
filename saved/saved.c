// saved.cpp: 定义应用程序的入口点。
//

#include "saved.h"
#include "log.h"
#include "define.h"
#include "codec_context.h"
#include "internal_context.h"
#include "format.h"

#include<libavformat/avformat.h>
#include<libavcodec/avcodec.h>
#include<libswresample/swresample.h>
#include<libswscale/swscale.h>
#include<libavutil/time.h>
#include<libavutil/avutil.h>


void test2(char *h) {
    printf("%s", h);
}


void test(void)
{
    saved_set_log_level(SAVEDLOG_LEVEL_W);
    SAVEDLOG1(NULL, 1, "nihao %d", 102);
    SAVLOGD("keyi keyi");
    SAVLOGE("E keyi");
    SAVLOGW("W keyi");
    printf("hello\n");
}

SAVEDContext* saved_create_context() {
    SAVEDContext *ctx = (SAVEDContext*)malloc(sizeof(SAVEDContext));
    ctx->ictx = NULL;
    ctx->openmark = OPENMARK;
    SAVLOGD("create context done");
    return ctx;
}

int saved_open(SAVEDContext *ctx, const char *path, const char *options, int isencoder) {

    RETIFCTXNULL(ctx);

    ctx->ictx = malloc(sizeof(SAVEDInternalContext*));

    SAVEDInternalContext *ictx = saved_internal_alloc();

    ictx->isencoder = isencoder;
     
    if (path == NULL && !isencoder)
    {
        SAVLOGE("there is no path for decoder use");
        return SAVED_E_USE_NULL;
    }
    saved_internal_open(ictx, path, options);
}

