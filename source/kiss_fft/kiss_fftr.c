/*
Copyright (c) 2003-2004, Mark Borgerding

All rights reserved.

Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:

    * Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.
    * Neither the author nor the names of any contributors may be used to endorse or promote products derived from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#include "kiss_fftr.h"
#include "_kiss_fft_guts.h"

// struct kiss_fftr_state{
//     kiss_fft_cfg substate;
//     kiss_fft_cpx * tmpbuf;
//     kiss_fft_cpx * super_twiddles;
// #ifdef USE_SIMD
//     void * pad;
// #endif
// };

extern bool do_print;

extern void OffloadFFT(kiss_fft_scalar*, kiss_fft_cpx*);
extern void OffloadIFFT(kiss_fft_scalar*, kiss_fft_cpx*);

extern bool do_fft_ifft_offload;

kiss_fftr_cfg kiss_fftr_alloc(int nfft,int inverse_fft,void * mem,size_t * lenmem)
{
    int i;
    kiss_fftr_cfg st = NULL;
    size_t subsize, memneeded;

    if (nfft & 1) {
        fprintf(stderr,"Real FFT optimization must be even.\n");
        return NULL;
    }
    nfft >>= 1;

    kiss_fft_alloc (nfft, inverse_fft, NULL, &subsize);
    memneeded = sizeof(struct kiss_fftr_state) + subsize + sizeof(kiss_fft_cpx) * ( nfft * 3 / 2);

    if (lenmem == NULL) {
        st = (kiss_fftr_cfg) esp_alloc (memneeded);
    } else {
        if (*lenmem >= memneeded)
            st = (kiss_fftr_cfg) mem;
        *lenmem = memneeded;
    }
    if (!st)
        return NULL;

    st->substate = (kiss_fft_cfg) (st + 1); /*just beyond kiss_fftr_state struct */
    st->tmpbuf = (kiss_fft_cpx *) (((char *) st->substate) + subsize);
    st->super_twiddles = st->tmpbuf + nfft;
    kiss_fft_alloc(nfft, inverse_fft, st->substate, &subsize);

    for (i = 0; i < nfft/2; ++i) {
        double phase =
            -3.14159265358979323846264338327 * ((double) (i+1) / nfft + .5);
        if (inverse_fft)
            phase *= -1;
        kf_cexp (st->super_twiddles+i,phase);
    }
    return st;
}

