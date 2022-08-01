#include "kiss_fft_wrap.h"
#include "_kiss_fft_guts.h"

static inline float fixed32_to_float(int value, int n_int_bits)
{
	unsigned shift_int = 0x3f800000 - 0x800000 * (32 - n_int_bits);
	float *shift = (float *) (&shift_int);

	return (*shift) * (float)value;
}

static inline int float_to_fixed32(float value, int n_int_bits)
{
	unsigned shift_int = 0x3f800000 + 0x800000 * (32 - n_int_bits);
	float *shift = (float *) &shift_int;

	return (int)(value * (*shift));
}

struct kiss_fftr_state{
    kiss_fft_cfg substate;
    kiss_fft_cpx * tmpbuf;
    kiss_fft_cpx * super_twiddles;
#ifdef USE_SIMD
    void * pad;
#endif
};

void kiss_fftr_wrap(kiss_fftr_cfg st, const kiss_fftw_scalar *timedata, kiss_fftw_cpx *freqdata)
{
#ifdef FIXED_POINT
	const unsigned num_samples = st->substate->nfft;
    kiss_fft_scalar* scalarTemp;
    kiss_fft_cpx* cpxTemp;

    scalarTemp = malloc(sizeof(kiss_fft_scalar)*2*num_samples);
    cpxTemp = malloc(sizeof(kiss_fft_cpx)*num_samples);

    // Float to fixed conversion
    for(unsigned niSample = 0; niSample < 2 * num_samples; niSample++)
    {
        scalarTemp[niSample] = float_to_fixed32((kiss_fftw_scalar) timedata[niSample], INT_BITS);
    }

    kiss_fftr(st, scalarTemp, cpxTemp);

    // Fixed to float conversion
    for(unsigned niSample = 0; niSample < num_samples; niSample++)
    {
        freqdata[niSample].r = (kiss_fftw_scalar) fixed32_to_float(cpxTemp[niSample].r, INT_BITS);
        freqdata[niSample].i = (kiss_fftw_scalar) fixed32_to_float(cpxTemp[niSample].i, INT_BITS);
    }

    free(scalarTemp);
    free(cpxTemp);
#else
    kiss_fftr(st, timedata, freqdata);
#endif
}

void kiss_fftri_wrap(kiss_fftr_cfg st, const kiss_fftw_cpx *freqdata, kiss_fftw_scalar *timedata)
{
#ifdef FIXED_POINT
	const unsigned num_samples = st->substate->nfft;
    kiss_fft_scalar* scalarTemp;
    kiss_fft_cpx* cpxTemp;

    scalarTemp = malloc(sizeof(kiss_fft_scalar)*2*num_samples);
    cpxTemp = malloc(sizeof(kiss_fft_cpx)*num_samples);

    // Float to fixed conversion
    for(unsigned niSample = 0; niSample < num_samples; niSample++)
    {
        cpxTemp[niSample].r = float_to_fixed32((kiss_fftw_scalar) freqdata[niSample].r, INT_BITS);
        cpxTemp[niSample+1].i = float_to_fixed32((kiss_fftw_scalar) freqdata[niSample].i, INT_BITS);
    }

    kiss_fftri(st, cpxTemp, scalarTemp);

    // Fixed to float conversion
    for(unsigned niSample = 0; niSample < 2 * num_samples; niSample++)
    {
        timedata[niSample] = (kiss_fftw_scalar) fixed32_to_float(scalarTemp[niSample], INT_BITS);
    }

    free(scalarTemp);
    free(cpxTemp);
#else
    kiss_fftri(st, freqdata, timedata);
#endif
}