#include "decoder.h"
#include <libavutil/hwcontext.h>
#include "log.h"
#include "define.h"

#ifdef  __ANDROID_NDK__
#include <jni.h>
#include <libavcodec/jni.h>
#endif

enum AVPixelFormat saved_find_fmt_by_hw_type(const enum AVHWDeviceType type) {
    enum AVPixelFormat fmt;
    switch (type)
    {
    case AV_HWDEVICE_TYPE_VAAPI:
        fmt = AV_PIX_FMT_VAAPI;
        break;
    case AV_HWDEVICE_TYPE_CUDA:
        fmt = AV_PIX_FMT_CUDA;
        break;
    case AV_HWDEVICE_TYPE_D3D11VA:
        fmt = AV_PIX_FMT_D3D11;
        break;
    case AV_HWDEVICE_TYPE_DXVA2:
        fmt = AV_PIX_FMT_DXVA2_VLD;
        break;
    case AV_HWDEVICE_TYPE_VIDEOTOOLBOX:
        fmt = AV_PIX_FMT_VIDEOTOOLBOX;
        break;
    default:
        fmt = AV_PIX_FMT_NONE;
        break;
    }

    return fmt;
}

enum AVPixelFormat saved_get_hw_format(AVCodecContext *ctx, const enum AVPixelFormat *pix_fmts) {
    const enum AVPixelFormat *p;
    for (p = pix_fmts; *p != -1; p++)
    {
        if (*p == ctx->pix_fmt)
        {
            return *p;
        }
    }
    return AV_PIX_FMT_NONE;
}

int saved_hw_decoder_init(SAVEDDecoderContext *ctx, const enum AVHWDeviceType type) {
    RETIFNULL(ctx) SAVED_E_USE_NULL;

    if (0!=av_hwdevice_ctx_create(&ctx->hw_bufferref,type,NULL,NULL,0))
    {
       SAVLOGW("create hw device error");
        
        return SAVED_OP_OK;
    }

    av_buffer_ref(ctx->hw_bufferref);

    return SAVED_OP_OK;

}



SAVEDDecoderContext* saved_decoder_alloc() {
    SAVEDDecoderContext *savctx = (SAVEDDecoderContext*)av_mallocz(sizeof(SAVEDDecoderContext));
    if (savctx==NULL)
    {
        SAVLOGE("no mem");
        return NULL;
    }
    return savctx;
}

int saved_decoder_init(SAVEDDecoderContext *ictx, SAVEDFormat *fmt, char *hwname){
    return  saved_decoder_create(ictx,hwname,fmt->astream,fmt->vstream,fmt->astream);
}

