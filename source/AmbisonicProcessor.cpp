/*############################################################################*/
/*#                                                                          #*/
/*#  Ambisonic C++ Library                                                   #*/
/*#  CAmbisonicProcessor - Ambisonic Processor                               #*/
/*#  Copyright © 2007 Aristotel Digenis                                      #*/
/*#  Copyright © 2017 Videolabs                                              #*/
/*#                                                                          #*/
/*#  Filename:      AmbisonicProcessor.cpp                                   #*/
/*#  Version:       0.2                                                      #*/
/*#  Date:          19/05/2007                                               #*/
/*#  Author(s):     Aristotel Digenis, Peter Stitt                           #*/
/*#  Licence:       LGPL                                                     #*/
/*#                                                                          #*/
/*############################################################################*/


#include "AmbisonicProcessor.h"
#include <iostream>
#include <RotateOrderOptimized.hpp>
#include "_kiss_fft_guts.h"

extern void OffloadPsychoChain(CBFormat*, kiss_fft_cpx**, float**, unsigned, bool);
extern void OffloadPsychoPipeline(CBFormat*, kiss_fft_cpx**, float**, unsigned);
extern void OffloadPsychoFFTIFFT(CBFormat*, kiss_fft_cpx**, float**, unsigned, kiss_fftr_cfg, kiss_fftr_cfg);
extern void OffloadRotateOrder(CBFormat*);

CAmbisonicProcessor::CAmbisonicProcessor()
    : m_orientation(0, 0, 0)
{
    m_pfTempSample = nullptr;
    m_pfScratchBufferA = nullptr;
    m_pFFT_psych_cfg = nullptr;
    m_pIFFT_psych_cfg = nullptr;
    m_ppcpPsychFilters = nullptr;
    m_pcpScratch = nullptr;
    m_pfOverlap = nullptr;
}

CAmbisonicProcessor::~CAmbisonicProcessor()
{
    if(m_pfTempSample)
        delete [] m_pfTempSample;
    if(m_pFFT_psych_cfg)
        kiss_fftr_free(m_pFFT_psych_cfg);
    if(m_pIFFT_psych_cfg)
        kiss_fftr_free(m_pIFFT_psych_cfg);
}

