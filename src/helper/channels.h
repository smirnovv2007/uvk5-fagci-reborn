#ifndef CHANNELS_H
#define CHANNELS_H

#include "../settings.h"
#include <stdint.h>

int32_t CHANNELS_GetCountMax();
void CHANNELS_Load(int32_t num, CH *p);
void CHANNELS_Save(int32_t num, CH *p);
CH *CHANNELS_Get(int32_t i);
bool CHANNELS_LoadBuf();
int32_t CHANNELS_Next(int32_t base, bool next);
void CHANNELS_Delete(int32_t i);
bool CHANNELS_Existing(int32_t i);
uint8_t CHANNELS_Scanlists(int32_t i);
void CHANNELS_LoadScanlist(uint8_t n);

extern int32_t gScanlistSize;
extern int32_t gScanlist[350];

#endif /* end of include guard: CHANNELS_H */
