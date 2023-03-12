/*############################################################################*/
/*#                                                                          #*/
/*#  Ambisonic C++ Library                                                   #*/
/*#  CAmbisonicBinauralizer - Ambisonic Binauralizer                         #*/
/*#  Copyright © 2007 Aristotel Digenis                                      #*/
/*#  Copyright © 2017 Videolabs                                              #*/
/*#                                                                          #*/
/*#  Filename:      AmbisonicBinauralizer.h                                  #*/
/*#  Version:       0.2                                                      #*/
/*#  Date:          19/05/2007                                               #*/
/*#  Author(s):     Aristotel Digenis, Peter Stitt                           #*/
/*#  Licence:       LGPL                                                     #*/
/*#                                                                          #*/
/*############################################################################*/


#ifndef _AMBISONIC_BINAURALIZER_H
#define _AMBISONIC_BINAURALIZER_H

#include <string>
#include <vector>

#include "AmbisonicDecoder.h"
#include "AmbisonicEncoder.h"
#include "kiss_fftr.h"

#include "mit_hrtf.h"
#include "sofa_hrtf.h"

/// Ambisonic binauralizer

/** B-Format to binaural decoder. */

class CAmbisonicBinauralizer : public CAmbisonicBase
{
public:
    CAmbisonicBinauralizer();
    /**
        Re-create the object for the given configuration. Previous data is
        lost. The tailLength variable it updated with the number of taps
        used for the processing, and this can be used to offset the delay
        this causes. The function returns true if the call is successful.
    */
    virtual bool Configure(unsigned nOrder,
                           bool b3D,
                           unsigned nSampleRate,
                           unsigned nBlockSize,
                           unsigned& tailLength,
                           std::string HRTFPath = "");
    /**
        Resets members.
    */
    virtual void Reset();
    /**
        Refreshes coefficients.
    */
    virtual void Refresh();
    /**
        Decode B-Format to binaural feeds. There is no arguement for the number
        of samples to process, as this is determined by the nBlockSize argument
        in the constructor and Configure() function. It is the responsibility of
        program using this library to handle the blocks of the signal by FIFO
        buffers or other means.
    */
    void Process(CBFormat* pBFSrc, float** ppfDst);

    void PrintTimeInfo(unsigned factor);
    
    CAmbisonicDecoder m_AmbDecoder;

    unsigned m_nBlockSize;
    unsigned m_nTaps;
    unsigned m_nFFTSize;
    unsigned m_nFFTBins;
    float m_fFFTScaler;
    unsigned m_nOverlapLength;

    std::unique_ptr<struct kiss_fftr_state, decltype(&kiss_fftr_free)> m_pFFT_cfg;
    std::unique_ptr<struct kiss_fftr_state, decltype(&kiss_fftr_free)> m_pIFFT_cfg;
    kiss_fft_cpx*** m_ppcpFilters;
    kiss_fft_cpx* m_pcpScratch;

    float* m_pfScratchBufferA;
    float* m_pfScratchBufferB;
    float* m_pfScratchBufferC;
    float** m_pfOverlap;

    HRTF *getHRTF(unsigned nSampleRate, std::string HRTFPath);
    virtual void ArrangeSpeakers();
    virtual void AllocateBuffers();
};

#endif // _AMBISONIC_BINAURALIZER_H