int saved_decoder_create(SAVEDDecoderContext *ictx,char *chwname,AVStream *audiostream, AVStream *videostream, AVStream *substream) {
    RETIFNULL(ictx) SAVED_E_USE_NULL;

    SAVEDDecoderContext *savctx = ictx;
    AVStream *astream = audiostream;
    AVStream *vstream = videostream;
    AVCodec *acodec = NULL;
    AVCodec *vcodec = NULL;

    //create audio deocer
    if (astream)
    {
        savctx->actx = avcodec_alloc_context3(NULL);
        if ((avcodec_parameters_to_context(savctx->actx, astream->codecpar)) < 0) {
            SAVLOGE("can't parse par to context");
            return SAVED_E_AVLIB_ERROR;
        }
        acodec = avcodec_find_decoder(savctx->actx->codec_id);
        if (acodec == NULL)
        {
            SAVLOGE("can't find decoder for audio ");
            return SAVED_E_AVLIB_ERROR;
        }
        SAVEDLOG1(NULL,SAVEDLOG_LEVEL_D,"find audio default decoder: %s", acodec->name);
        
    }

    //create video decoder
    if (vstream)
    {
        savctx->vctx = avcodec_alloc_context3(NULL);
        if ((avcodec_parameters_to_context(savctx->vctx, vstream->codecpar)) < 0) {
            SAVLOGE("can't parse par to context");
            return SAVED_E_AVLIB_ERROR;
        }
        vcodec = avcodec_find_decoder(savctx->vctx->codec_id);
        if (vcodec == NULL)
        {
            SAVLOGE("can't find decoder for video");

            return SAVED_E_AVLIB_ERROR;
        }
        SAVEDLOG1(NULL,SAVEDLOG_LEVEL_D,"find video default decoder: %s", vcodec->name);

#if __ANDROID_NDK__

        if (av_jni_get_java_vm(NULL)==NULL)
        {
            goto skip_mc;
        }

        AVCodec *MCCodec = NULL;
        char *codec_name = NULL;
        switch (savctx->vctx->codec_id)
        {
        case AV_CODEC_ID_H264:
            codec_name = "h264_mediacodec";
            break;
        case AV_CODEC_ID_HEVC:
            codec_name = "hevc_mediacodec";
            break;
        case AV_CODEC_ID_MPEG2VIDEO:
            codec_name = "mpeg2_mediacodec";
            break;
        case AV_CODEC_ID_MPEG4:
            codec_name = "mpeg4_mediacodec";
            break;
        case AV_CODEC_ID_VP8:
            codec_name = "vp8_mediacodec";
            break;
        case AV_CODEC_ID_VP9:
            codec_name = "vp9_mediacodec";
            break;
        default:
            break;
        }

        MCCodec = avcodec_find_decoder_by_name((const char*)codec_name);

        if (MCCodec == NULL) {
            SAVLOGW("can't nopen android mediacodec ");
            goto skip_mc;
        }

        vcodec = MCCodec;


        skip_mc:
#endif


        //use hardware
        if (savctx->use_hw)
        {
            enum AVHWDeviceType hwdevice;
            hwdevice = AV_HWDEVICE_TYPE_NONE;

            char *hwname = NULL;
            
            if (chwname!=NULL)
            {
                hwname = chwname;
                goto set_hw_name_done;
            }

#if _WIN32||_WIN64
            hwname = "dxva2";
#endif // _WIN32||_WIN64

#if TARGET_OS_IPHONE
            hwname = "videotoolbox";
#endif  
            if (hwname == NULL)
            {
                goto skip_hw;
            }

            set_hw_name_done:

            savctx->hw_name = hwname;

            hwdevice = av_hwdevice_find_type_by_name(hwname);

            if (hwdevice == AV_HWDEVICE_TYPE_NONE)
            {
                SAVEDLOG1(NULL, SAVEDLOG_LEVEL_W, "can not find hwdevice by name :%s", hwname);
                goto skip_hw;
            }

            enum AVPixelFormat hwpixfmt = saved_find_fmt_by_hw_type(hwdevice);

            if (hwpixfmt == -1)
            {
                SAVEDLOG1(NULL, SAVEDLOG_LEVEL_W, "can not find hw fmt by hwname :%s", hwname);
                goto skip_hw;
            }


            savctx->vctx->pix_fmt = hwpixfmt;
            savctx->vctx->get_format = saved_get_hw_format;

            if (saved_hw_decoder_init(savctx,hwdevice) == SAVED_OP_OK)
            {
                savctx->use_hw = 1;
                SAVLOGD("init hw decoder done");
            }
            else
            {
                SAVLOGW("init hw decoder error");
                av_buffer_unref(&savctx->hw_bufferref);
                savctx->use_hw= 0;
            }

        }


        skip_hw:

        if (acodec!=NULL&&0!=avcodec_open2(savctx->actx,acodec,NULL))
        {
            SAVLOGE("audio codec open error");
        }

        if (vcodec!=NULL&&0!=avcodec_open2(savctx->vctx,vcodec,NULL))
        {
            SAVLOGE("video codec open error");
        }



    }

    return SAVED_OP_OK;

}

int saved_decoder_send_pkt(SAVEDDecoderContext *ictx, AVPacket *pkt) {
    return SAVED_E_UNDEFINE;
}

int saved_decoder_recive_frame(SAVEDDecoderContext *ictx, AVFrame *f) {
    return SAVED_E_UNDEFINE;
}

int saved_decoder_close(SAVEDDecoderContext *ictx) {
    return SAVED_E_UNDEFINE;
}