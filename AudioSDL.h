#pragma once

#include "Audio.h"
//#include <SDL.h>

typedef void (*SdlCallbackFunc)(Uint8 *stream, int len);

class AudioSDL : public Audio {
public:
	AudioSDL(void *userData, unsigned int sampleFrq_, unsigned int bufDurInMsec);
	virtual ~AudioSDL();
	virtual void play();
	virtual void pause();
	virtual void stop();
	virtual void sleep(unsigned int msec);
	virtual void lock();
	virtual void unlock();
	virtual void setSampleRate(unsigned int newSampleRate);
	static void setCallback(SdlCallbackFunc);
protected:
	SDL_AudioSpec *audiohwspec;
	static SdlCallbackFunc callback;
	static void audioCallback(void *userData, Uint8 *stream, int len);
	static short *ringBuffer;
	static size_t ringBufferSize;
	static size_t ringBufferIndex;
};