bool CAmbisonicProcessor::Configure(unsigned nOrder, bool b3D, unsigned nBlockSize, unsigned nMisc)
{
    bool success = CAmbisonicBase::Configure(nOrder, b3D, nMisc);
    if(!success)
        return false;
    if(m_pfTempSample)
        delete [] m_pfTempSample;
    m_pfTempSample = new float[m_nChannelCount];
    memset(m_pfTempSample, 0, m_nChannelCount * sizeof(float));

    /* This bool should be set as a user option to turn optimisation on and off*/
    m_bOpt = true;

    // All optimisation filters have the same number of taps so take from the first order 3D impulse response arbitrarily
    unsigned nbTaps = sizeof(first_order_3D[0]) / sizeof(int16_t);

    m_nBlockSize = nBlockSize;
    m_nTaps = nbTaps;

    //What will the overlap size be?
    m_nOverlapLength = m_nBlockSize < m_nTaps ? m_nBlockSize - 1 : m_nTaps - 1;
    //How large does the FFT need to be
    m_nFFTSize = 1;
    while(m_nFFTSize < (m_nBlockSize + m_nTaps + m_nOverlapLength))
        m_nFFTSize <<= 1;
    //How many bins is that
    m_nFFTBins = m_nFFTSize / 2 + 1;
    //What do we need to scale the result of the iFFT by
    m_fFFTScaler = 1.f / m_nFFTSize;

    //Allocate buffers
    m_pfOverlap = (float **) esp_alloc(m_nChannelCount * sizeof(float *));
    for(unsigned i=0; i<m_nChannelCount; i++)
        m_pfOverlap[i] = (float *) esp_alloc(round_up(m_nOverlapLength, 2) * sizeof(float));

    m_pfScratchBufferA = (float *) esp_alloc(m_nFFTSize * sizeof(float));
    m_ppcpPsychFilters = (kiss_fft_cpx **) esp_alloc((m_nOrder+1) * sizeof(kiss_fft_cpx *));
    for(unsigned i = 0; i <= m_nOrder; i++)
        m_ppcpPsychFilters[i] = (kiss_fft_cpx *) esp_alloc(m_nFFTBins * sizeof(kiss_fft_cpx));

    m_pcpScratch = (kiss_fft_cpx *) esp_alloc(m_nFFTBins * sizeof(kiss_fft_cpx));

    //Allocate temporary buffers for retrieving taps of psychoacoustic opimisation filters
    std::vector<std::unique_ptr<float[]>> pfPsychIR;
    for(unsigned i = 0; i <= m_nOrder; i++)
    {
        pfPsychIR.emplace_back(new float[m_nTaps]);
    }

    Reset();

    //Allocate FFT and iFFT for new size
    m_pFFT_psych_cfg = kiss_fftr_alloc(m_nFFTSize, 0, 0, 0);
    m_pIFFT_psych_cfg = kiss_fftr_alloc(m_nFFTSize, 1, 0, 0);

    // get impulse responses for psychoacoustic optimisation based on playback system (2D or 3D) and playback order (1 to 3)
    //Convert from short to float representation
    for (unsigned i_m = 0; i_m <= m_nOrder; i_m++){
        for(unsigned i = 0; i < m_nTaps; i++)
            if(m_b3D){
                switch(m_nOrder){
                    case 1: pfPsychIR[i_m][i] = 2.f*first_order_3D[i_m][i] / 32767.f; break;
                    case 2: pfPsychIR[i_m][i] = 2.f*second_order_3D[i_m][i] / 32767.f; break;
                    case 3: pfPsychIR[i_m][i] = 2.f*third_order_3D[i_m][i] / 32767.f; break;
                }
                }
                else{
                    switch(m_nOrder){
                    case 1: pfPsychIR[i_m][i] = 2.f*first_order_2D[i_m][i] / 32767.f; break;
                    case 2: pfPsychIR[i_m][i] = 2.f*second_order_2D[i_m][i] / 32767.f; break;
                    case 3: pfPsychIR[i_m][i] = 2.f*third_order_2D[i_m][i] / 32767.f; break;
                }
            }
        // Convert the impulse responses to the frequency domain
        memcpy(m_pfScratchBufferA, pfPsychIR[i_m].get(), m_nTaps * sizeof(float));
        memset(&m_pfScratchBufferA[m_nTaps], 0, (m_nFFTSize - m_nTaps) * sizeof(float));
        kiss_fftr(m_pFFT_psych_cfg, m_pfScratchBufferA, m_ppcpPsychFilters[i_m]);
    }

    return true;
}

void CAmbisonicProcessor::Reset()
{
    for(unsigned i=0; i<m_nChannelCount; i++)
        memset(m_pfOverlap[i], 0, m_nOverlapLength * sizeof(float));
}

void CAmbisonicProcessor::Refresh()
{
    // Trig terms used multiple times in rotation equations
    m_fCosAlpha = cosf(m_orientation.fAlpha);
    m_fSinAlpha = sinf(m_orientation.fAlpha);
    m_fCosBeta = cosf(m_orientation.fBeta);
    m_fSinBeta = sinf(m_orientation.fBeta);
    m_fCosGamma = cosf(m_orientation.fGamma);
    m_fSinGamma = sinf(m_orientation.fGamma);
    m_fCos2Alpha = cosf(2.f * m_orientation.fAlpha);
    m_fSin2Alpha = sinf(2.f * m_orientation.fAlpha);
    m_fCos2Beta = cosf(2.f * m_orientation.fBeta);
    m_fSin2Beta = sinf(2.f * m_orientation.fBeta);
    m_fCos2Gamma = cosf(2.f * m_orientation.fGamma);
    m_fSin2Gamma = sinf(2.f * m_orientation.fGamma);
    m_fCos3Alpha = cosf(3.f * m_orientation.fAlpha);
    m_fSin3Alpha = sinf(3.f * m_orientation.fAlpha);
    m_fCos3Beta = cosf(3.f * m_orientation.fBeta);
    m_fSin3Beta = sinf(3.f * m_orientation.fBeta);
    m_fCos3Gamma = cosf(3.f * m_orientation.fGamma);
    m_fSin3Gamma = sinf(3.f * m_orientation.fGamma);
}

void CAmbisonicProcessor::SetOrientation(Orientation orientation)
{
    m_orientation = orientation;
}

Orientation CAmbisonicProcessor::GetOrientation()
{
    return m_orientation;
}

