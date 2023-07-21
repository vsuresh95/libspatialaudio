#ifndef KISS_FTR_H
#define KISS_FTR_H

#include "kiss_fft.h"
#ifdef __cplusplus
extern "C" {
#endif


/*

 Real optimized version can save about 45% cpu time vs. complex fft of a real seq.



 */

typedef struct kiss_fftr_state *kiss_fftr_cfg;

struct kiss_fftr_state {
    kiss_fft_cfg substate;
    kiss_fft_cpx * tmpbuf;
    kiss_fft_cpx * super_twiddles;
};

#define kiss_fftr_scalar float

typedef struct {
    kiss_fftr_scalar r;
    kiss_fftr_scalar i;
}kiss_fftr_cpx;

kiss_fftr_cfg kiss_fftr_alloc(int nfft,int inverse_fft,void * mem, size_t * lenmem);
/*
 nfft must be even

 If you don't care to allocate space, use mem = lenmem = NULL
*/


unsigned long long  kiss_fftr(kiss_fftr_cfg cfg,const kiss_fftr_scalar *timedata_input,kiss_fft_cpx *freqdata);
/*
 input timedata has nfft scalar points
 output freqdata has nfft/2+1 complex points
*/

unsigned long long  kiss_fftri(kiss_fftr_cfg cfg,const kiss_fft_cpx *freqdata,kiss_fftr_scalar *timedata_output);
/*
 input freqdata has  nfft/2+1 complex points
 output timedata has nfft scalar points
*/

unsigned long long KissGetCounter();
/*
 Timing helper functions and variables
*/

#define kiss_fftr_free esp_free

#ifdef __cplusplus
}
#endif
#endif