void kiss_fftr(kiss_fftr_cfg st,const kiss_fftr_scalar *timedata_input,kiss_fft_cpx *freqdata)
{
    /* input buffer timedata is stored row-wise */
    int k,ncfft;
    kiss_fft_cpx fpnk,fpk,f1k,f2k,tw,tdc;

    if ( st->substate->inverse) {
        fprintf(stderr,"kiss fft usage error: improper alloc\n");
        exit(1);
    }

    ncfft = st->substate->nfft;

    // printf("FFT input:\n");
    // for(unsigned niSample = 0; niSample < ncfft; niSample++) {
    //     printf("%.9g ", timedata_input[niSample]);
    //     if ((niSample + 1) % 8 == 0) printf("\n");
    // }
    // printf("\n");

#ifdef FIXED_POINT
    kiss_fft_scalar *timedata = (kiss_fft_scalar *) esp_alloc(2 * ncfft * sizeof(kiss_fft_scalar));

    // printf("timedata:\n");
    for(unsigned niSample = 0; niSample < 2*ncfft; niSample++) {
        timedata[niSample] = float_to_fixed32(timedata_input[niSample], AUDIO_FX_IL);
        // printf("%08x ", timedata[niSample]);
        // if ((niSample + 1) % 8 == 0) printf("\n");
    }
    // printf("\n");
#else
    const kiss_fft_scalar *timedata = timedata_input;
#endif

    if (do_print) {
        printf("FFT Input\n");
        for(unsigned niSample = 0; niSample < 2 * ncfft; niSample++) {
            printf("%.6g ", timedata[niSample]);
            if ((niSample + 1) % 8 == 0) printf("\n");
        }
        printf("\n");
    }
    /*perform the parallel fft of two real signals packed in real,imag*/
    
    if (DO_FFT_OFFLOAD && do_fft_ifft_offload) {
        OffloadFFT((kiss_fft_scalar *) timedata, st->tmpbuf);
    } else {
        kiss_fft( st->substate , (const kiss_fft_cpx*)timedata, st->tmpbuf );
    }
    /* The real part of the DC element of the frequency spectrum in st->tmpbuf
     * contains the sum of the even-numbered elements of the input time sequence
     * The imag part is the sum of the odd-numbered elements
     *
     * The sum of tdc.r and tdc.i is the sum of the input time sequence. 
     *      yielding DC of input time sequence
     * The difference of tdc.r - tdc.i is the sum of the input (dot product) [1,-1,1,-1... 
     *      yielding Nyquist bin of input time sequence
     */
 
//     printf("FFT output:\n");
//     for(unsigned niSample = 0; niSample < ncfft; niSample++) {
// #ifdef FIXED_POINT
//         printf("%.9g ", fixed32_to_float(st->tmpbuf[niSample].r, AUDIO_FX_IL));
//         printf("%.9g ", fixed32_to_float(st->tmpbuf[niSample].i, AUDIO_FX_IL));
// #else
//         printf("%.9g ", st->tmpbuf[niSample].r);
//         printf("%.9g ", st->tmpbuf[niSample].i);
// #endif
//         if ((niSample + 1) % 4 == 0) printf("\n");
//     }
//     printf("\n");
        
    if (do_print) {
        printf("FFT Output\n");
        for(unsigned niSample = 0; niSample < ncfft; niSample++) {
            printf("%.6g ", st->tmpbuf[niSample].r);
            printf("%.6g ", st->tmpbuf[niSample].i);
            if ((niSample + 1) % 4 == 0) printf("\n");
        }
        printf("\n");
    }
 
    tdc.r = st->tmpbuf[0].r;
    tdc.i = st->tmpbuf[0].i;
    C_FIXDIV(tdc,2);
    CHECK_OVERFLOW_OP(tdc.r ,+, tdc.i);
    CHECK_OVERFLOW_OP(tdc.r ,-, tdc.i);
    freqdata[0].r = tdc.r + tdc.i;
    freqdata[ncfft].r = tdc.r - tdc.i;
#ifdef USE_SIMD    
    freqdata[ncfft].i = freqdata[0].i = _mm_set1_ps(0);
#else
    freqdata[ncfft].i = freqdata[0].i = 0;
#endif

    for ( k=1;k <= ncfft/2 ; ++k ) {
        fpk    = st->tmpbuf[k]; 
        fpnk.r =   st->tmpbuf[ncfft-k].r;
        fpnk.i = - st->tmpbuf[ncfft-k].i;
        C_FIXDIV(fpk,2);
        C_FIXDIV(fpnk,2);

        C_ADD( f1k, fpk , fpnk );
        C_SUB( f2k, fpk , fpnk );
        C_MUL( tw , f2k , st->super_twiddles[k-1]);

        freqdata[k].r = HALF_OF(f1k.r + tw.r);
        freqdata[k].i = HALF_OF(f1k.i + tw.i);
        freqdata[ncfft-k].r = HALF_OF(f1k.r - tw.r);
        freqdata[ncfft-k].i = HALF_OF(tw.i - f1k.i);
    }

// #ifdef FIXED_POINT
//     printf("freqdata:\n");
//     for(unsigned niSample = 0; niSample < ncfft+1; niSample++) {
//         printf("%08x ", freqdata[niSample].r);
//         printf("%08x ", freqdata[niSample].i);
//         if ((niSample + 1) % 4 == 0) printf("\n");
//     }
//     printf("\n");
// #endif

#ifdef FIXED_POINT
    esp_free(timedata);
#endif
}

