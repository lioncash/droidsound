#include <stdint.h>
#include <math.h>
#include "com_ssb_droidsound_utils_FFT.h"

/* Code from numerical recipes */
static void four1(float data[], int nn, int isign)
{
    int n, mmax, m, j, istep, i;
    float wtemp, wr, wpr, wpi, wi, theta;
    float tempr, tempi;
    
    n=nn << 1;
    j = 1;
    for (i = 1; i < n; i += 2) {
        if (j > i) {
            tempr = data[j];     data[j] = data[i];     data[i] = tempr;
            tempr = data[j+1]; data[j+1] = data[i+1]; data[i+1] = tempr;
        }
        m = n >> 1;
        while (m >= 2 && j > m) {
            j -= m;
            m >>= 1;
        }
        j += m;
    }
    mmax = 2;
    while (n > mmax) {
        istep = 2*mmax;
        theta = (float) (2.0 * M_PI) / (float) (isign*mmax);
        wtemp = sinf(0.5f*theta);
        wpr = -2.0f*wtemp*wtemp;
        wpi = sinf(theta);
        wr = 1.0f;
        wi = 0.0f;
        for (m = 1; m < mmax; m += 2) {
            for (i = m; i <= n; i += istep) {
                j =i + mmax;
                tempr = wr*data[j]   - wi*data[j+1];
                tempi = wr*data[j+1] + wi*data[j];
                data[j]   = data[i]   - tempr;
                data[j+1] = data[i+1] - tempi;
                data[i] += tempr;
                data[i+1] += tempi;
            }
            wr = (wtemp = wr)*wpr - wi*wpi + wr;
            wi = wi*wpr + wtemp*wpi + wi;
        }
        mmax = istep;
    }
}

static void realft(float data[], unsigned long n, int isign)
{
    unsigned long i,i1,i2,i3,i4,np3;
    float c1=0.5f,c2,h1r,h1i,h2r,h2i;
    float wr,wi,wpr,wpi,wtemp,theta;

    theta = (float) M_PI / (float) (n>>1);
    if (isign == 1) {
        c2=-0.5f;
        four1(data,n>>1,1);
    } else {
        c2=0.5f;
        theta = -theta;
    }
    wtemp=sinf(0.5f*theta);
    wpr = -2.0f*wtemp*wtemp;
    wpi=sinf(theta);
    wr=1.0f+wpr;
    wi=wpi;
    np3=n+3;
    for (i=2;i<=(n>>2);i++) {
        i4=1+(i3=np3-(i2=1+(i1=i+i-1)));
        h1r=c1*(data[i1]+data[i3]);
        h1i=c1*(data[i2]-data[i4]);
        h2r = -c2*(data[i2]+data[i4]);
        h2i=c2*(data[i1]-data[i3]);
        data[i1]=h1r+wr*h2r-wi*h2i;
        data[i2]=h1i+wr*h2i+wi*h2r;
        data[i3]=h1r-wr*h2r+wi*h2i;
        data[i4] = -h1i+wr*h2i+wi*h2r;
        wr=(wtemp=wr)*wpr-wi*wpi+wr;
        wi=wi*wpr+wtemp*wpi+wi;
    }
    if (isign == 1) {
        data[1] = (h1r=data[1])+data[2];
        data[2] = h1r-data[2];
    } else {
        data[1]=c1*((h1r=data[1])+data[2]);
        data[2]=c1*(h1r-data[2]);
        four1(data,n>>1,-1);
    }
}

JNIEXPORT void JNICALL Java_com_ssb_droidsound_utils_FFT_fft(JNIEnv *env, jclass klass, jfloatArray jinout)
{
    int32_t inoutLen = env->GetArrayLength(jinout);
    if (inoutLen & (inoutLen - 1)) {
        return;
    }

    jfloat* inout = (jfloat *) env->GetPrimitiveArrayCritical(jinout, NULL);
    if (inout == NULL) {
        return;
    }

    realft(inout, inoutLen, 1);

    env->ReleasePrimitiveArrayCritical(jinout, inout, 0);
}