void CAmbisonicProcessor::Process(CBFormat* pBFSrcDst, unsigned nSamples)
{

    /* Rotate the sound scene based on the rotation angle from the 360 video*/
    /* Before the rotation we apply the psychoacoustic optimisation filters */
    if(m_bOpt)
    {
        if (DO_PP_CHAIN_OFFLOAD) {
            StartCounter();
            OffloadPsychoPipeline(pBFSrcDst, m_ppcpPsychFilters, m_pfOverlap, m_nOverlapLength);
            EndCounter(0);
        } else if (DO_CHAIN_OFFLOAD || DO_NP_CHAIN_OFFLOAD) {
            bool IsSharedMemory = (DO_NP_CHAIN_OFFLOAD) ? true : false;

            StartCounter();
            OffloadPsychoChain(pBFSrcDst, m_ppcpPsychFilters, m_pfOverlap, m_nOverlapLength, IsSharedMemory);
            EndCounter(0);
        } else if (DO_FFT_IFFT_OFFLOAD) {
            StartCounter();
            OffloadPsychoFFTIFFT(pBFSrcDst, m_ppcpPsychFilters, m_pfOverlap, m_nOverlapLength, m_pFFT_psych_cfg, m_pIFFT_psych_cfg);
            EndCounter(0);
        } else {
            ShelfFilterOrder(pBFSrcDst);
        }
    }
    else
    {
        // No filtering required
    }

    /* 3D Ambisonics input expected so perform 3D rotations */
    StartCounter();
    if (DO_ROTATE_OFFLOAD) {
        OffloadRotateOrder(pBFSrcDst);
    } else {
        if(m_nOrder >= 1) {
            ProcessOrder1_3D_Optimized(pBFSrcDst, nSamples);
        }
        if(m_nOrder >= 2) {
            ProcessOrder2_3D_Optimized(pBFSrcDst, nSamples);
        }
        if(m_nOrder >= 3) {
            ProcessOrder3_3D_Optimized(pBFSrcDst, nSamples);
        }
    }
    EndCounter(5);
}

void CAmbisonicProcessor::ProcessOrder1_3D(CBFormat* pBFSrcDst, unsigned nSamples)
{
    /* Rotations are performed in the following order:
        1 - rotation around the z-axis
        2 - rotation around the *new* y-axis (y')
        3 - rotation around the new z-axis (z'')
    This is different to the rotations obtained from the video, which are around z, y' then x''.
    The rotation equations used here work for third order. However, for higher orders a recursive algorithm
    should be considered.*/
    for(unsigned niSample = 0; niSample < nSamples; niSample++)
    {
        // Alpha rotation
        m_pfTempSample[kY] = -pBFSrcDst->m_ppfChannels[kX][niSample] * m_fSinAlpha
                            + pBFSrcDst->m_ppfChannels[kY][niSample] * m_fCosAlpha;
        m_pfTempSample[kZ] = pBFSrcDst->m_ppfChannels[kZ][niSample];
        m_pfTempSample[kX] = pBFSrcDst->m_ppfChannels[kX][niSample] * m_fCosAlpha
                            + pBFSrcDst->m_ppfChannels[kY][niSample] * m_fSinAlpha;

        // Beta rotation
        pBFSrcDst->m_ppfChannels[kY][niSample] = m_pfTempSample[kY];
        pBFSrcDst->m_ppfChannels[kZ][niSample] = m_pfTempSample[kZ] * m_fCosBeta
                            +  m_pfTempSample[kX] * m_fSinBeta;
        pBFSrcDst->m_ppfChannels[kX][niSample] = m_pfTempSample[kX] * m_fCosBeta
                            - m_pfTempSample[kZ] * m_fSinBeta;

        // Gamma rotation
        m_pfTempSample[kY] = -pBFSrcDst->m_ppfChannels[kX][niSample] * m_fSinGamma
                            + pBFSrcDst->m_ppfChannels[kY][niSample] * m_fCosGamma;
        m_pfTempSample[kZ] = pBFSrcDst->m_ppfChannels[kZ][niSample];
        m_pfTempSample[kX] = pBFSrcDst->m_ppfChannels[kX][niSample] * m_fCosGamma
                            + pBFSrcDst->m_ppfChannels[kY][niSample] * m_fSinGamma;

        pBFSrcDst->m_ppfChannels[kX][niSample] = m_pfTempSample[kX];
        pBFSrcDst->m_ppfChannels[kY][niSample] = m_pfTempSample[kY];
        pBFSrcDst->m_ppfChannels[kZ][niSample] = m_pfTempSample[kZ];
    }
}

