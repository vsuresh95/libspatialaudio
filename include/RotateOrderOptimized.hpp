#ifndef ROTATE_ORDER_OPT
#define ROTATE_ORDER_OPT

// Coherence defines to different modes in ESP and Spandex.
#define QUAUX(X) #X
#define QU(X) QUAUX(X)

#define ESP_NON_COHERENT_DMA 3
#define ESP_LLC_COHERENT_DMA 2
#define ESP_COHERENT_DMA 1
#define ESP_BASELINE_MESI 0

#define SPX_OWNER_PREDICTION 3
#define SPX_WRITE_THROUGH_FWD 2
#define SPX_BASELINE_REQV 1
#define SPX_BASELINE_MESI 0

// Choosing the read and write code for Extended ASM functions.
#define READ_CODE 0x0002B30B
#define WRITE_CODE 0x0062B02B

#if (IS_ESP == 1)
#define READ_CODE_REQV READ_CODE
#define READ_CODE_REQODATA READ_CODE
#define WRITE_CODE_WTFWD WRITE_CODE
#else
#if (COH_MODE == SPX_OWNER_PREDICTION)
#define READ_CODE_REQV 0x2002B30B
#define READ_CODE_REQODATA 0x4002B30B
#define WRITE_CODE_WTFWD 0x2262B82B
#elif (COH_MODE == SPX_WRITE_THROUGH_FWD)
#define READ_CODE_REQV 0x2002B30B
#define READ_CODE_REQODATA 0x4002B30B
#define WRITE_CODE_WTFWD 0x2062B02B
#elif (COH_MODE == SPX_BASELINE_REQV)
#define READ_CODE_REQV 0x2002B30B
#define READ_CODE_REQODATA 0x4002B30B
#define WRITE_CODE_WTFWD WRITE_CODE
#else
#define READ_CODE_REQV READ_CODE
#define READ_CODE_REQODATA READ_CODE
#define WRITE_CODE_WTFWD WRITE_CODE
#endif
#endif

// Use float for CPU data
typedef float audio_t;

typedef union
{
  struct
  {
    audio_t value_32_1;
    audio_t value_32_2;
  };
  int64_t value_64;
} audio_token_t;

// Helper function to perform memory write using Spandex request types.
// By default, the writes are regular stores.
// See CohDefines.hpp for definition of WRITE_CODE.
static inline void write_mem (void* dst, int64_t value_64)
{
	asm volatile (
		"mv t0, %0;"
		"mv t1, %1;"
		".word " QU(WRITE_CODE)
		:
		: "r" (dst), "r" (value_64)
		: "t0", "t1", "memory"
	);
}

// Helper function to perform memory write using Spandex request types.
// By default, the writes are regular stores.
// See CohDefines.hpp for definition of WRITE_CODE.
static inline void write_mem_wtfwd (void* dst, int64_t value_64)
{
	asm volatile (
		"mv t0, %0;"
		"mv t1, %1;"
		".word " QU(WRITE_CODE_WTFWD)
		:
		: "r" (dst), "r" (value_64)
		: "t0", "t1", "memory"
	);
}

// Helper function to perform memory read using Spandex request types.
// By default, the reads are regular loads.
// See CohDefines.hpp for definition of READ_CODE.
static inline int64_t read_mem (void* dst)
{
	int64_t value_64;

	asm volatile (
		"mv t0, %1;"
		".word " QU(READ_CODE) ";"
		"mv %0, t1"
		: "=r" (value_64)
		: "r" (dst)
		: "t0", "t1", "memory"
	);

	return value_64;
}

// Helper function to perform memory read using Spandex request types.
// By default, the reads are regular loads.
// See CohDefines.hpp for definition of READ_CODE.
static inline int64_t read_mem_reqv (void* dst)
{
	int64_t value_64;

	asm volatile (
		"mv t0, %1;"
		".word " QU(READ_CODE_REQV) ";"
		"mv %0, t1"
		: "=r" (value_64)
		: "r" (dst)
		: "t0", "t1", "memory"
	);

	return value_64;
}

