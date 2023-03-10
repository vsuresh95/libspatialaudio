#ifndef COH_DEFINES_H
#define COH_DEFINES_H

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