void CAmbisonicProcessor::ProcessOrder2_3D(CBFormat* pBFSrcDst, unsigned nSamples)
{
    float fSqrt3 = sqrt(3.f);

    for(unsigned niSample = 0; niSample < nSamples; niSample++)
    {
        // Alpha rotation
        m_pfTempSample[kV] = - pBFSrcDst->m_ppfChannels[kU][niSample] * m_fSin2Alpha
                            + pBFSrcDst->m_ppfChannels[kV][niSample] * m_fCos2Alpha;
        m_pfTempSample[kT] = - pBFSrcDst->m_ppfChannels[kS][niSample] * m_fSinAlpha
                            + pBFSrcDst->m_ppfChannels[kT][niSample] * m_fCosAlpha;
        m_pfTempSample[kR] = pBFSrcDst->m_ppfChannels[kR][niSample];
        m_pfTempSample[kS] = pBFSrcDst->m_ppfChannels[kS][niSample] * m_fCosAlpha
                            + pBFSrcDst->m_ppfChannels[kT][niSample] * m_fSinAlpha;
        m_pfTempSample[kU] = pBFSrcDst->m_ppfChannels[kU][niSample] * m_fCos2Alpha
                            + pBFSrcDst->m_ppfChannels[kV][niSample] * m_fSin2Alpha;

        // Beta rotation
        pBFSrcDst->m_ppfChannels[kV][niSample] = -m_fSinBeta * m_pfTempSample[kT]
                                        + m_fCosBeta * m_pfTempSample[kV];
        pBFSrcDst->m_ppfChannels[kT][niSample] = -m_fCosBeta * m_pfTempSample[kT]
                                        + m_fSinBeta * m_pfTempSample[kV];
        pBFSrcDst->m_ppfChannels[kR][niSample] = (0.75f * m_fCos2Beta + 0.25f) * m_pfTempSample[kR]
                            + (0.5 * fSqrt3 * pow(m_fSinBeta,2.0) ) * m_pfTempSample[kU]
                            + (fSqrt3 * m_fSinBeta * m_fCosBeta) * m_pfTempSample[kS];
        pBFSrcDst->m_ppfChannels[kS][niSample] = m_fCos2Beta * m_pfTempSample[kS]
                            - fSqrt3 * m_fCosBeta * m_fSinBeta * m_pfTempSample[kR]
                            + m_fCosBeta * m_fSinBeta * m_pfTempSample[kU];
        pBFSrcDst->m_ppfChannels[kU][niSample] = (0.25f * m_fCos2Beta + 0.75f) * m_pfTempSample[kU]
                            - m_fCosBeta * m_fSinBeta * m_pfTempSample[kS]
                            +0.5 * fSqrt3 * pow(m_fSinBeta,2.0) * m_pfTempSample[kR];

        // Gamma rotation
        m_pfTempSample[kV] = - pBFSrcDst->m_ppfChannels[kU][niSample] * m_fSin2Gamma
                            + pBFSrcDst->m_ppfChannels[kV][niSample] * m_fCos2Gamma;
        m_pfTempSample[kT] = - pBFSrcDst->m_ppfChannels[kS][niSample] * m_fSinGamma
                            + pBFSrcDst->m_ppfChannels[kT][niSample] * m_fCosGamma;

        m_pfTempSample[kR] = pBFSrcDst->m_ppfChannels[kR][niSample];
        m_pfTempSample[kS] = pBFSrcDst->m_ppfChannels[kS][niSample] * m_fCosGamma
                            + pBFSrcDst->m_ppfChannels[kT][niSample] * m_fSinGamma;
        m_pfTempSample[kU] = pBFSrcDst->m_ppfChannels[kU][niSample] * m_fCos2Gamma
                            + pBFSrcDst->m_ppfChannels[kV][niSample] * m_fSin2Gamma;

        pBFSrcDst->m_ppfChannels[kR][niSample] = m_pfTempSample[kR];
        pBFSrcDst->m_ppfChannels[kS][niSample] = m_pfTempSample[kS];
        pBFSrcDst->m_ppfChannels[kT][niSample] = m_pfTempSample[kT];
        pBFSrcDst->m_ppfChannels[kU][niSample] = m_pfTempSample[kU];
        pBFSrcDst->m_ppfChannels[kV][niSample] = m_pfTempSample[kV];
    }
}