void kiss_fftri(kiss_fftr_cfg st,const kiss_fft_cpx *freqdata,kiss_fftr_scalar *timedata_output)
{
    /* input buffer timedata is stored row-wise */
    int k, ncfft;

    if (st->substate->inverse == 0) {
        fprintf (stderr, "kiss fft usage error: improper alloc\n");
        exit (1);
    }

    ncfft = st->substate->nfft;

#ifdef FIXED_POINT
    kiss_fft_scalar *timedata = (kiss_fft_scalar *) esp_alloc(2 * ncfft * sizeof(kiss_fft_scalar));
#else
    kiss_fft_scalar *timedata = timedata_output;
#endif

// #ifdef FIXED_POINT
//     printf("freqdata:\n");
//     for(unsigned niSample = 0; niSample < ncfft+1; niSample++) {
//         printf("%08x ", freqdata[niSample].r);
//         printf("%08x ", freqdata[niSample].i);
//         if ((niSample + 1) % 4 == 0) printf("\n");
//     }
//     printf("\n");
// #endif

    st->tmpbuf[0].r = freqdata[0].r + freqdata[ncfft].r;
    st->tmpbuf[0].i = freqdata[0].r - freqdata[ncfft].r;
    C_FIXDIV(st->tmpbuf[0],2);

    for (k = 1; k <= ncfft / 2; ++k) {
        kiss_fft_cpx fk, fnkc, fek, fok, tmp;
        fk = freqdata[k];
        fnkc.r = freqdata[ncfft - k].r;
        fnkc.i = -freqdata[ncfft - k].i;
        C_FIXDIV( fk , 2 );
        C_FIXDIV( fnkc , 2 );

        C_ADD (fek, fk, fnkc);
        C_SUB (tmp, fk, fnkc);
        C_MUL (fok, tmp, st->super_twiddles[k-1]);
        C_ADD (st->tmpbuf[k],     fek, fok);
        C_SUB (st->tmpbuf[ncfft - k], fek, fok);
#ifdef USE_SIMD        
        st->tmpbuf[ncfft - k].i *= _mm_set1_ps(-1.0);
#else
        st->tmpbuf[ncfft - k].i *= -1;
#endif
    }

    if (do_print) {
        printf("FIR Output\n");
        for(unsigned niSample = 0; niSample < ncfft; niSample++) {
            printf("%.6g ", st->tmpbuf[niSample].r);
            printf("%.6g ", st->tmpbuf[niSample].i);
            if ((niSample + 1) % 4 == 0) printf("\n");
        }
        printf("\n");
    }
   
//     printf("IFFT input:\n");
//     for(unsigned niSample = 0; niSample < ncfft; niSample++) {
// #ifdef FIXED_POINT
//         printf("%.9g ", fixed32_to_float(st->tmpbuf[niSample].r, AUDIO_FX_IL));
//         printf("%.9g ", fixed32_to_float(st->tmpbuf[niSample].i, AUDIO_FX_IL));
// #else
//         printf("%.9g ", st->tmpbuf[niSample].r);
//         printf("%.9g ", st->tmpbuf[niSample].i);
// #endif
//         if ((niSample + 1) % 4 == 0) printf("\n");
//     }
//     printf("\n"); 

    if (DO_IFFT_OFFLOAD && do_fft_ifft_offload) {
        OffloadIFFT(timedata, st->tmpbuf);
    } else {
        kiss_fft (st->substate, st->tmpbuf, (kiss_fft_cpx *) timedata);
    }

#ifdef FIXED_POINT
    for(unsigned niSample = 0; niSample < 2*ncfft; niSample++) {
        timedata_output[niSample] = fixed32_to_float(timedata[niSample], AUDIO_FX_IL);
    }
#endif

    // printf("IFFT output:\n");
    // for(unsigned niSample = 0; niSample < ncfft; niSample++) {
    //     printf("%.9g ", timedata_output[niSample]);
    //     if ((niSample + 1) % 8 == 0) printf("\n");
    // }
    // printf("\n");

    if (do_print) {
        printf("IFFT Output\n");
        for(unsigned niSample = 0; niSample < 2 * ncfft; niSample++) {
            printf("%.6g ", timedata[niSample]);
            if ((niSample + 1) % 8 == 0) printf("\n");
        }
        printf("\n");
    }

#ifdef FIXED_POINT
    esp_free(timedata);
#endif
}
