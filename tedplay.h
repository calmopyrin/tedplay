#pragma once

#include <string>
#include "psid.h"

class TED;
class Audio;

extern TED *machineInit(unsigned int sampleRate, unsigned int filterOrder);
extern void tedPlayGetInfo(void *file, PsidHeader &hdr);
extern int tedplayMain(char *fileName, Audio *player);
extern void tedPlaySetVolume(unsigned int masterVolume);
extern void tedPlaySetSpeed(unsigned int speedPct);
extern void tedplayPause();
extern void tedplayPlay();
extern void tedplayStop();
extern void tedplayClose();
extern void tedPlayGetSongs(unsigned int &current, unsigned int &total);
extern bool tedPlayIsChannelEnabled(unsigned int channel);
extern void tedPlayChannelEnable(unsigned int channel, bool enable);
extern int tedPlayGetState();
extern unsigned int tedPlaySidEnable(unsigned int typeEnabled, unsigned int disableMask);
extern void tedPlaySetFilterOrder(unsigned int filterOrder);
extern unsigned int tedPlayGetWaveform(unsigned int channel);
extern void tedPlaySetWaveform(unsigned int channel, unsigned int wave);
extern bool tedPlayCreateWav(const char *fileName);
extern void tedPlayCloseWav();
extern void tedPlaySidModelSelection(unsigned int model);
extern unsigned int tedplayGetSecondsPlayed();
extern void tedPlayResetCycleCounter();
extern short tedPlayGetLastSample();
extern short* tedPlayGetLastBuffer(size_t& size);
extern void tedPlayStopUsingLastBuffer();
//
extern void machineReset();
extern void machineDoSomeFrames(unsigned int count);
extern void dumpMem(std::string name);
extern void tedShowScreenData(char* out);
