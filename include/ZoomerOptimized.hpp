#ifndef ZOOMER_OPT
#define ZOOMER_OPT

// Coherence defines to different modes in ESP and Spandex.
#define QUAUX(X) #X
#define QU(X) QUAUX(X)

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

void CAmbisonicZoomer::ProcessOptimized(CBFormat *pBFSrcDst, unsigned nSamples)
{
    audio_t* src_0;
    audio_t* src_1;
    audio_t* src_2;
    audio_t* src_3;
    audio_t* src_4;
    audio_t* src_5;
    audio_t* src_6;
    audio_t* src_7;
    audio_t* src_8;
    audio_t* src_9;
    audio_t* src_10;
    audio_t* src_11;
    audio_t* src_12;
    audio_t* src_13;
    audio_t* src_14;
    audio_t* src_15;

    audio_token_t SrcData_0;
    audio_token_t SrcData_1;
    audio_token_t SrcData_2;
    audio_token_t SrcData_3;
    audio_token_t SrcData_4;
    audio_token_t SrcData_5;
    audio_token_t SrcData_6;
    audio_token_t SrcData_7;
    audio_token_t SrcData_8;
    audio_token_t SrcData_9;
    audio_token_t SrcData_10;
    audio_token_t SrcData_11;
    audio_token_t SrcData_12;
    audio_token_t SrcData_13;
    audio_token_t SrcData_14;
    audio_token_t SrcData_15;

    src_0 = pBFSrcDst->m_ppfChannels[0];
    src_1 = pBFSrcDst->m_ppfChannels[1];
    src_2 = pBFSrcDst->m_ppfChannels[2];
    src_3 = pBFSrcDst->m_ppfChannels[3];
    src_4 = pBFSrcDst->m_ppfChannels[4];
    src_5 = pBFSrcDst->m_ppfChannels[5];
    src_6 = pBFSrcDst->m_ppfChannels[6];
    src_7 = pBFSrcDst->m_ppfChannels[7];
    src_8 = pBFSrcDst->m_ppfChannels[8];
    src_9 = pBFSrcDst->m_ppfChannels[9];
    src_10 = pBFSrcDst->m_ppfChannels[10];
    src_11 = pBFSrcDst->m_ppfChannels[11];
    src_12 = pBFSrcDst->m_ppfChannels[12];
    src_13 = pBFSrcDst->m_ppfChannels[13];
    src_14 = pBFSrcDst->m_ppfChannels[14];
    src_15 = pBFSrcDst->m_ppfChannels[15];

    audio_t ZoomDivVal = m_fZoomBlend + std::fabs(m_fZoom)*m_AmbFrontMic;

    asm volatile ("fence w, w");

    for(unsigned niSample = 0; niSample < nSamples; niSample+=2, src_0+=2, src_1+=2, src_2+=2, src_3+=2, src_4+=2, src_5+=2, src_6+=2, src_7+=2, src_8+=2, src_9+=2, src_10+=2, src_11+=2, src_12+=2, src_13+=2, src_14+=2, src_15+=2)
    {
        audio_t fMic = 0.f;

        SrcData_0.value_64 = read_mem_reqv((void *) src_0);
        SrcData_1.value_64 = read_mem_reqv((void *) src_1);
        SrcData_2.value_64 = read_mem_reqv((void *) src_2);
        SrcData_3.value_64 = read_mem_reqv((void *) src_3);
        SrcData_4.value_64 = read_mem_reqv((void *) src_4);
        SrcData_5.value_64 = read_mem_reqv((void *) src_5);
        SrcData_6.value_64 = read_mem_reqv((void *) src_6);
        SrcData_7.value_64 = read_mem_reqv((void *) src_7);
        SrcData_8.value_64 = read_mem_reqv((void *) src_8);
        SrcData_9.value_64 = read_mem_reqv((void *) src_9);
        SrcData_10.value_64 = read_mem_reqv((void *) src_10);
        SrcData_11.value_64 = read_mem_reqv((void *) src_11);
        SrcData_12.value_64 = read_mem_reqv((void *) src_12);
        SrcData_13.value_64 = read_mem_reqv((void *) src_13);
        SrcData_14.value_64 = read_mem_reqv((void *) src_14);
        SrcData_15.value_64 = read_mem_reqv((void *) src_15);

        // virtual microphone with polar pattern narrowing as Ambisonic order increases
        fMic = m_AmbEncoderFront_weighted[0] * SrcData_0.value_32_1
             + m_AmbEncoderFront_weighted[1] * SrcData_1.value_32_1
             + m_AmbEncoderFront_weighted[2] * SrcData_2.value_32_1
             + m_AmbEncoderFront_weighted[3] * SrcData_3.value_32_1
             + m_AmbEncoderFront_weighted[4] * SrcData_4.value_32_1
             + m_AmbEncoderFront_weighted[5] * SrcData_5.value_32_1
             + m_AmbEncoderFront_weighted[6] * SrcData_6.value_32_1
             + m_AmbEncoderFront_weighted[7] * SrcData_7.value_32_1
             + m_AmbEncoderFront_weighted[8] * SrcData_8.value_32_1
             + m_AmbEncoderFront_weighted[9] * SrcData_9.value_32_1
             + m_AmbEncoderFront_weighted[10] * SrcData_10.value_32_1
             + m_AmbEncoderFront_weighted[11] * SrcData_11.value_32_1
             + m_AmbEncoderFront_weighted[12] * SrcData_12.value_32_1
             + m_AmbEncoderFront_weighted[13] * SrcData_13.value_32_1
             + m_AmbEncoderFront_weighted[14] * SrcData_14.value_32_1
             + m_AmbEncoderFront_weighted[15] * SrcData_15.value_32_1;

        // Blend original channel with the virtual microphone pointed directly to the front
        // Only do this for Ambisonics components that aren't zero for an encoded frontal source
        // Else, reduce the level of the Ambisonic components that are zero for a frontal source
        if(std::abs(m_AmbEncoderFront[0]) > 1e-6) {
            SrcData_0.value_32_1 = (m_fZoomBlend * SrcData_0.value_32_1 + m_AmbEncoderFront[0] * m_fZoom * fMic) / ZoomDivVal;
        } else {
            SrcData_0.value_32_1 = SrcData_0.value_32_1 * m_fZoomRed;
        }
        if(std::abs(m_AmbEncoderFront[1]) > 1e-6) {
            SrcData_1.value_32_1 = (m_fZoomBlend * SrcData_1.value_32_1 + m_AmbEncoderFront[1] * m_fZoom * fMic) / ZoomDivVal;
        } else {
            SrcData_1.value_32_1 = SrcData_1.value_32_1 * m_fZoomRed;
        }
        if(std::abs(m_AmbEncoderFront[2]) > 1e-6) {
            SrcData_2.value_32_1 = (m_fZoomBlend * SrcData_2.value_32_1 + m_AmbEncoderFront[2] * m_fZoom * fMic) / ZoomDivVal;
        } else {
            SrcData_2.value_32_1 = SrcData_2.value_32_1 * m_fZoomRed;
        }
        if(std::abs(m_AmbEncoderFront[3]) > 1e-6) {
            SrcData_3.value_32_1 = (m_fZoomBlend * SrcData_3.value_32_1 + m_AmbEncoderFront[3] * m_fZoom * fMic) / ZoomDivVal;
        } else {
            SrcData_3.value_32_1 = SrcData_3.value_32_1 * m_fZoomRed;
        }
        if(std::abs(m_AmbEncoderFront[4]) > 1e-6) {
            SrcData_4.value_32_1 = (m_fZoomBlend * SrcData_4.value_32_1 + m_AmbEncoderFront[4] * m_fZoom * fMic) / ZoomDivVal;
        } else {
            SrcData_4.value_32_1 = SrcData_4.value_32_1 * m_fZoomRed;
        }
        if(std::abs(m_AmbEncoderFront[5]) > 1e-6) {
            SrcData_5.value_32_1 = (m_fZoomBlend * SrcData_5.value_32_1 + m_AmbEncoderFront[5] * m_fZoom * fMic) / ZoomDivVal;
        } else {
            SrcData_5.value_32_1 = SrcData_5.value_32_1 * m_fZoomRed;
        }
        if(std::abs(m_AmbEncoderFront[6]) > 1e-6) {
            SrcData_6.value_32_1 = (m_fZoomBlend * SrcData_6.value_32_1 + m_AmbEncoderFront[6] * m_fZoom * fMic) / ZoomDivVal;
        } else {
            SrcData_6.value_32_1 = SrcData_6.value_32_1 * m_fZoomRed;
        }
        if(std::abs(m_AmbEncoderFront[7]) > 1e-6) {
            SrcData_7.value_32_1 = (m_fZoomBlend * SrcData_7.value_32_1 + m_AmbEncoderFront[7] * m_fZoom * fMic) / ZoomDivVal;
        } else {
            SrcData_7.value_32_1 = SrcData_7.value_32_1 * m_fZoomRed;
        }
        if(std::abs(m_AmbEncoderFront[8]) > 1e-6) {
            SrcData_8.value_32_1 = (m_fZoomBlend * SrcData_8.value_32_1 + m_AmbEncoderFront[8] * m_fZoom * fMic) / ZoomDivVal;
        } else {
            SrcData_8.value_32_1 = SrcData_8.value_32_1 * m_fZoomRed;
        }
        if(std::abs(m_AmbEncoderFront[9]) > 1e-6) {
            SrcData_9.value_32_1 = (m_fZoomBlend * SrcData_9.value_32_1 + m_AmbEncoderFront[9] * m_fZoom * fMic) / ZoomDivVal;
        } else {
            SrcData_9.value_32_1 = SrcData_9.value_32_1 * m_fZoomRed;
        }
        if(std::abs(m_AmbEncoderFront[10]) > 1e-6) {
            SrcData_10.value_32_1 = (m_fZoomBlend * SrcData_10.value_32_1 + m_AmbEncoderFront[10] * m_fZoom * fMic) / ZoomDivVal;
        } else {
            SrcData_10.value_32_1 = SrcData_10.value_32_1 * m_fZoomRed;
        }
        if(std::abs(m_AmbEncoderFront[11]) > 1e-6) {
            SrcData_11.value_32_1 = (m_fZoomBlend * SrcData_11.value_32_1 + m_AmbEncoderFront[11] * m_fZoom * fMic) / ZoomDivVal;
        } else {
            SrcData_11.value_32_1 = SrcData_11.value_32_1 * m_fZoomRed;
        }
        if(std::abs(m_AmbEncoderFront[12]) > 1e-6) {
            SrcData_12.value_32_1 = (m_fZoomBlend * SrcData_12.value_32_1 + m_AmbEncoderFront[12] * m_fZoom * fMic) / ZoomDivVal;
        } else {
            SrcData_12.value_32_1 = SrcData_12.value_32_1 * m_fZoomRed;
        }
        if(std::abs(m_AmbEncoderFront[13]) > 1e-6) {
            SrcData_13.value_32_1 = (m_fZoomBlend * SrcData_13.value_32_1 + m_AmbEncoderFront[13] * m_fZoom * fMic) / ZoomDivVal;
        } else {
            SrcData_13.value_32_1 = SrcData_13.value_32_1 * m_fZoomRed;
        }
        if(std::abs(m_AmbEncoderFront[14]) > 1e-6) {
            SrcData_14.value_32_1 = (m_fZoomBlend * SrcData_14.value_32_1 + m_AmbEncoderFront[14] * m_fZoom * fMic) / ZoomDivVal;
        } else {
            SrcData_14.value_32_1 = SrcData_14.value_32_1 * m_fZoomRed;
        }
        if(std::abs(m_AmbEncoderFront[15]) > 1e-6) {
            SrcData_15.value_32_1 = (m_fZoomBlend * SrcData_15.value_32_1 + m_AmbEncoderFront[15] * m_fZoom * fMic) / ZoomDivVal;
        } else {
            SrcData_15.value_32_1 = SrcData_15.value_32_1 * m_fZoomRed;
        }

        // virtual microphone with polar pattern narrowing as Ambisonic order increases
        fMic = m_AmbEncoderFront_weighted[0] * SrcData_0.value_32_2
             + m_AmbEncoderFront_weighted[1] * SrcData_1.value_32_2
             + m_AmbEncoderFront_weighted[2] * SrcData_2.value_32_2
             + m_AmbEncoderFront_weighted[3] * SrcData_3.value_32_2
             + m_AmbEncoderFront_weighted[4] * SrcData_4.value_32_2
             + m_AmbEncoderFront_weighted[5] * SrcData_5.value_32_2
             + m_AmbEncoderFront_weighted[6] * SrcData_6.value_32_2
             + m_AmbEncoderFront_weighted[7] * SrcData_7.value_32_2
             + m_AmbEncoderFront_weighted[8] * SrcData_8.value_32_2
             + m_AmbEncoderFront_weighted[9] * SrcData_9.value_32_2
             + m_AmbEncoderFront_weighted[10] * SrcData_10.value_32_2
             + m_AmbEncoderFront_weighted[11] * SrcData_11.value_32_2
             + m_AmbEncoderFront_weighted[12] * SrcData_12.value_32_2
             + m_AmbEncoderFront_weighted[13] * SrcData_13.value_32_2
             + m_AmbEncoderFront_weighted[14] * SrcData_14.value_32_2
             + m_AmbEncoderFront_weighted[15] * SrcData_15.value_32_2;

        // Blend original channel with the virtual microphone pointed directly to the front
        // Only do this for Ambisonics components that aren't zero for an encoded frontal source
        // Else, reduce the level of the Ambisonic components that are zero for a frontal source
        if(std::abs(m_AmbEncoderFront[0]) > 1e-6) {
            SrcData_0.value_32_2 = (m_fZoomBlend * SrcData_0.value_32_2 + m_AmbEncoderFront[0] * m_fZoom * fMic) / ZoomDivVal;
        } else {
            SrcData_0.value_32_2 = SrcData_0.value_32_2 * m_fZoomRed;
        }
        if(std::abs(m_AmbEncoderFront[1]) > 1e-6) {
            SrcData_1.value_32_2 = (m_fZoomBlend * SrcData_1.value_32_2 + m_AmbEncoderFront[1] * m_fZoom * fMic) / ZoomDivVal;
        } else {
            SrcData_1.value_32_2 = SrcData_1.value_32_2 * m_fZoomRed;
        }
        if(std::abs(m_AmbEncoderFront[2]) > 1e-6) {
            SrcData_2.value_32_2 = (m_fZoomBlend * SrcData_2.value_32_2 + m_AmbEncoderFront[2] * m_fZoom * fMic) / ZoomDivVal;
        } else {
            SrcData_2.value_32_2 = SrcData_2.value_32_2 * m_fZoomRed;
        }
        if(std::abs(m_AmbEncoderFront[3]) > 1e-6) {
            SrcData_3.value_32_2 = (m_fZoomBlend * SrcData_3.value_32_2 + m_AmbEncoderFront[3] * m_fZoom * fMic) / ZoomDivVal;
        } else {
            SrcData_3.value_32_2 = SrcData_3.value_32_2 * m_fZoomRed;
        }
        if(std::abs(m_AmbEncoderFront[4]) > 1e-6) {
            SrcData_4.value_32_2 = (m_fZoomBlend * SrcData_4.value_32_2 + m_AmbEncoderFront[4] * m_fZoom * fMic) / ZoomDivVal;
        } else {
            SrcData_4.value_32_2 = SrcData_4.value_32_2 * m_fZoomRed;
        }
        if(std::abs(m_AmbEncoderFront[5]) > 1e-6) {
            SrcData_5.value_32_2 = (m_fZoomBlend * SrcData_5.value_32_2 + m_AmbEncoderFront[5] * m_fZoom * fMic) / ZoomDivVal;
        } else {
            SrcData_5.value_32_2 = SrcData_5.value_32_2 * m_fZoomRed;
        }
        if(std::abs(m_AmbEncoderFront[6]) > 1e-6) {
            SrcData_6.value_32_2 = (m_fZoomBlend * SrcData_6.value_32_2 + m_AmbEncoderFront[6] * m_fZoom * fMic) / ZoomDivVal;
        } else {
            SrcData_6.value_32_2 = SrcData_6.value_32_2 * m_fZoomRed;
        }
        if(std::abs(m_AmbEncoderFront[7]) > 1e-6) {
            SrcData_7.value_32_2 = (m_fZoomBlend * SrcData_7.value_32_2 + m_AmbEncoderFront[7] * m_fZoom * fMic) / ZoomDivVal;
        } else {
            SrcData_7.value_32_2 = SrcData_7.value_32_2 * m_fZoomRed;
        }
        if(std::abs(m_AmbEncoderFront[8]) > 1e-6) {
            SrcData_8.value_32_2 = (m_fZoomBlend * SrcData_8.value_32_2 + m_AmbEncoderFront[8] * m_fZoom * fMic) / ZoomDivVal;
        } else {
            SrcData_8.value_32_2 = SrcData_8.value_32_2 * m_fZoomRed;
        }
        if(std::abs(m_AmbEncoderFront[9]) > 1e-6) {
            SrcData_9.value_32_2 = (m_fZoomBlend * SrcData_9.value_32_2 + m_AmbEncoderFront[9] * m_fZoom * fMic) / ZoomDivVal;
        } else {
            SrcData_9.value_32_2 = SrcData_9.value_32_2 * m_fZoomRed;
        }
        if(std::abs(m_AmbEncoderFront[10]) > 1e-6) {
            SrcData_10.value_32_2 = (m_fZoomBlend * SrcData_10.value_32_2 + m_AmbEncoderFront[10] * m_fZoom * fMic) / ZoomDivVal;
        } else {
            SrcData_10.value_32_2 = SrcData_10.value_32_2 * m_fZoomRed;
        }
        if(std::abs(m_AmbEncoderFront[11]) > 1e-6) {
            SrcData_11.value_32_2 = (m_fZoomBlend * SrcData_11.value_32_2 + m_AmbEncoderFront[11] * m_fZoom * fMic) / ZoomDivVal;
        } else {
            SrcData_11.value_32_2 = SrcData_11.value_32_2 * m_fZoomRed;
        }
        if(std::abs(m_AmbEncoderFront[12]) > 1e-6) {
            SrcData_12.value_32_2 = (m_fZoomBlend * SrcData_12.value_32_2 + m_AmbEncoderFront[12] * m_fZoom * fMic) / ZoomDivVal;
        } else {
            SrcData_12.value_32_2 = SrcData_12.value_32_2 * m_fZoomRed;
        }
        if(std::abs(m_AmbEncoderFront[13]) > 1e-6) {
            SrcData_13.value_32_2 = (m_fZoomBlend * SrcData_13.value_32_2 + m_AmbEncoderFront[13] * m_fZoom * fMic) / ZoomDivVal;
        } else {
            SrcData_13.value_32_2 = SrcData_13.value_32_2 * m_fZoomRed;
        }
        if(std::abs(m_AmbEncoderFront[14]) > 1e-6) {
            SrcData_14.value_32_2 = (m_fZoomBlend * SrcData_14.value_32_2 + m_AmbEncoderFront[14] * m_fZoom * fMic) / ZoomDivVal;
        } else {
            SrcData_14.value_32_2 = SrcData_14.value_32_2 * m_fZoomRed;
        }
        if(std::abs(m_AmbEncoderFront[15]) > 1e-6) {
            SrcData_15.value_32_2 = (m_fZoomBlend * SrcData_15.value_32_2 + m_AmbEncoderFront[15] * m_fZoom * fMic) / ZoomDivVal;
        } else {
            SrcData_15.value_32_2 = SrcData_15.value_32_2 * m_fZoomRed;
        }

		write_mem_wtfwd((void *) src_0, SrcData_0.value_64);
		write_mem_wtfwd((void *) src_1, SrcData_1.value_64);
		write_mem_wtfwd((void *) src_2, SrcData_2.value_64);
		write_mem_wtfwd((void *) src_3, SrcData_3.value_64);
		write_mem_wtfwd((void *) src_4, SrcData_4.value_64);
		write_mem_wtfwd((void *) src_5, SrcData_5.value_64);
		write_mem_wtfwd((void *) src_6, SrcData_6.value_64);
		write_mem_wtfwd((void *) src_7, SrcData_7.value_64);
		write_mem_wtfwd((void *) src_8, SrcData_8.value_64);
		write_mem_wtfwd((void *) src_9, SrcData_9.value_64);
		write_mem_wtfwd((void *) src_10, SrcData_10.value_64);
		write_mem_wtfwd((void *) src_11, SrcData_11.value_64);
		write_mem_wtfwd((void *) src_12, SrcData_12.value_64);
		write_mem_wtfwd((void *) src_13, SrcData_13.value_64);
		write_mem_wtfwd((void *) src_14, SrcData_14.value_64);
		write_mem_wtfwd((void *) src_15, SrcData_15.value_64);
    }
}

#endif // ZOOMER_OPT