// Helper function to perform memory read using Spandex request types.
// By default, the reads are regular loads.
// See CohDefines.hpp for definition of READ_CODE.
static inline int64_t read_mem_reqodata (void* dst)
{
	int64_t value_64;

	asm volatile (
		"mv t0, %1;"
		".word " QU(READ_CODE_REQODATA) ";"
		"mv %0, t1"
		: "=r" (value_64)
		: "r" (dst)
		: "t0", "t1", "memory"
	);

	return value_64;
}

void CAmbisonicProcessor::ProcessOrder1_3D_Optimized(CBFormat* pBFSrcDst, unsigned nSamples)
{
    /* Rotations are performed in the following order:
        1 - rotation around the z-axis
        2 - rotation around the *new* y-axis (y')
        3 - rotation around the new z-axis (z'')
    This is different to the rotations obtained from the video, which are around z, y' then x''.
    The rotation equations used here work for third order. However, for higher orders a recursive algorithm
    should be considered.*/
	
    audio_t* src_kX;
    audio_t* src_kY;
    audio_t* src_kZ;

	audio_token_t SrcData_kX_0, SrcData_kX_1;
    audio_token_t SrcData_kY_0, SrcData_kY_1;
    audio_token_t SrcData_kZ_0, SrcData_kZ_1;

	src_kX = pBFSrcDst->m_ppfChannels[kX];
	src_kY = pBFSrcDst->m_ppfChannels[kY];
	src_kZ = pBFSrcDst->m_ppfChannels[kZ];

    asm volatile ("fence w, w");

    for(unsigned niSample = 0; niSample < nSamples; niSample+=2, src_kX+=2, src_kY+=2, src_kZ+=2)
    {
		SrcData_kX_0.value_64 = read_mem_reqv((void *) src_kX);
		SrcData_kY_0.value_64 = read_mem_reqv((void *) src_kY);
		SrcData_kZ_0.value_64 = read_mem_reqv((void *) src_kZ);

        // Alpha rotation
        SrcData_kY_1.value_32_1 = -SrcData_kX_0.value_32_1 * m_fSinAlpha + SrcData_kY_0.value_32_1 * m_fCosAlpha;
        SrcData_kY_1.value_32_2 = -SrcData_kX_0.value_32_2 * m_fSinAlpha + SrcData_kY_0.value_32_2 * m_fCosAlpha;

        SrcData_kZ_1.value_64 = SrcData_kZ_0.value_64;

        SrcData_kX_1.value_32_1 = SrcData_kX_0.value_32_1 * m_fCosAlpha + SrcData_kY_0.value_32_1 * m_fSinAlpha;
        SrcData_kX_1.value_32_2 = SrcData_kX_0.value_32_2 * m_fCosAlpha + SrcData_kY_0.value_32_2 * m_fSinAlpha;

        // Beta rotation
        SrcData_kY_0.value_64 = SrcData_kY_1.value_64;

        SrcData_kZ_0.value_32_1 = SrcData_kZ_1.value_32_1 * m_fCosBeta + SrcData_kX_1.value_32_1 * m_fSinBeta;
        SrcData_kZ_0.value_32_2 = SrcData_kZ_1.value_32_2 * m_fCosBeta + SrcData_kX_1.value_32_2 * m_fSinBeta;

        SrcData_kX_0.value_32_1 = SrcData_kX_1.value_32_1 * m_fCosBeta - SrcData_kZ_1.value_32_1 * m_fSinBeta;
        SrcData_kX_0.value_32_2 = SrcData_kX_1.value_32_2 * m_fCosBeta - SrcData_kZ_1.value_32_2 * m_fSinBeta;

        // Gamma rotation
        SrcData_kY_1.value_32_1 = -SrcData_kX_0.value_32_1 * m_fSinGamma + SrcData_kY_0.value_32_1 * m_fCosGamma;
        SrcData_kY_1.value_32_2 = -SrcData_kX_0.value_32_2 * m_fSinGamma + SrcData_kY_0.value_32_2 * m_fCosGamma;

        SrcData_kZ_1.value_64 = SrcData_kZ_0.value_64;

        SrcData_kX_1.value_32_1 = SrcData_kX_0.value_32_1 * m_fCosGamma + SrcData_kY_0.value_32_1 * m_fSinGamma;
        SrcData_kX_1.value_32_2 = SrcData_kX_0.value_32_2 * m_fCosGamma + SrcData_kY_0.value_32_2 * m_fSinGamma;

		write_mem_wtfwd((void *) src_kX, SrcData_kX_1.value_64);
		write_mem_wtfwd((void *) src_kY, SrcData_kY_1.value_64);
		write_mem_wtfwd((void *) src_kZ, SrcData_kZ_1.value_64);
    }
}

