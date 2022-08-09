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

    // TODO: REMOVE AFTER CHECKING FXP CORRECTNESS
    kiss_fftw_scalar sample_1 = timedata[0];
    kiss_fftw_scalar sample_2 = timedata[1];

    printf("sample_1 = %f sample_2 = %f\n", sample_1, sample_2);

    kiss_fft_scalar sample_1_fx = float_to_fixed32((kiss_fftw_scalar) sample_1, INT_BITS);
    kiss_fft_scalar sample_2_fx = float_to_fixed32((kiss_fftw_scalar) sample_2, INT_BITS);

    printf("sample_1_fx = %x sample_2_fx = %x\n", sample_1_fx, sample_2_fx);

    kiss_fft_cpx sample_1_cpx, sample_2_cpx;
    sample_1_cpx.r = sample_1_fx;
    sample_1_cpx.i = sample_2_fx;
    sample_2_cpx.r = sample_1_fx;
    sample_2_cpx.i = sample_2_fx;

    C_FIXDIV(sample_1_cpx, 2);

    printf("After div sample_1_cpx = R:%x I:%x\n", sample_1_cpx.r, sample_1_cpx.i);
    kiss_fftw_scalar div_fp_r = (kiss_fftw_scalar) fixed32_to_float(sample_1_cpx.r, INT_BITS);
    kiss_fftw_scalar div_fp_i = (kiss_fftw_scalar) fixed32_to_float(sample_1_cpx.i, INT_BITS);
    printf("Fixed to float = R:%f I:%f\n", div_fp_r, div_fp_i);
    kiss_fftw_scalar div_exp_r = sample_1/2;
    kiss_fftw_scalar div_exp_i = sample_2/2;
    printf("Expected = R:%f I:%f\n", div_exp_r, div_exp_i);
    
    C_MULBYSCALAR(sample_1_cpx, float_to_fixed32((kiss_fftw_scalar) 2, INT_BITS));

    printf("After mulby sample_1_cpx = R:%x I:%x\n", sample_1_cpx.r, sample_1_cpx.i);
    kiss_fftw_scalar mulby_fp_r = (kiss_fftw_scalar) fixed32_to_float(sample_1_cpx.r, INT_BITS);
    kiss_fftw_scalar mulby_fp_i = (kiss_fftw_scalar) fixed32_to_float(sample_1_cpx.i, INT_BITS);
    printf("Fixed to float = R:%f I:%f\n", mulby_fp_r, mulby_fp_i);
    kiss_fftw_scalar mulby_exp_r = div_exp_r*2;
    kiss_fftw_scalar mulby_exp_i = div_exp_i*2;
    printf("Expected = R:%f I:%f\n", mulby_exp_r, mulby_exp_i);

    kiss_fft_cpx mul_cpx;
    C_MUL(mul_cpx, sample_1_cpx, sample_2_cpx);

    printf("mul_cpx = R:%x I:%x\n", mul_cpx.r, mul_cpx.i);
    kiss_fftw_scalar mul_fp_r = (kiss_fftw_scalar) fixed32_to_float(mul_cpx.r, INT_BITS);
    kiss_fftw_scalar mul_fp_i = (kiss_fftw_scalar) fixed32_to_float(mul_cpx.i, INT_BITS);
    printf("Fixed to float = R:%f I:%f\n", mul_fp_r, mul_fp_i);
    kiss_fftw_scalar mul_exp_r = (mulby_exp_r*sample_1) - (mulby_exp_i*sample_2);
    kiss_fftw_scalar mul_exp_i = (mulby_exp_r*sample_2) + (mulby_exp_i*sample_1);
    printf("Expected = R:%f I:%f\n", mul_exp_r, mul_exp_i);

    kiss_fft_cpx sub_cpx;
    C_SUB(sub_cpx, mul_cpx, sample_1_cpx);

    printf("sub_cpx = R:%x I:%x\n", sub_cpx.r, sub_cpx.i);
    kiss_fftw_scalar sub_fp_r = (kiss_fftw_scalar) fixed32_to_float(sub_cpx.r, INT_BITS);
    kiss_fftw_scalar sub_fp_i = (kiss_fftw_scalar) fixed32_to_float(sub_cpx.i, INT_BITS);
    printf("Fixed to float = R:%f I:%f\n", sub_fp_r, sub_fp_i);
    kiss_fftw_scalar sub_exp_r = mul_exp_r - mulby_exp_r;
    kiss_fftw_scalar sub_exp_i = mul_exp_i - mulby_exp_i;
    printf("Expected = R:%f I:%f\n", sub_exp_r, sub_exp_i);

    C_ADDTO(sub_cpx, sample_2_cpx);

    printf("sub_cpx = R:%x I:%x\n", sub_cpx.r, sub_cpx.i);
    sub_fp_r = (kiss_fftw_scalar) fixed32_to_float(sub_cpx.r, INT_BITS);
    sub_fp_i = (kiss_fftw_scalar) fixed32_to_float(sub_cpx.i, INT_BITS);
    printf("Fixed to float = R:%f I:%f\n", sub_fp_r, sub_fp_i);
    kiss_fftw_scalar addto_exp_r = sub_exp_r + sample_1;
    kiss_fftw_scalar addto_exp_i = sub_exp_i + sample_2;
    printf("Expected = R:%f I:%f\n", addto_exp_r, addto_exp_i);

    kiss_fft_cpx add_cpx;
    C_ADD(add_cpx, sub_cpx, sample_2_cpx);

    printf("add_cpx = R:%x I:%x\n", add_cpx.r, add_cpx.i);
    kiss_fftw_scalar add_fp_r = (kiss_fftw_scalar) fixed32_to_float(add_cpx.r, INT_BITS);
    kiss_fftw_scalar add_fp_i = (kiss_fftw_scalar) fixed32_to_float(add_cpx.i, INT_BITS);
    printf("Fixed to float = R:%f I:%f\n", add_fp_r, add_fp_i);
    kiss_fftw_scalar add_exp_r = addto_exp_r + sample_1;
    kiss_fftw_scalar add_exp_i = addto_exp_i + sample_2;
    printf("Expected = R:%f I:%f\n", add_exp_r, add_exp_i);

    kiss_fft_scalar half_cpx;
    half_cpx = HALF_OF(sample_2_fx);

    printf("half_cpx = %x\n", half_cpx);
    kiss_fftw_scalar half_fp = (kiss_fftw_scalar) fixed32_to_float(half_cpx, INT_BITS);
    printf("Fixed to float = %f\n", half_fp);
    kiss_fftw_scalar half_exp_i = sample_2/2;
    printf("Expected = %f\n", half_exp_i);

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