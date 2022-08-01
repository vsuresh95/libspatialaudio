#ifndef KISS_FFT_WRAP_H
#define KISS_FFT_WRAP_H

#include "kiss_fft.h"
#include "kiss_fftr.h"

#ifdef __cplusplus
extern "C" {
#endif

#define kiss_fftw_scalar float

#define INT_BITS FIXED_POINT-FRACBITS

typedef struct {
    kiss_fftw_scalar r;
    kiss_fftw_scalar i;
} kiss_fftw_cpx;

void kiss_fftr_wrap(kiss_fftr_cfg st, const kiss_fftw_scalar *timedata, kiss_fftw_cpx *freqdata);

void kiss_fftri_wrap(kiss_fftr_cfg st, const kiss_fftw_cpx *freqdata, kiss_fftw_scalar *timedata);

#ifdef __cplusplus
}
#endif

#endif // KISS_FFT_WRAP_H