void CAmbisonicProcessor::ProcessOrder2_3D_Optimized(CBFormat* pBFSrcDst, unsigned nSamples)
{
    audio_t fSqrt3 = sqrt(3.f);
    audio_t fPowSinBeta2 = m_fSinBeta * m_fSinBeta;
	
    audio_t* src_kR;
    audio_t* src_kS;
    audio_t* src_kT;
    audio_t* src_kU;
    audio_t* src_kV;

    audio_token_t SrcData_kR_0, SrcData_kR_1;
    audio_token_t SrcData_kS_0, SrcData_kS_1;
    audio_token_t SrcData_kT_0, SrcData_kT_1;
    audio_token_t SrcData_kU_0, SrcData_kU_1;
    audio_token_t SrcData_kV_0, SrcData_kV_1;

	src_kR = pBFSrcDst->m_ppfChannels[kR];
	src_kS = pBFSrcDst->m_ppfChannels[kS];
	src_kT = pBFSrcDst->m_ppfChannels[kT];
	src_kU = pBFSrcDst->m_ppfChannels[kU];
	src_kV = pBFSrcDst->m_ppfChannels[kV];

    asm volatile ("fence w, w");

    for(unsigned niSample = 0; niSample < nSamples; niSample+=2, src_kR+=2, src_kS+=2, src_kT+=2, src_kU+=2, src_kV+=2)
    {
		SrcData_kR_0.value_64 = read_mem_reqv((void *) src_kR);
		SrcData_kS_0.value_64 = read_mem_reqv((void *) src_kS);
		SrcData_kT_0.value_64 = read_mem_reqv((void *) src_kT);
		SrcData_kU_0.value_64 = read_mem_reqv((void *) src_kU);
		SrcData_kV_0.value_64 = read_mem_reqv((void *) src_kV);

        // Alpha rotation
        SrcData_kV_1.value_32_1 = -SrcData_kU_0.value_32_1 * m_fSin2Alpha + SrcData_kV_0.value_32_1 * m_fCos2Alpha;
        SrcData_kV_1.value_32_2 = -SrcData_kU_0.value_32_2 * m_fSin2Alpha + SrcData_kV_0.value_32_2 * m_fCos2Alpha;

        SrcData_kT_1.value_32_1 = -SrcData_kS_0.value_32_1 * m_fSinAlpha + SrcData_kT_0.value_32_1 * m_fCosAlpha;
        SrcData_kT_1.value_32_2 = -SrcData_kS_0.value_32_2 * m_fSinAlpha + SrcData_kT_0.value_32_2 * m_fCosAlpha;

        SrcData_kR_1.value_64 = SrcData_kR_0.value_64;

        SrcData_kS_1.value_32_1 = SrcData_kS_0.value_32_1 * m_fCosAlpha + SrcData_kT_0.value_32_1 * m_fSinAlpha;
        SrcData_kS_1.value_32_2 = SrcData_kS_0.value_32_2 * m_fCosAlpha + SrcData_kT_0.value_32_2 * m_fSinAlpha;

        SrcData_kU_1.value_32_1 = SrcData_kU_0.value_32_1 * m_fCos2Alpha + SrcData_kV_0.value_32_1 * m_fSin2Alpha;
        SrcData_kU_1.value_32_2 = SrcData_kU_0.value_32_2 * m_fCos2Alpha + SrcData_kV_0.value_32_2 * m_fSin2Alpha;

        // Beta rotation
        SrcData_kV_0.value_32_1 = -m_fSinBeta * SrcData_kT_1.value_32_1 + m_fCosBeta * SrcData_kV_1.value_32_1;
        SrcData_kV_0.value_32_2 = -m_fSinBeta * SrcData_kT_1.value_32_2 + m_fCosBeta * SrcData_kV_1.value_32_2;

        SrcData_kT_0.value_32_1 = -m_fCosBeta * SrcData_kT_1.value_32_1 + m_fSinBeta * SrcData_kV_1.value_32_1;
        SrcData_kT_0.value_32_2 = -m_fCosBeta * SrcData_kT_1.value_32_2 + m_fSinBeta * SrcData_kV_1.value_32_2;

        SrcData_kR_0.value_32_1 = (0.75f * m_fCos2Beta + 0.25f) * SrcData_kR_1.value_32_1
                                + (0.5 * fSqrt3 * fPowSinBeta2) * SrcData_kU_1.value_32_1
                                + (fSqrt3 * m_fSinBeta * m_fCosBeta) * SrcData_kS_1.value_32_1;
        SrcData_kR_0.value_32_2 = (0.75f * m_fCos2Beta + 0.25f) * SrcData_kR_1.value_32_2
                                + (0.5 * fSqrt3 * fPowSinBeta2) * SrcData_kU_1.value_32_2
                                + (fSqrt3 * m_fSinBeta * m_fCosBeta) * SrcData_kS_1.value_32_2;

        SrcData_kS_0.value_32_1 = m_fCos2Beta * SrcData_kS_1.value_32_1
                                - fSqrt3 * m_fCosBeta * m_fSinBeta * SrcData_kR_1.value_32_1
                                + m_fCosBeta * m_fSinBeta * SrcData_kU_1.value_32_1;
        SrcData_kS_0.value_32_2 = m_fCos2Beta * SrcData_kS_1.value_32_2
                                - fSqrt3 * m_fCosBeta * m_fSinBeta * SrcData_kR_1.value_32_2
                                + m_fCosBeta * m_fSinBeta * SrcData_kU_1.value_32_2;

        SrcData_kU_0.value_32_1 = (0.25f * m_fCos2Beta + 0.75f) * SrcData_kU_1.value_32_1
                                - m_fCosBeta * m_fSinBeta * SrcData_kS_1.value_32_1
                                +0.5 * fSqrt3 * fPowSinBeta2 * SrcData_kR_1.value_32_1;
        SrcData_kU_0.value_32_2 = (0.25f * m_fCos2Beta + 0.75f) * SrcData_kU_1.value_32_2
                                - m_fCosBeta * m_fSinBeta * SrcData_kS_1.value_32_2
                                +0.5 * fSqrt3 * fPowSinBeta2 * SrcData_kR_1.value_32_2;

        // Gamma rotation
        SrcData_kV_1.value_32_1 = -SrcData_kU_0.value_32_1 * m_fSin2Gamma + SrcData_kV_0.value_32_1 * m_fCos2Gamma;
        SrcData_kV_1.value_32_2 = -SrcData_kU_0.value_32_2 * m_fSin2Gamma + SrcData_kV_0.value_32_2 * m_fCos2Gamma;

        SrcData_kT_1.value_32_1 = -SrcData_kS_0.value_32_1 * m_fSinGamma + SrcData_kT_0.value_32_1 * m_fCosGamma;
        SrcData_kT_1.value_32_2 = -SrcData_kS_0.value_32_2 * m_fSinGamma + SrcData_kT_0.value_32_2 * m_fCosGamma;

        SrcData_kR_1.value_64 = SrcData_kR_0.value_64;

        SrcData_kS_1.value_32_1 = SrcData_kS_0.value_32_1 * m_fCosGamma + SrcData_kT_0.value_32_1 * m_fSinGamma;
        SrcData_kS_1.value_32_2 = SrcData_kS_0.value_32_2 * m_fCosGamma + SrcData_kT_0.value_32_2 * m_fSinGamma;

        SrcData_kU_1.value_32_1 = SrcData_kU_0.value_32_1 * m_fCos2Gamma + SrcData_kV_0.value_32_1 * m_fSin2Gamma;
        SrcData_kU_1.value_32_2 = SrcData_kU_0.value_32_2 * m_fCos2Gamma + SrcData_kV_0.value_32_2 * m_fSin2Gamma;

		write_mem_wtfwd((void *) src_kR, SrcData_kR_1.value_64);
		write_mem_wtfwd((void *) src_kS, SrcData_kS_1.value_64);
		write_mem_wtfwd((void *) src_kT, SrcData_kT_1.value_64);
		write_mem_wtfwd((void *) src_kU, SrcData_kU_1.value_64);
		write_mem_wtfwd((void *) src_kV, SrcData_kV_1.value_64);
    }
}

