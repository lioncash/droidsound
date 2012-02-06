/**********************************************
 *
 *  General TODO's in this file.
 *
 *  -Figure out why the hell the songs keep cutting out
 *   after playing for roughly 1:25.
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

VGMSTREAM * vgmStream = NULL;

int current_sample;
int total_samples = 0;

int ignore_loop;
int force_loop;
int loop_count;

bool playing = false;

int channels = 1;
int samplerate = 44100;
int kbps = 320;

long length;
int subtunes;

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
    jboolean iscopy;
	const char *s = env->GetStringUTFChars(fname, &iscopy);
    
    //Initialize and check if format is playable
    vgmStream = init_vgmstream(s);
    
    env->ReleaseStringUTFChars(fname, s);
    
    //If stream could not be opened, exit.
    if(!vgmStream)
    {
        return -1;
    }
    
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
    samplerate = vgmStream->sample_rate;
    kbps = get_vgmstream_frame_size(vgmStream);
    total_samples = get_vgmstream_play_samples((double)loop_count, 0, 0, vgmStream);
    length = (total_samples * 1000) / vgmStream->sample_rate;
    
    //If length is not specified, give it a default.
    if( length <= 0 ) 
    {
        length = 200000;
    }
   
    
    playing = true;
	return length;
}


JNIEXPORT void Java_com_ssb_droidsound_plugins_VGMStreamPlugin_N_1unload(JNIEnv *env, jobject obj, jlong song)
{
    // Cleanup
    close_vgmstream(vgmStream);
    vgmStream = NULL;
    
}

JNIEXPORT jint JNICALL Java_com_ssb_droidsound_plugins_VGMStreamPlugin_N_1getSoundData(JNIEnv *env, jobject obj, jlong song, jshortArray sArray, jint size) 
{
    if (playing = true)
    {       
        // Audio write function
        // Have we finished decoding ?
        current_sample += size / (channels);
        
        if(current_sample >= total_samples) 
        {
            playing = false;
            return 0;
        }
        
        // Get the short* pointer from the Java array
        jshort *ptr = (jshort*)env->GetShortArrayElements(sArray, NULL);
        
        //Original: Just in case the current one gives us problems.
        //render_vgmstream((sample *)ptr, size / (channels << 1), vgmStream);
        render_vgmstream((sample *)ptr, size / channels, vgmStream);
        
        env->ReleaseShortArrayElements(sArray, ptr, 0);
        
        return size;
    }

    return 0;
}


JNIEXPORT jboolean JNICALL Java_com_ssb_droidsound_plugins_VGMStreamPlugin_N_1seekTo(JNIEnv *, jobject, jlong, jint)
{
    return false;
}


JNIEXPORT jboolean JNICALL Java_com_ssb_droidsound_plugins_VGMStreamPlugin_N_1setTune(JNIEnv *env, jobject obj, jlong song, jint tune)
{
    return true;
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








