/*############################################################################*/
/*#                                                                          #*/
/*#  Ambisonic C++ Library                                                   #*/
/*#  CAmbisonicEncoderDist - Ambisonic Encoder with distance                 #*/
/*#  Copyright © 2007 Aristotel Digenis                                      #*/
/*#                                                                          #*/
/*#  Filename:      AmbisonicEncoderDist.cpp                                 #*/
/*#  Version:       0.1                                                      #*/
/*#  Date:          19/05/2007                                               #*/
/*#  Author(s):     Aristotel Digenis                                        #*/
/*#  Licence:       MIT                                                      #*/
/*#                                                                          #*/
/*############################################################################*/


#include <cassert>
#include "AmbisonicEncoderDist.h"

CAmbisonicEncoderDist::CAmbisonicEncoderDist()
{
    m_nSampleRate = 0;
    m_fDelay = 0.f;
    m_nDelay = 0;
    m_nDelayBufferLength = 0;
    m_pfDelayBuffer = 0;
    m_nIn = 0;
    m_nOutA = 0;
    m_nOutB = 0;
    m_fRoomRadius = 5.f;
    m_fInteriorGain = 0.f;
    m_fExteriorGain = 0.f;

    Configure(DEFAULT_ORDER, DEFAULT_HEIGHT, DEFAULT_SAMPLERATE);
}

CAmbisonicEncoderDist::~CAmbisonicEncoderDist()
{
    if (m_pfDelayBuffer) {
        delete [] m_pfDelayBuffer;
        m_pfDelayBuffer = nullptr;
    }
}

bool CAmbisonicEncoderDist::Configure(unsigned nOrder, bool b3D, unsigned nSampleRate)
{
    bool success = CAmbisonicEncoder::Configure(nOrder, b3D, 0);
    if (!success) {
        return false;
    }

    m_nSampleRate = nSampleRate;
    m_nDelayBufferLength = (unsigned)((float)knMaxDistance * m_nSampleRate / knSpeedOfSound + 0.5f);
    assert(m_nDelayBufferLength);

    if (m_pfDelayBuffer) {
        delete [] m_pfDelayBuffer;
        m_pfDelayBuffer = nullptr;
    }

    m_pfDelayBuffer = new float[m_nDelayBufferLength];
    // m_pfDelayBuffer .. (m_pfDelayBuffer + m_nDelayBufferLength) is an array of memory.
    // I want to ensure that this range of addresses does not contain the address of m_nDelayBufferLength
    // If it does, the memset on L69 will incorrectly zero out m_nDelayBufferLength.
    assert(reinterpret_cast<void*>(&m_nDelayBufferLength) <  reinterpret_cast<void*>(m_pfDelayBuffer) 
        || reinterpret_cast<void*>(&m_nDelayBufferLength) >= reinterpret_cast<void*>(m_pfDelayBuffer + m_nDelayBufferLength));
    Reset();
    
    return true;
}

void CAmbisonicEncoderDist::Reset()
{
    assert(m_nDelayBufferLength);
    memset(m_pfDelayBuffer, 0, m_nDelayBufferLength * sizeof(float));
    assert(m_nDelayBufferLength);
    // If the previous assertion fails, the modulo for m_nOutA and m_nOutB (dividing by 0) will fail with SIGFPE

    m_fDelay = m_polPosition.fDistance / knSpeedOfSound * m_nSampleRate + 0.5f;
    m_nDelay = (int)m_fDelay;
    m_fDelay -= m_nDelay;
    m_nIn = 0;
    m_nOutA = (m_nIn - m_nDelay + m_nDelayBufferLength) % m_nDelayBufferLength;
    m_nOutB = (m_nOutA + 1) % m_nDelayBufferLength;
}

void CAmbisonicEncoderDist::Refresh()
{
    CAmbisonicEncoder::Refresh();

    m_fDelay = fabs(m_polPosition.fDistance) / knSpeedOfSound * m_nSampleRate; //TODO abs() sees float as int!
    m_nDelay = (int)m_fDelay;
    m_fDelay -= m_nDelay;
    m_nOutA = (m_nIn - m_nDelay + m_nDelayBufferLength) % m_nDelayBufferLength;
    m_nOutB = (m_nOutA + 1) % m_nDelayBufferLength;

    //Source is outside speaker array
    if(fabs(m_polPosition.fDistance) >= m_fRoomRadius)
    {
        m_fInteriorGain    = (m_fRoomRadius / fabs(m_polPosition.fDistance)) / 2.f;
        m_fExteriorGain    = m_fInteriorGain;
    }
    else
    {
        m_fInteriorGain = (2.f - fabs(m_polPosition.fDistance) / m_fRoomRadius) / 2.f;
        m_fExteriorGain = (fabs(m_polPosition.fDistance) / m_fRoomRadius) / 2.f;
    }
}

void CAmbisonicEncoderDist::Process(float* pfSrc, unsigned nSamples, CBFormat* pfDst)
{
    unsigned niChannel = 0;
    unsigned niSample = 0;
    float fSrcSample = 0;

    for(niSample = 0; niSample < nSamples; niSample++)
    {
        //Store
        m_pfDelayBuffer[m_nIn] = pfSrc[niSample];
        //Read
        fSrcSample = m_pfDelayBuffer[m_nOutA] * (1.f - m_fDelay)
                    + m_pfDelayBuffer[m_nOutB] * m_fDelay;

        pfDst->m_ppfChannels[kW][niSample] = fSrcSample * m_fInteriorGain * m_pfCoeff[kW];

        fSrcSample *= m_fExteriorGain;
        for(niChannel = 1; niChannel < m_nChannelCount; niChannel++)
        {
            pfDst->m_ppfChannels[niChannel][niSample] = fSrcSample * m_pfCoeff[niChannel];
        }

        m_nIn = (m_nIn + 1) % m_nDelayBufferLength;
        m_nOutA = (m_nOutA + 1) % m_nDelayBufferLength;
        m_nOutB = (m_nOutB + 1) % m_nDelayBufferLength;
    }
}

void CAmbisonicEncoderDist::SetRoomRadius(float fRoomRadius)
{
    m_fRoomRadius = fRoomRadius;
}

float CAmbisonicEncoderDist::GetRoomRadius()
{
    return m_fRoomRadius;
}