void CAmbisonicProcessor::ProcessOrder3_3D_Optimized(CBFormat* pBFSrcDst, unsigned nSamples)
{
    /* (should move these somewhere that does recompute each time) */
    audio_t fSqrt3_2 = sqrt(3.f/2.f);
    audio_t fSqrt15 = sqrt(15.f);
    audio_t fSqrt5_2 = sqrt(5.f/2.f);

    audio_t fPowSinBeta2 = m_fSinBeta * m_fSinBeta;
    audio_t fPowSinBeta3 = m_fSinBeta * m_fSinBeta * m_fSinBeta;

    audio_t* src_kQ;
    audio_t* src_kO;
    audio_t* src_kM;
    audio_t* src_kK;
    audio_t* src_kL;
    audio_t* src_kN;
    audio_t* src_kP;

    audio_token_t SrcData_kQ_0, SrcData_kQ_1;
    audio_token_t SrcData_kO_0, SrcData_kO_1;
    audio_token_t SrcData_kM_0, SrcData_kM_1;
    audio_token_t SrcData_kK_0, SrcData_kK_1;
    audio_token_t SrcData_kL_0, SrcData_kL_1;
    audio_token_t SrcData_kN_0, SrcData_kN_1;
    audio_token_t SrcData_kP_0, SrcData_kP_1;

	src_kQ = pBFSrcDst->m_ppfChannels[kQ];
	src_kO = pBFSrcDst->m_ppfChannels[kO];
	src_kM = pBFSrcDst->m_ppfChannels[kM];
	src_kK = pBFSrcDst->m_ppfChannels[kK];
	src_kL = pBFSrcDst->m_ppfChannels[kL];
	src_kN = pBFSrcDst->m_ppfChannels[kN];
	src_kP = pBFSrcDst->m_ppfChannels[kP];

    asm volatile ("fence w, w");

    for(unsigned niSample = 0; niSample < nSamples; niSample+=2, src_kQ+=2, src_kO+=2, src_kM+=2, src_kK+=2, src_kL+=2, src_kN+=2, src_kP+=2)
    {
		SrcData_kQ_0.value_64 = read_mem_reqv((void *) src_kQ);
		SrcData_kO_0.value_64 = read_mem_reqv((void *) src_kO);
		SrcData_kM_0.value_64 = read_mem_reqv((void *) src_kM);
		SrcData_kK_0.value_64 = read_mem_reqv((void *) src_kK);
		SrcData_kL_0.value_64 = read_mem_reqv((void *) src_kL);
		SrcData_kN_0.value_64 = read_mem_reqv((void *) src_kN);
		SrcData_kP_0.value_64 = read_mem_reqv((void *) src_kP);

        // Alpha rotation
        SrcData_kQ_1.value_32_1 = -SrcData_kP_0.value_32_1 * m_fSin3Alpha + SrcData_kQ_0.value_32_1 * m_fCos3Alpha;
        SrcData_kQ_1.value_32_2 = -SrcData_kP_0.value_32_2 * m_fSin3Alpha + SrcData_kQ_0.value_32_2 * m_fCos3Alpha;

        SrcData_kO_1.value_32_1 = -SrcData_kN_0.value_32_1 * m_fSin2Alpha + SrcData_kO_0.value_32_1 * m_fCos2Alpha;
        SrcData_kO_1.value_32_2 = -SrcData_kN_0.value_32_2 * m_fSin2Alpha + SrcData_kO_0.value_32_2 * m_fCos2Alpha;

        SrcData_kM_1.value_32_1 = -SrcData_kL_0.value_32_1 * m_fSinAlpha + SrcData_kM_0.value_32_1 * m_fCosAlpha;
        SrcData_kM_1.value_32_2 = -SrcData_kL_0.value_32_2 * m_fSinAlpha + SrcData_kM_0.value_32_2 * m_fCosAlpha;

        SrcData_kK_1.value_64 = SrcData_kK_0.value_64;

        SrcData_kL_1.value_32_1 = SrcData_kL_0.value_32_1 * m_fCosAlpha + SrcData_kM_0.value_32_1 * m_fSinAlpha;
        SrcData_kL_1.value_32_2 = SrcData_kL_0.value_32_2 * m_fCosAlpha + SrcData_kM_0.value_32_2 * m_fSinAlpha;

        SrcData_kN_1.value_32_1 = SrcData_kN_0.value_32_1 * m_fCos2Alpha + SrcData_kO_0.value_32_1 * m_fSin2Alpha;
        SrcData_kN_1.value_32_2 = SrcData_kN_0.value_32_2 * m_fCos2Alpha + SrcData_kO_0.value_32_2 * m_fSin2Alpha;

        SrcData_kP_1.value_32_1 = SrcData_kP_0.value_32_1 * m_fCos3Alpha + SrcData_kQ_0.value_32_1 * m_fSin3Alpha;
        SrcData_kP_1.value_32_2 = SrcData_kP_0.value_32_2 * m_fCos3Alpha + SrcData_kQ_0.value_32_2 * m_fSin3Alpha;

        // Beta rotation
        SrcData_kQ_0.value_32_1 = 0.125f * SrcData_kQ_1.value_32_1 * (5.f + 3.f * m_fCos2Beta)
                                - fSqrt3_2 * SrcData_kO_1.value_32_1 * m_fCosBeta * m_fSinBeta
                                + 0.25f * fSqrt15 * SrcData_kM_1.value_32_1 * fPowSinBeta2;
        SrcData_kQ_0.value_32_2 = 0.125f * SrcData_kQ_1.value_32_2 * (5.f + 3.f * m_fCos2Beta)
                                - fSqrt3_2 * SrcData_kO_1.value_32_2 * m_fCosBeta * m_fSinBeta
                                + 0.25f * fSqrt15 * SrcData_kM_1.value_32_2 * fPowSinBeta2;

        SrcData_kO_0.value_32_1 = SrcData_kO_1.value_32_1 * m_fCos2Beta
                                - fSqrt5_2 * SrcData_kM_1.value_32_1 * m_fCosBeta * m_fSinBeta
                                + fSqrt3_2 * SrcData_kQ_1.value_32_1 * m_fCosBeta * m_fSinBeta;
        SrcData_kO_0.value_32_2 = SrcData_kO_1.value_32_2 * m_fCos2Beta
                                - fSqrt5_2 * SrcData_kM_1.value_32_2 * m_fCosBeta * m_fSinBeta
                                + fSqrt3_2 * SrcData_kQ_1.value_32_2 * m_fCosBeta * m_fSinBeta;

        SrcData_kM_0.value_32_1 = 0.125f * SrcData_kM_1.value_32_1 * (3.f + 5.f * m_fCos2Beta)
                                - fSqrt5_2 * SrcData_kO_1.value_32_1 *m_fCosBeta * m_fSinBeta
                                + 0.25f * fSqrt15 * SrcData_kQ_1.value_32_1 * fPowSinBeta2;
        SrcData_kM_0.value_32_2 = 0.125f * SrcData_kM_1.value_32_2 * (3.f + 5.f * m_fCos2Beta)
                                - fSqrt5_2 * SrcData_kO_1.value_32_2 *m_fCosBeta * m_fSinBeta
                                + 0.25f * fSqrt15 * SrcData_kQ_1.value_32_2 * fPowSinBeta2;

        SrcData_kK_0.value_32_1 = 0.25f * SrcData_kK_1.value_32_1 * m_fCosBeta * (-1.f + 15.f * m_fCos2Beta)
                                + 0.5f * fSqrt15 * SrcData_kN_1.value_32_1 * m_fCosBeta * fPowSinBeta2
                                + 0.5f * fSqrt5_2 * SrcData_kP_1.value_32_1 * fPowSinBeta3
                                + 0.125f * fSqrt3_2 * SrcData_kL_1.value_32_1 * (m_fSinBeta + 5.f * m_fSin3Beta);
        SrcData_kK_0.value_32_2 = 0.25f * SrcData_kK_1.value_32_2 * m_fCosBeta * (-1.f + 15.f * m_fCos2Beta)
                                + 0.5f * fSqrt15 * SrcData_kN_1.value_32_2 * m_fCosBeta * fPowSinBeta2
                                + 0.5f * fSqrt5_2 * SrcData_kP_1.value_32_2 * fPowSinBeta3
                                + 0.125f * fSqrt3_2 * SrcData_kL_1.value_32_2 * (m_fSinBeta + 5.f * m_fSin3Beta);

        SrcData_kL_0.value_32_1 = 0.0625f * SrcData_kL_1.value_32_1 * (m_fCosBeta + 15.f * m_fCos3Beta)
                                + 0.25f * fSqrt5_2 * SrcData_kN_1.value_32_1 * (1.f + 3.f * m_fCos2Beta) * m_fSinBeta
                                + 0.25f * fSqrt15 * SrcData_kP_1.value_32_1 * m_fCosBeta * fPowSinBeta2
                                - 0.125 * fSqrt3_2 * SrcData_kK_1.value_32_1 * (m_fSinBeta + 5.f * m_fSin3Beta);
        SrcData_kL_0.value_32_2 = 0.0625f * SrcData_kL_1.value_32_2 * (m_fCosBeta + 15.f * m_fCos3Beta)
                                + 0.25f * fSqrt5_2 * SrcData_kN_1.value_32_2 * (1.f + 3.f * m_fCos2Beta) * m_fSinBeta
                                + 0.25f * fSqrt15 * SrcData_kP_1.value_32_2 * m_fCosBeta * fPowSinBeta2
                                - 0.125 * fSqrt3_2 * SrcData_kK_1.value_32_2 * (m_fSinBeta + 5.f * m_fSin3Beta);

        SrcData_kN_0.value_32_1 = 0.125f * SrcData_kN_1.value_32_1 * (5.f * m_fCosBeta + 3.f * m_fCos3Beta)
                                + 0.25f * fSqrt3_2 * SrcData_kP_1.value_32_1 * (3.f + m_fCos2Beta) * m_fSinBeta
                                + 0.5f * fSqrt15 * SrcData_kK_1.value_32_1 * m_fCosBeta * fPowSinBeta2
                                + 0.125 * fSqrt5_2 * SrcData_kL_1.value_32_1 * (m_fSinBeta - 3.f * m_fSin3Beta);
        SrcData_kN_0.value_32_2 = 0.125f * SrcData_kN_1.value_32_2 * (5.f * m_fCosBeta + 3.f * m_fCos3Beta)
                                + 0.25f * fSqrt3_2 * SrcData_kP_1.value_32_2 * (3.f + m_fCos2Beta) * m_fSinBeta
                                + 0.5f * fSqrt15 * SrcData_kK_1.value_32_2 * m_fCosBeta * fPowSinBeta2
                                + 0.125 * fSqrt5_2 * SrcData_kL_1.value_32_2 * (m_fSinBeta - 3.f * m_fSin3Beta);

        SrcData_kP_0.value_32_1 = 0.0625f * SrcData_kP_1.value_32_1 * (15.f * m_fCosBeta + m_fCos3Beta)
                                - 0.25f * fSqrt3_2 * SrcData_kN_1.value_32_1 * (3.f + m_fCos2Beta) * m_fSinBeta
                                + 0.25f * fSqrt15 * SrcData_kL_1.value_32_1 * m_fCosBeta * fPowSinBeta2
                                - 0.5 * fSqrt5_2 * SrcData_kK_1.value_32_1 * fPowSinBeta3;
        SrcData_kP_0.value_32_2 = 0.0625f * SrcData_kP_1.value_32_2 * (15.f * m_fCosBeta + m_fCos3Beta)
                                - 0.25f * fSqrt3_2 * SrcData_kN_1.value_32_2 * (3.f + m_fCos2Beta) * m_fSinBeta
                                + 0.25f * fSqrt15 * SrcData_kL_1.value_32_2 * m_fCosBeta * fPowSinBeta2
                                - 0.5 * fSqrt5_2 * SrcData_kK_1.value_32_2 * fPowSinBeta3;

        // Gamma rotation
        SrcData_kQ_1.value_32_1 = -SrcData_kP_0.value_32_1 * m_fSin3Gamma + SrcData_kQ_0.value_32_1 * m_fCos3Gamma;
        SrcData_kQ_1.value_32_2 = -SrcData_kP_0.value_32_2 * m_fSin3Gamma + SrcData_kQ_0.value_32_2 * m_fCos3Gamma;

        SrcData_kO_1.value_32_1 = -SrcData_kN_0.value_32_1 * m_fSin2Gamma + SrcData_kO_0.value_32_1 * m_fCos2Gamma;
        SrcData_kO_1.value_32_2 = -SrcData_kN_0.value_32_2 * m_fSin2Gamma + SrcData_kO_0.value_32_2 * m_fCos2Gamma;

        SrcData_kM_1.value_32_1 = -SrcData_kL_0.value_32_1 * m_fSinGamma + SrcData_kM_0.value_32_1 * m_fCosGamma;
        SrcData_kM_1.value_32_2 = -SrcData_kL_0.value_32_2 * m_fSinGamma + SrcData_kM_0.value_32_2 * m_fCosGamma;

        SrcData_kK_1.value_64 = SrcData_kK_0.value_64;

        SrcData_kL_1.value_32_1 = SrcData_kL_0.value_32_1 * m_fCosGamma + SrcData_kM_0.value_32_1 * m_fSinGamma;
        SrcData_kL_1.value_32_2 = SrcData_kL_0.value_32_2 * m_fCosGamma + SrcData_kM_0.value_32_2 * m_fSinGamma;

        SrcData_kN_1.value_32_1 = SrcData_kN_0.value_32_1 * m_fCos2Gamma + SrcData_kO_0.value_32_1 * m_fSin2Gamma;
        SrcData_kN_1.value_32_2 = SrcData_kN_0.value_32_2 * m_fCos2Gamma + SrcData_kO_0.value_32_2 * m_fSin2Gamma;

        SrcData_kP_1.value_32_1 = SrcData_kP_0.value_32_1 * m_fCos3Gamma + SrcData_kQ_0.value_32_1 * m_fSin3Gamma;
        SrcData_kP_1.value_32_2 = SrcData_kP_0.value_32_2 * m_fCos3Gamma + SrcData_kQ_0.value_32_2 * m_fSin3Gamma;

		write_mem_wtfwd((void *) src_kQ, SrcData_kQ_1.value_64);
		write_mem_wtfwd((void *) src_kO, SrcData_kO_1.value_64);
		write_mem_wtfwd((void *) src_kM, SrcData_kM_1.value_64);
		write_mem_wtfwd((void *) src_kK, SrcData_kK_1.value_64);
		write_mem_wtfwd((void *) src_kL, SrcData_kL_1.value_64);
		write_mem_wtfwd((void *) src_kN, SrcData_kN_1.value_64);
		write_mem_wtfwd((void *) src_kP, SrcData_kP_1.value_64);
    }
}

#endif // ROTATE_ORDER_OPT