void CAmbisonicProcessor::ProcessOrder3_3D(CBFormat* pBFSrcDst, unsigned nSamples)
{
        /* (should move these somewhere that does recompute each time) */
        float fSqrt3_2 = sqrt(3.f/2.f);
        float fSqrt15 = sqrt(15.f);
        float fSqrt5_2 = sqrt(5.f/2.f);

    for(unsigned niSample = 0; niSample < nSamples; niSample++)
    {
        // Alpha rotation
        m_pfTempSample[kQ] = - pBFSrcDst->m_ppfChannels[kP][niSample] * m_fSin3Alpha
                            + pBFSrcDst->m_ppfChannels[kQ][niSample] * m_fCos3Alpha;
        m_pfTempSample[kO] = - pBFSrcDst->m_ppfChannels[kN][niSample] * m_fSin2Alpha
                            + pBFSrcDst->m_ppfChannels[kO][niSample] * m_fCos2Alpha;
        m_pfTempSample[kM] = - pBFSrcDst->m_ppfChannels[kL][niSample] * m_fSinAlpha
                            + pBFSrcDst->m_ppfChannels[kM][niSample] * m_fCosAlpha;
        m_pfTempSample[kK] = pBFSrcDst->m_ppfChannels[kK][niSample];
        m_pfTempSample[kL] = pBFSrcDst->m_ppfChannels[kL][niSample] * m_fCosAlpha
                            + pBFSrcDst->m_ppfChannels[kM][niSample] * m_fSinAlpha;
        m_pfTempSample[kN] = pBFSrcDst->m_ppfChannels[kN][niSample] * m_fCos2Alpha
                            + pBFSrcDst->m_ppfChannels[kO][niSample] * m_fSin2Alpha;
        m_pfTempSample[kP] = pBFSrcDst->m_ppfChannels[kP][niSample] * m_fCos3Alpha
                            + pBFSrcDst->m_ppfChannels[kQ][niSample] * m_fSin3Alpha;

        // Beta rotation
        pBFSrcDst->m_ppfChannels[kQ][niSample] = 0.125f * m_pfTempSample[kQ] * (5.f + 3.f*m_fCos2Beta)
                    - fSqrt3_2 * m_pfTempSample[kO] *m_fCosBeta * m_fSinBeta
                    + 0.25f * fSqrt15 * m_pfTempSample[kM] * pow(m_fSinBeta,2.0f);
        pBFSrcDst->m_ppfChannels[kO][niSample] = m_pfTempSample[kO] * m_fCos2Beta
                    - fSqrt5_2 * m_pfTempSample[kM] * m_fCosBeta * m_fSinBeta
                    + fSqrt3_2 * m_pfTempSample[kQ] * m_fCosBeta * m_fSinBeta;
        pBFSrcDst->m_ppfChannels[kM][niSample] = 0.125f * m_pfTempSample[kM] * (3.f + 5.f*m_fCos2Beta)
                    - fSqrt5_2 * m_pfTempSample[kO] *m_fCosBeta * m_fSinBeta
                    + 0.25f * fSqrt15 * m_pfTempSample[kQ] * pow(m_fSinBeta,2.0f);
        pBFSrcDst->m_ppfChannels[kK][niSample] = 0.25f * m_pfTempSample[kK] * m_fCosBeta * (-1.f + 15.f*m_fCos2Beta)
                    + 0.5f * fSqrt15 * m_pfTempSample[kN] * m_fCosBeta * pow(m_fSinBeta,2.f)
                    + 0.5f * fSqrt5_2 * m_pfTempSample[kP] * pow(m_fSinBeta,3.f)
                    + 0.125f * fSqrt3_2 * m_pfTempSample[kL] * (m_fSinBeta + 5.f * m_fSin3Beta);
        pBFSrcDst->m_ppfChannels[kL][niSample] = 0.0625f * m_pfTempSample[kL] * (m_fCosBeta + 15.f * m_fCos3Beta)
                    + 0.25f * fSqrt5_2 * m_pfTempSample[kN] * (1.f + 3.f * m_fCos2Beta) * m_fSinBeta
                    + 0.25f * fSqrt15 * m_pfTempSample[kP] * m_fCosBeta * pow(m_fSinBeta,2.f)
                    - 0.125 * fSqrt3_2 * m_pfTempSample[kK] * (m_fSinBeta + 5.f * m_fSin3Beta);
        pBFSrcDst->m_ppfChannels[kN][niSample] = 0.125f * m_pfTempSample[kN] * (5.f * m_fCosBeta + 3.f * m_fCos3Beta)
                    + 0.25f * fSqrt3_2 * m_pfTempSample[kP] * (3.f + m_fCos2Beta) * m_fSinBeta
                    + 0.5f * fSqrt15 * m_pfTempSample[kK] * m_fCosBeta * pow(m_fSinBeta,2.f)
                    + 0.125 * fSqrt5_2 * m_pfTempSample[kL] * (m_fSinBeta - 3.f * m_fSin3Beta);
        pBFSrcDst->m_ppfChannels[kP][niSample] = 0.0625f * m_pfTempSample[kP] * (15.f * m_fCosBeta + m_fCos3Beta)
                    - 0.25f * fSqrt3_2 * m_pfTempSample[kN] * (3.f + m_fCos2Beta) * m_fSinBeta
                    + 0.25f * fSqrt15 * m_pfTempSample[kL] * m_fCosBeta * pow(m_fSinBeta,2.f)
                    - 0.5 * fSqrt5_2 * m_pfTempSample[kK] * pow(m_fSinBeta,3.f);

        // Gamma rotation
        m_pfTempSample[kQ] = - pBFSrcDst->m_ppfChannels[kP][niSample] * m_fSin3Gamma
                            + pBFSrcDst->m_ppfChannels[kQ][niSample] * m_fCos3Gamma;
        m_pfTempSample[kO] = - pBFSrcDst->m_ppfChannels[kN][niSample] * m_fSin2Gamma
                            + pBFSrcDst->m_ppfChannels[kO][niSample] * m_fCos2Gamma;
        m_pfTempSample[kM] = - pBFSrcDst->m_ppfChannels[kL][niSample] * m_fSinGamma
                            + pBFSrcDst->m_ppfChannels[kM][niSample] * m_fCosGamma;
        m_pfTempSample[kK] = pBFSrcDst->m_ppfChannels[kK][niSample];
        m_pfTempSample[kL] = pBFSrcDst->m_ppfChannels[kL][niSample] * m_fCosGamma
                            + pBFSrcDst->m_ppfChannels[kM][niSample] * m_fSinGamma;
        m_pfTempSample[kN] = pBFSrcDst->m_ppfChannels[kN][niSample] * m_fCos2Gamma
                            + pBFSrcDst->m_ppfChannels[kO][niSample] * m_fSin2Gamma;
        m_pfTempSample[kP] = pBFSrcDst->m_ppfChannels[kP][niSample] * m_fCos3Gamma
                            + pBFSrcDst->m_ppfChannels[kQ][niSample] * m_fSin3Gamma;

        pBFSrcDst->m_ppfChannels[kQ][niSample] = m_pfTempSample[kQ];
        pBFSrcDst->m_ppfChannels[kO][niSample] = m_pfTempSample[kO];
        pBFSrcDst->m_ppfChannels[kM][niSample] = m_pfTempSample[kM];
        pBFSrcDst->m_ppfChannels[kK][niSample] = m_pfTempSample[kK];
        pBFSrcDst->m_ppfChannels[kL][niSample] = m_pfTempSample[kL];
        pBFSrcDst->m_ppfChannels[kN][niSample] = m_pfTempSample[kN];
        pBFSrcDst->m_ppfChannels[kP][niSample] = m_pfTempSample[kP];
    }
}

