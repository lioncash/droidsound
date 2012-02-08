/**********************************************
 *
 *  General TODO's in this file.
 *
 *  -Figure out why the hell the songs keep cutting out
 *   after playing for roughly 1:25.
 *
 *  UPDATE: See line 131
 *
 **********************************************/


#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>

#include <android/log.h>
#include "com_ssb_droidsound_plugins_VGMStreamPlugin.h"

extern "C" {
#include <vgmstream/vgmstream.h>
}

int current_sample;
int total_samples = 0;

int ignore_loop;
int force_loop;
int loop_count;

bool playing = false;

int channels;
int samplerate = 44100;
int kbps = 320;

long length;
int subtunes = 1;

char title[128];
char artist[128];
char album[128];
char copyright[128];
char format[128];

/**** END DECLARATIONS ****/


static jstring NewString(JNIEnv *env, const char *str)
{
	static jchar temp[256];
	jchar *ptr = temp;
	while(*str) {
		unsigned char c = (unsigned char)*str++;
		*ptr++ = (c < 0x7f && c >= 0x20) || c >= 0xa0 ? c : '?';
	}
	jstring j = env->NewString(temp, ptr - temp);
	return j;
}


JNIEXPORT jlong JNICALL Java_com_ssb_droidsound_plugins_VGMStreamPlugin_N_1loadFile(JNIEnv *env, jobject obj, jstring fname)
{
    __android_log_print(ANDROID_LOG_VERBOSE, "VGMStreamPlugin", "in loadFile()");
    
    VGMSTREAM * vgmStream;
    
    jboolean iscopy;
	const char *s = env->GetStringUTFChars(fname, &iscopy);
    
    __android_log_print(ANDROID_LOG_VERBOSE, "VGMStreamPlugin", "Checking if file is playable");
    //Initialize and check if format is playable
    if ((vgmStream = init_vgmstream(s)) == NULL)
    {
	    return 0;
    }
    __android_log_print(ANDROID_LOG_VERBOSE, "VGMStreamPlugin", "File is indeed playable");
    
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
        // This requires a bit more messing with the VGMSTREAM than I'm comfortable with... 
        vgmStream->loop_flag = 1;
        vgmStream->loop_start_sample = 0;
        vgmStream->loop_end_sample = vgmStream->num_samples;
        //TODO: Fix this
        //vgmStream->loop_ch = calloc(vgmStream->channels, sizeof(VGMSTREAMCHANNEL));
    }
    
    /* Ignore Loop Flags */
    if (ignore_loop) 
    {
        vgmStream->loop_flag = 0;
    }
    

    channels = vgmStream->channels; 
    __android_log_print(ANDROID_LOG_VERBOSE, "VGMStreamPlugin", "Audio Channels in File: %d", channels);

    samplerate = vgmStream->sample_rate;
    __android_log_print(ANDROID_LOG_VERBOSE, "VGMStreamPlugin", "File Sample Rate: %d", samplerate);
    
    kbps = get_vgmstream_frame_size(vgmStream);
    __android_log_print(ANDROID_LOG_VERBOSE, "VGMStreamPlugin", "File Kbps: %d", kbps);
    
    total_samples = get_vgmstream_play_samples((double)loop_count, 0, 0, vgmStream);
    __android_log_print(ANDROID_LOG_VERBOSE, "VGMStreamPlugin", "File Total Samples: %d", total_samples);
    
    length = (total_samples * 1000) / vgmStream->sample_rate;
    __android_log_print(ANDROID_LOG_VERBOSE, "VGMStreamPlugin", "File Length: %d", length);
    
    //If length is not specified, give it a default.
    if( length <= 0 ) 
    {
        length = 200000;
    }
   
    playing = true;
	return (long)vgmStream;  //return length;
}


JNIEXPORT void Java_com_ssb_droidsound_plugins_VGMStreamPlugin_N_1unload(JNIEnv *env, jobject obj, jlong song)
{
    /*Clean up and free resources */
    
    VGMSTREAM* vgmDealloc = (VGMSTREAM*)song;

    __android_log_print(ANDROID_LOG_VERBOSE, "VGMStreamPlugin", "Closing and freeing file: %d", song);
    close_vgmstream(vgmDealloc);
    __android_log_print(ANDROID_LOG_VERBOSE, "VGMStreamPlugin", "Successfully closed the file");
    
    vgmDealloc = NULL;    
}


/*
 * NOTE:
 * The problem with the crashing seems to occur in getSoundData when the jshort array is freed.
 *
 * This is assumed from what I can grasp of the issue. 
 *
 * It may be in a different area of the code. vgmstream is incredibly peculiar. This -should- be working
 * 
 * Someone, anyone with awesome JNI skills, please help my stupid ass out here. I literally cannot 
 * find anything wrong with this code, yet it still crashes droidsound when playing AAX files.
 *
 */
JNIEXPORT jint JNICALL Java_com_ssb_droidsound_plugins_VGMStreamPlugin_N_1getSoundData(JNIEnv *env, jobject obj, jlong song, jshortArray sArray, jint size) 
{   
    VGMSTREAM* vgm = (VGMSTREAM*)song;
                
    jshort *ptr = env->GetShortArrayElements(sArray, NULL);
    /*
    if (playing = true)
    {   
        // Audio write function
        // Have we finished decoding ?
        current_sample += size / (channels);
        
        if(current_sample >= total_samples) 
        {
            playing = false;
        }

        //Original: Just in case the current one gives us problems.
        //render_vgmstream((sample *)ptr, size / channels, vgmStream);
        render_vgmstream((sample*)ptr, size / (vgm->channels), (VGMSTREAM*)song);

        env->ReleaseShortArrayElements(sArray, ptr, 0);

        return size;
    }*/
    
	render_vgmstream((sample*)ptr, size / (vgm->channels), (VGMSTREAM*)song);
    
    env->ReleaseShortArrayElements(sArray, (sample*)ptr, 0);
    return size;
}


JNIEXPORT jboolean JNICALL Java_com_ssb_droidsound_plugins_VGMStreamPlugin_N_1seekTo(JNIEnv *, jobject, jlong, jint)
{
    return false;
}


JNIEXPORT jboolean JNICALL Java_com_ssb_droidsound_plugins_VGMStreamPlugin_N_1setTune(JNIEnv *env, jobject obj, jlong song, jint tune)
{
    return false;
}


JNIEXPORT jstring JNICALL Java_com_ssb_droidsound_plugins_VGMStreamPlugin_N_1getStringInfo(JNIEnv *env, jobject obj, jlong song, jint what)
{
    /* To be implemented */
    return NULL;
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








