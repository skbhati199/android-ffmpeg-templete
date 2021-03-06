#include <jni.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include <pthread.h>
#include <android/log.h>
#include <libavcodec/jni.h>
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/dict.h>

AVFormatContext *i_fmt_ctx;
AVStream *i_video_stream;

AVFormatContext *o_fmt_ctx;
AVStream *o_video_stream;

static int pfd[2];
static pthread_t thr;
static const char *tag = "VideoKit";

static void* thread_func(void* in) {
    ssize_t rdsz;
    char buf[128];
    while((rdsz = read(pfd[0], buf, sizeof buf - 1)) > 0) {
        if(buf[rdsz - 1] == '\n') --rdsz;
        buf[rdsz] = 0;  /* add null-terminator */
        __android_log_write(ANDROID_LOG_DEBUG, tag, buf);
    }
    return 0;
}

jint JNI_OnLoad(JavaVM* vm, void* reserved){
    av_jni_set_java_vm(vm, NULL);

    setvbuf(stdout, 0, _IOLBF, 0);
    setvbuf(stderr, 0, _IONBF, 0);

    pipe(pfd);
    dup2(pfd[1], 1);
    dup2(pfd[1], 2);

    if(pthread_create(&thr, 0, thread_func, 0) == -1) return JNI_VERSION_1_6;

    pthread_detach(thr);

    return JNI_VERSION_1_6;
}

int main(int argc, char **argv);
JNIEXPORT jint JNICALL Java_com_syllogismobile_ffmpeg_1sample_VideoKit_run(
        JNIEnv *env,
        jobject thiz,
        jobjectArray args
) {
    int i = 0;
    int argc = 0;
    char **argv = NULL;
    jstring *strr = NULL;

    if (args != NULL) {
        argc = (*env)->GetArrayLength(env, args);
        argv = (char **) malloc(sizeof(char *) * argc);
        strr = (jstring *) malloc(sizeof(jstring) * argc);

        for (i = 0; i < argc; ++i) {
            strr[i] = (jstring)(*env)->GetObjectArrayElement(env, args, i);
            argv[i] = (char *)(*env)->GetStringUTFChars(env, strr[i], 0);
        }
    }

    jint retcode = 0;
//    retcode = main(argc, argv);





    for (i = 0; i < argc; ++i) {
        (*env)->ReleaseStringUTFChars(env, strr[i], argv[i]);
    }

    free(argv);
    free(strr);

    return retcode;
}

JNIEXPORT jint JNICALL
Java_com_syllogismobile_ffmpeg_1sample_VideoManager_createVideoFinal(JNIEnv *env, jobject thiz,
                                                                     jstring jvideo, jstring start,
                                                                     jstring middle, jstring end) {

    AVFormatContext *fmt_ctx = NULL;
    AVDictionaryEntry *tag = NULL;
    int ret;
    const char* videoPath = (*env)->GetStringUTFChars( env, jvideo, NULL ) ;
    printf("usage: file %s \n", videoPath);
//    env->ReleaseStringUTFChars(videoPath,videoPath,0);
//    if (video != 2) {
//        printf("usage: %s <input_file>\n"
//               "example program to demonstrate the use of the libavformat metadata API.\n"
//               "\n", video[0]);
//        return 1;
//    }
    if ((ret = avformat_open_input(&fmt_ctx, videoPath, NULL, NULL)))
        {
            fprintf(stderr,"usage: error %s \n", videoPath);
            return ret;

        }
    if ((ret = avformat_find_stream_info(fmt_ctx, NULL)) < 0) {
        av_log(NULL, AV_LOG_ERROR, "Cannot find stream information\n");
        return ret;
    }


    while ((tag = av_dict_get(fmt_ctx->metadata, "", tag, AV_DICT_IGNORE_SUFFIX)))
        printf("%s=%s\n", tag->key, tag->value);

//    int64_t duration = fmt_ctx->streams[0]->duration;
//    printf("duration=%s\n", duration);
    avformat_close_input(&fmt_ctx);
    return 0;


}

JNIEXPORT jint JNICALL
Java_com_syllogismobile_ffmpeg_1sample_VideoManager_getVideoTime(JNIEnv *env, jobject thiz,
                                                                 jstring video) {
    av_register_all();
    AVFormatContext* pFormatCtx = avformat_alloc_context();
    const char* videoPath = (*env)->GetStringUTFChars( env, video, NULL ) ;
    if (avformat_open_input(&pFormatCtx, videoPath, NULL, NULL) < 0) {
        fprintf(stderr,"File could not open");
        return 0;
    }


    if (avformat_find_stream_info(pFormatCtx, NULL) < 0) {
        fprintf(stderr,"File could not open");
        return 0;
    }

    int64_t duration = pFormatCtx->duration;

    avformat_close_input(&pFormatCtx);
    avformat_free_context(pFormatCtx);
    return (jint) (duration / AV_TIME_BASE);
}