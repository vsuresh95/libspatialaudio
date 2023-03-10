/*############################################################################*/
/*#                                                                          #*/
/*#  Ambisonic C++ Library                                                   #*/
/*#  CAmbisonicBase - Ambisonic Base                                         #*/
/*#  Copyright © 2007 Aristotel Digenis                                      #*/
/*#                                                                          #*/
/*#  Filename:      AmbisonicBase.cpp                                        #*/
/*#  Version:       0.1                                                      #*/
/*#  Date:          19/05/2007                                               #*/
/*#  Author(s):     Aristotel Digenis                                        #*/
/*#  Licence:       MIT                                                      #*/
/*#                                                                          #*/
/*############################################################################*/


#include "AmbisonicBase.h"

CAmbisonicBase::CAmbisonicBase()
    : m_nOrder(0)
    , m_b3D(0)
    , m_nChannelCount(0)
    , m_bOpt(0)
{
}

unsigned CAmbisonicBase::GetOrder()
{
    return m_nOrder;
}

bool CAmbisonicBase::GetHeight()
{
    return m_b3D;
}

unsigned CAmbisonicBase::GetChannelCount()
{
    return m_nChannelCount;
}

bool CAmbisonicBase::Configure(unsigned nOrder, bool b3D, unsigned nMisc)
{
    StartTime = 0;
    EndTime = 0;

	for (unsigned i = 0; i < N_TIME_MARKERS; i++)
    	TotalTime[i] = 0;

    m_nOrder = nOrder;
    m_b3D = b3D;
    m_nChannelCount = OrderToComponents(m_nOrder, m_b3D);

    return true;
}

void CAmbisonicBase::StartCounter() {
	asm volatile (
		"li t0, 0;"
		"csrr t0, cycle;"
		"mv %0, t0"
		: "=r" (StartTime)
		:
		: "t0"
	);
}

void CAmbisonicBase::EndCounter(unsigned Index) {
	asm volatile (
		"li t0, 0;"
		"csrr t0, cycle;"
		"mv %0, t0"
		: "=r" (EndTime)
		:
		: "t0"
	);

    TotalTime[Index] += EndTime - StartTime;
}
