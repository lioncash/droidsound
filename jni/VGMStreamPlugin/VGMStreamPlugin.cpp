#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>

#include <android/log.h>
#include "com_ssb_droidsound_plugins_VGMStreamPlugin.h"

extern "C"
{
#include <vgmstream/vgmstream.h>
}

int total_samples = 0;

int ignore_loop;
int force_loop;
int loop_count = 0;

int channels;
int samplerate = 44100;

long length;

/**** END DECLARATIONS ****/


static jstring NewString(JNIEnv *env, const char *str)
{
    static jchar temp[256];
    jchar *ptr = temp;
    
    while(*str)
    {
        unsigned char c = (unsigned char)*str++;
        *ptr++ = (c < 0x7f && c >= 0x20) || c >= 0xa0 ? c : '?';
    }

    jstring j = env->NewString(temp, ptr - temp);
    return j;
}


JNIEXPORT jlong JNICALL Java_com_ssb_droidsound_plugins_VGMStreamPlugin_N_1loadFile(JNIEnv *env, jobject obj, jstring fname)
{
    __android_log_print(ANDROID_LOG_VERBOSE, "VGMStreamPlugin", "in loadFile()");

    VGMSTREAM * vgmStream = NULL;

    jboolean iscopy;
    const char *s = env->GetStringUTFChars(fname, &iscopy);

    __android_log_print(ANDROID_LOG_VERBOSE, "VGMStreamPlugin", "Checking if file is playable");
    //Initialize and check if format is playable
    if ((vgmStream = init_vgmstream(s)) == NULL)
    {
        return 0;
    }
    __android_log_print(ANDROID_LOG_VERBOSE, "VGMStreamPlugin", "File is indeed playable");

    if (!vgmStream)
    {
        __android_log_print(ANDROID_LOG_VERBOSE, "VGMStreamPlugin", "Failed to open file");
        return 1;
    }

    env->ReleaseStringUTFChars(fname, s);

    //If no channels are present/recognized
    if (vgmStream->channels <= 0)
    {
        close_vgmstream(vgmStream);
        vgmStream = NULL;
        return -1;
    }

    /* Loop flag */
    /* Force only if there aren't already loop points */
    if (force_loop && !vgmStream->loop_flag)
    {
        vgmStream->loop_flag = 1;
        vgmStream->loop_start_sample = 0;
        vgmStream->loop_end_sample = vgmStream->num_samples;
        vgmStream->loop_ch = (VGMSTREAMCHANNEL*) calloc(vgmStream->channels, sizeof(VGMSTREAMCHANNEL));
    }

    /* Ignore Loop Flags */
    if (ignore_loop)
    {
        vgmStream->loop_flag = 0;
    }

    channels = vgmStream->channels;
    __android_log_print(ANDROID_LOG_VERBOSE, "VGMStreamPlugin", "Audio Channels in File: %d", channels);

    samplerate = vgmStream->sample_rate;
    __android_log_print(ANDROID_LOG_VERBOSE, "VGMStreamPlugin", "File Sample Rate: %d hz", samplerate);

    total_samples = get_vgmstream_play_samples(loop_count, 0, 0, vgmStream);
    __android_log_print(ANDROID_LOG_VERBOSE, "VGMStreamPlugin", "File Total Samples: %d", total_samples);

    length = (total_samples * 1000) / vgmStream->sample_rate;
    __android_log_print(ANDROID_LOG_VERBOSE, "VGMStreamPlugin", "File Length: %d", length);

    //If length is not specified, give it a default.
    if (length <= 0)
    {
        length = 200000;
    }

    return (long)vgmStream;
}


JNIEXPORT void Java_com_ssb_droidsound_plugins_VGMStreamPlugin_N_1unload(JNIEnv *env, jobject obj, jlong song)
{
    /*Clean up and free resources */

    VGMSTREAM* vgmDealloc = (VGMSTREAM*)song;

    __android_log_print(ANDROID_LOG_VERBOSE, "VGMStreamPlugin", "Closing and freeing file");
    close_vgmstream(vgmDealloc);
    __android_log_print(ANDROID_LOG_VERBOSE, "VGMStreamPlugin", "Successfully closed the file");

    vgmDealloc = NULL;
}


JNIEXPORT jint JNICALL Java_com_ssb_droidsound_plugins_VGMStreamPlugin_N_1getSoundData(JNIEnv *env, jobject obj, jlong song, jshortArray sArray, jint size)
{
    VGMSTREAM* vgm = (VGMSTREAM*)song;
    jshort *ptr = env->GetShortArrayElements(sArray, NULL);

    if (total_samples - (size / vgm->channels) < 0)
    {
        size = total_samples * vgm->channels;
    }

    render_vgmstream(ptr, size / vgm->channels, vgm);

    total_samples -= (size / vgm->channels);

    env->ReleaseShortArrayElements(sArray, ptr, 0);
    return size;
}

JNIEXPORT jint Java_com_ssb_droidsound_plugins_VGMStreamPlugin_N_1getFrameRate(JNIEnv *env, jobject obj, jlong song)
{
    VGMSTREAM *vgm = (VGMSTREAM*)song;

    return vgm->sample_rate;
}


JNIEXPORT jboolean JNICALL Java_com_ssb_droidsound_plugins_VGMStreamPlugin_N_1seekTo(JNIEnv *, jobject, jlong, jint)
{
    /* To be implemented */
    return false;
}


JNIEXPORT jboolean JNICALL Java_com_ssb_droidsound_plugins_VGMStreamPlugin_N_1setTune(JNIEnv *env, jobject obj, jlong song, jint tune)
{
    /* To be implemented */
    return false;
}


JNIEXPORT jstring JNICALL Java_com_ssb_droidsound_plugins_VGMStreamPlugin_N_1getStringInfo(JNIEnv *env, jobject obj, jlong song, jint what)
{
    VGMSTREAM *vgm = (VGMSTREAM*)song;
    
    //NOTE: Is there a better way of getting all this info ?
    switch(what) 
    {
        //Shows the sample rate of the file
        case 50:
        {
            char text[2];
            sprintf(text, "%d", vgm->channels);
            char* codingType = text;
            return NewString(env, codingType);
            break;
        }
        //Shows the number of channels in the file
        case 51:
        {
            char text[7];
            sprintf(text, "%d", vgm->sample_rate);
            char* sRate = text;
            return NewString(env, sRate);
            break;   
        }
        // Shows the total samples that are to be played
        case 52:
        {
            char text[12];
            sprintf(text, "%d", get_vgmstream_play_samples(loop_count, 0, 0, vgm));
            char *totalSamples = text;
            return NewString(env, totalSamples);
            break;
        }
        // Shows the frame size of the stream
        case 53:
        {
            char text[5];
            sprintf(text, "%d", get_vgmstream_frame_size(vgm));
            char *frameSize = text;
            return NewString(env, frameSize);
            break;
        }
        // Shows the total samples per frame
        case 54:
        {
            char text[5];
            sprintf(text, "%d", get_vgmstream_samples_per_frame(vgm));
            char *samplesPerFrame = text;
            return NewString(env, samplesPerFrame);
            break;
        }
    }
    return 0;
}


JNIEXPORT void JNICALL Java_com_ssb_droidsound_plugins_VGMStreamPlugin_N_1setOption(JNIEnv *env, jclass cl, jint what, jint val)
{
    /* To be implemented */
}


JNIEXPORT jint JNICALL Java_com_ssb_droidsound_plugins_VGMStreamPlugin_N_1getIntInfo(JNIEnv *env, jobject obj, jlong song, jint what)
{
    /* To be implemented */
    return 0;
}