// ACN/SN3D is generally only ever produced for 3D Ambisonics.
// If 2D Ambisonics is required then these equations need to be modified (they can be found in the 3D code for the first Z-rotation).
// Generally, 2D-only rotations do not make sense for use with 360 degree videos.
/*
void CAmbisonicProcessor::ProcessOrder1_2D(CBFormat* pBFSrcDst, unsigned nSamples)
{
    for(unsigned niSample = 0; niSample < nSamples; niSample++)
    {
        //Yaw
        m_pfTempSample[kX] = pBFSrcDst->m_ppfChannels[kX][niSample] * m_fCosYaw
                            - pBFSrcDst->m_ppfChannels[kY][niSample] * m_fSinYaw;
        m_pfTempSample[kY] = pBFSrcDst->m_ppfChannels[kX][niSample] * m_fSinYaw
                            + pBFSrcDst->m_ppfChannels[kY][niSample] * m_fCosYaw;

        pBFSrcDst->m_ppfChannels[kX][niSample] = m_pfTempSample[kX];
        pBFSrcDst->m_ppfChannels[kY][niSample] = m_pfTempSample[kY];
    }
}

void CAmbisonicProcessor::ProcessOrder2_2D(CBFormat* pBFSrcDst, unsigned nSamples)
{
    for(unsigned niSample = 0; niSample < nSamples; niSample++)
    {
        //Yaw
        m_pfTempSample[kS] = pBFSrcDst->m_ppfChannels[kS][niSample] * m_fCosYaw
                            - pBFSrcDst->m_ppfChannels[kT][niSample] * m_fSinYaw;
        m_pfTempSample[kT] = pBFSrcDst->m_ppfChannels[kS][niSample] * m_fSinYaw
                            + pBFSrcDst->m_ppfChannels[kT][niSample] * m_fCosYaw;
        m_pfTempSample[kU] = pBFSrcDst->m_ppfChannels[kU][niSample] * m_fCos2Yaw
                            - pBFSrcDst->m_ppfChannels[kV][niSample] * m_fSin2Yaw;
        m_pfTempSample[kV] = pBFSrcDst->m_ppfChannels[kU][niSample] * m_fSin2Yaw
                            + pBFSrcDst->m_ppfChannels[kV][niSample] * m_fCos2Yaw;

        pBFSrcDst->m_ppfChannels[kS][niSample] = m_pfTempSample[kS];
        pBFSrcDst->m_ppfChannels[kT][niSample] = m_pfTempSample[kT];
        pBFSrcDst->m_ppfChannels[kU][niSample] = m_pfTempSample[kU];
        pBFSrcDst->m_ppfChannels[kV][niSample] = m_pfTempSample[kV];
    }
}

void CAmbisonicProcessor::ProcessOrder3_2D(CBFormat* pBFSrcDst, unsigned nSamples)
{
    //TODO
}
*/

