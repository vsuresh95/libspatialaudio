#ifndef COH_DEFINES_H
#define COH_DEFINES_H

// Use float for CPU data and fixed point for accelerators
typedef float audio_t;
typedef int device_t;

// Helper unions for Spandex register and coalescing.
typedef union
{
  struct
  {
    unsigned int r_en   : 1;
    unsigned int r_op   : 1;
    unsigned int r_type : 2;
    unsigned int r_cid  : 4;
    unsigned int w_en   : 1;
    unsigned int w_op   : 1;
    unsigned int w_type : 2;
    unsigned int w_cid  : 4;
	unsigned int reserved: 16;
  };
  unsigned int spandex_reg;
} spandex_config_t;

typedef union
{
  struct
  {
    device_t value_32_1;
    device_t value_32_2;
  };
  int64_t value_64;
} device_token_t;

typedef union
{
  struct
  {
    audio_t value_32_1;
    audio_t value_32_2;
  };
  int64_t value_64;
} audio_token_t;

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

// Coherence mode string for print header.
#if (IS_ESP == 1)
#if (COH_MODE == ESP_NON_COHERENT_DMA)
const char CohPrintHeader[] = "ESP Non-Coherent DMA";
#elif (COH_MODE == ESP_LLC_COHERENT_DMA)
const char CohPrintHeader[] = "ESP LLC-Coherent DMA";
#elif (COH_MODE == ESP_COHERENT_DMA)
const char CohPrintHeader[] = "ESP Coherent DMA";
#else
const char CohPrintHeader[] = "ESP Baseline MESI";
#endif
#else
#if (COH_MODE == SPX_OWNER_PREDICTION)
const char CohPrintHeader[] = "SPX Owner Prediction";
#elif (COH_MODE == SPX_WRITE_THROUGH_FWD)
const char CohPrintHeader[] = "SPX Write-through forwarding";
#elif (COH_MODE == SPX_BASELINE_REQV)
const char CohPrintHeader[] = "SPX Baseline Spandex (ReqV)";
#else
const char CohPrintHeader[] = "SPX Baseline Spandex";
#endif
#endif

#endif // COH_DEFINES_H