void CAmbisonicProcessor::ShelfFilterOrder(CBFormat* pBFSrcDst)
{
    kiss_fft_cpx cpTemp;

    unsigned iChannelOrder = 0;

    // Pointers/structures to manage input/output data
	audio_token_t SrcData;
	audio_t* src;
	audio_token_t DstData;
	audio_t* dst;
	audio_token_t OverlapData;
	audio_t* overlap_dst;

	unsigned InitLength = m_nBlockSize;
	unsigned ZeroLength = m_nFFTSize - m_nBlockSize;
	unsigned ReadLength = m_nBlockSize;
	unsigned OverlapLength = round_up(m_nOverlapLength, 2);

    // Filter the Ambisonics channels
    // All  channels are filtered using linear phase FIR filters.
    // In the case of the 0th order signal (W channel) this takes the form of a delay
    // For all other channels shelf filters are used
    for(unsigned niChannel = 0; niChannel < m_nChannelCount; niChannel++)
    {
        iChannelOrder = int(sqrt(niChannel));    //get the order of the current channel

        src = pBFSrcDst->m_ppfChannels[niChannel];
        dst = m_pfScratchBufferA;

        StartCounter();
        // Copying from pBFSrcDst->m_ppfChannels[niChannel] to m_pfScratchBufferA
        for (unsigned niSample = 0; niSample < InitLength; niSample+=2, src+=2, dst+=2)
        {
            // Need to cast to void* for extended ASM code.
            SrcData.value_64 = read_mem_reqv((void *) src);
            write_mem_wtfwd((void *) dst, SrcData.value_64);
        }

        // Zeroing rest of m_pfScratchBufferA
        for (unsigned niSample = 0; niSample < ZeroLength; niSample+=2, dst+=2)
        {
            // Need to cast to void* for extended ASM code.
            SrcData.value_64 = 0;
            write_mem_wtfwd((void *) dst, SrcData.value_64);
        }
        EndCounter(0);

        // Convert from time domain back to frequency domain
        StartCounter();
        unsigned long long FFTPostProcTime = kiss_fftr(m_pFFT_psych_cfg, m_pfScratchBufferA, m_pcpScratch);
        EndCounter(1);

        // Perform the convolution in the frequency domain
        StartCounter();
        for(unsigned ni = 0; ni < m_nFFTBins; ni++)
        {
            C_MUL(cpTemp , m_pcpScratch[ni] , m_ppcpPsychFilters[iChannelOrder][ni]);
            m_pcpScratch[ni] = cpTemp;
        }
        EndCounter(2);

        // Convert from frequency domain back to time domain
        StartCounter();
        unsigned long long IFFTPreProcTime = kiss_fftri(m_pIFFT_psych_cfg, m_pcpScratch, m_pfScratchBufferA);
        EndCounter(3);

        src = m_pfScratchBufferA;
        dst = pBFSrcDst->m_ppfChannels[niChannel];
        overlap_dst = m_pfOverlap[niChannel];

        StartCounter();
        // First, we copy the output, scale it and account for the overlap
        // data from the previous block, for the same channel.
        for (unsigned niSample = 0; niSample < OverlapLength; niSample+=2, src+=2, dst+=2, overlap_dst+=2)
        {
            // Need to cast to void* for extended ASM code.
            SrcData.value_64 = read_mem_reqodata((void *) src);
            OverlapData.value_64 = read_mem_reqv((void *) overlap_dst);

            DstData.value_32_1 = OverlapData.value_32_1 + m_fFFTScaler * SrcData.value_32_1;
            DstData.value_32_2 = OverlapData.value_32_2 + m_fFFTScaler * SrcData.value_32_2;

            // Need to cast to void* for extended ASM code.
            write_mem_wtfwd((void *) dst, DstData.value_64);
        }

        // Second, we simply copy the output (with scaling) as we are outside the overlap range.
        for (unsigned niSample = OverlapLength; niSample < ReadLength; niSample+=2, src+=2, dst+=2)
        {
            // Need to cast to void* for extended ASM code.
            SrcData.value_64 = read_mem_reqodata((void *) src);

            DstData.value_32_1 = m_fFFTScaler * SrcData.value_32_1;
            DstData.value_32_2 = m_fFFTScaler * SrcData.value_32_2;

            // Need to cast to void* for extended ASM code.
            write_mem_wtfwd((void *) dst, DstData.value_64);
        }

        overlap_dst = m_pfOverlap[niChannel];

        // Last, we copy our output (with scaling) directly to the overlap buffer only.
        // This data will be used in the first loop for the next audio block.
        for (unsigned niSample = 0; niSample < OverlapLength; niSample+=2, src+=2, dst+=2, overlap_dst+=2)
        {
            // Need to cast to void* for extended ASM code.
            SrcData.value_64 = read_mem_reqodata((void *) src);

            OverlapData.value_32_1 = m_fFFTScaler * SrcData.value_32_1;
            OverlapData.value_32_2 = m_fFFTScaler * SrcData.value_32_2;

            // Need to cast to void* for extended ASM code.
            write_mem_wtfwd((void *) overlap_dst, OverlapData.value_64);
        }
        EndCounter(4);

        TotalTime[1] -= FFTPostProcTime;
        TotalTime[2] += FFTPostProcTime + IFFTPreProcTime;
        TotalTime[3] -= IFFTPreProcTime;
    }
}

void CAmbisonicProcessor::PrintTimeInfo(unsigned factor) {
    printf("Rotate Order\t\t = %llu\n", TotalTime[5]/factor);
    printf("\n");
    printf("---------------------------------------------\n");
    printf("PSYCHO-ACOUSTIC TIME\n");
    printf("---------------------------------------------\n");
    printf("Total Time\t\t = %llu\n", (TotalTime[0] + TotalTime[1] + TotalTime[2] + TotalTime[3] + TotalTime[4])/factor);

    if (!(DO_FFT_IFFT_OFFLOAD || DO_CHAIN_OFFLOAD || DO_NP_CHAIN_OFFLOAD || DO_PP_CHAIN_OFFLOAD)) {
        printf("\n");
        printf("Psycho Init Data\t = %llu\n", TotalTime[0]/factor);
        printf("Psycho FFT\t\t = %llu\n", TotalTime[1]/factor);
        printf("Psycho FIR\t\t = %llu\n", TotalTime[2]/factor);
        printf("Psycho IFFT\t\t = %llu\n", TotalTime[3]/factor);
        printf("Psycho Overlap\t\t = %llu\n", TotalTime[4]/factor);
    }
}
