#include <SDL.h>
#include <iostream>
#include "AudioSDL.h"
#include "Tedmem.h"

#define TED_SOUND_CLOCK 110860
#pragma comment(lib, "SDL.lib")

SdlCallbackFunc AudioSDL::callback;
short *AudioSDL::resampleBuffer;
size_t AudioSDL::resampleBufferSize;
size_t AudioSDL::resampleBufferIndex;

void AudioSDL::audioCallback(void *userData, Uint8 *stream, int len)
{
	TED *ted = reinterpret_cast<TED *>(userData);

	if (ted) {
#if 1
		short *playerBuffer = (short*) stream;
		unsigned int sampleRate = ted->getSampleRate();
		unsigned int sampleCnt = len / 2;
		static unsigned int remainder = 0;
		unsigned int sampleIndex = 0;
		do {
			// since we double buffer, fill only the half if the one we're playing is depleted
			if ((resampleBufferIndex + 1) % (resampleBufferSize / 2) == 0) {
				unsigned int half = resampleBufferIndex + 1 >= resampleBufferSize ? 0 : resampleBufferSize / 2;
				//ted->ted_process(resampleBuffer, resampleBufferSize / 2);
				ted->ted_process(resampleBuffer + half, resampleBufferSize / 2);
			}
			//
			unsigned int newCount = remainder + sampleRate;
			if ( newCount >= TED_SOUND_CLOCK) {
				remainder = newCount - TED_SOUND_CLOCK;
#if 1
				int weightPrev = remainder;
				int weightCurr = (TED_SOUND_CLOCK - remainder);
				short sample = (weightPrev * int(resampleBuffer[resampleBufferIndex]) + 
						weightCurr * int(resampleBuffer[(resampleBufferIndex + 1) % resampleBufferSize]) ) / TED_SOUND_CLOCK;
				*playerBuffer++ = sample;
#else
				*playerBuffer++ = resampleBuffer[sampleIndex];
#endif
				sampleCnt--;
			} else {
				remainder = newCount;
			}
			sampleIndex += 1;
			resampleBufferIndex = (resampleBufferIndex + 1) % resampleBufferSize;
		} while (sampleCnt);
#else
		ted->ted_process((short*) stream, len / 2);
#endif
	}
}

void AudioSDL::setCallback(SdlCallbackFunc callback_)
{
	SDL_LockAudio();
	callback = callback_;
	SDL_UnlockAudio();
}

void AudioSDL::setSampleRate(unsigned int newSampleRate)
{
	if (audiohwspec) {
		SDL_PauseAudio(1);
		unsigned int fadeoutTime = 1000 * bufferLength / audiohwspec->samples;
		SDL_Delay(fadeoutTime);
		SDL_CloseAudio();

		SDL_AudioSpec *obtained = new SDL_AudioSpec;
		audiohwspec->samples = newSampleRate;
		try {
			SDL_OpenAudio(audiohwspec, obtained);
			if (obtained) {
				SDL_AudioSpec *temp = audiohwspec;
				audiohwspec = obtained;
				delete temp;
			}
			SDL_PauseAudio(0);
		} catch (char *txt) {
			std::cerr << "Exception occurred: " << txt << std::endl;
		}
	} else {
	}
	Audio::setSampleRate(newSampleRate);
}

AudioSDL::AudioSDL(void *userData, unsigned int sampleFrq_ = 48000) : Audio(sampleFrq_), audiohwspec(0)
{
	if (SDL_Init(SDL_INIT_AUDIO) < 0) { //  SDL_INIT_AUDIO|
		std::cerr << "Unable to init SDL: " << SDL_GetError() << std::endl;
		exit(1);
	}
	atexit(SDL_Quit);

	SDL_AudioSpec *desired, *obtained = NULL;

	try {
		desired = new SDL_AudioSpec;
		obtained = new SDL_AudioSpec;
	} catch(char *txt) {
		std::cerr << "Exception occurred: " << txt << std::endl;
	}

	//sampleFrq = sampleFrq_;
	unsigned int bufDurInMsec = 40;
	unsigned int fragsPerSec = 1000 / bufDurInMsec;
	unsigned int bufSize1kbChunk = (sampleFrq_ / fragsPerSec / 1024) * 1024;
	if (!bufSize1kbChunk) bufSize1kbChunk = 512;
	bufferLength = bufSize1kbChunk;

	desired->freq		= sampleFrq_;
	desired->format		= AUDIO_S16;
	desired->channels	= 1;
	desired->samples	= bufferLength;
	desired->callback	= audioCallback;
	desired->userdata	= userData;
	desired->size		= desired->channels * desired->samples * sizeof(Uint8);
	desired->silence	= 0x00;

	unsigned int bfSize = TED_SOUND_CLOCK / fragsPerSec; //(bufferLength * TED_SOUND_CLOCK + sampleFrq_ / 2) / sampleFrq_;
	// double length ring buffer, dividable by 8
	resampleBufferSize = (bfSize / 8 + 1) * 16;
	resampleBuffer = new short[resampleBufferSize];
	// trigger initial buffer fill
	resampleBufferIndex = resampleBufferSize-1;

	if (SDL_OpenAudio(desired, obtained)) {
		fprintf(stderr,"SDL_OpenAudio failed!\n");
		return;
	} else {
		char drvnamebuf[16];
		fprintf(stderr,"SDL_OpenAudio success!\n");
		SDL_AudioDriverName( drvnamebuf, 16);
		fprintf(stderr, "Using audio driver : %s\n", drvnamebuf);		
		if ( obtained == NULL ) {
			fprintf(stderr, "Great! We have our desired audio format!\n");
			audiohwspec = desired;
			delete obtained;
		} else {
			//fprintf(stderr, "Oops! Failed to get desired audio format!\n");
			audiohwspec = obtained;
			delete desired;
		}
	}
	paused = true;
}

void AudioSDL::play()
{
	SDL_PauseAudio(0);
	paused = false;
}

void AudioSDL::pause()
{
	SDL_PauseAudio(1);
	paused = true;
}

void AudioSDL::stop()
{
	SDL_PauseAudio(1);
	paused = true;
	///
}

AudioSDL::~AudioSDL()
{
    SDL_PauseAudio(1);
    SDL_Delay(20);
	SDL_CloseAudio();
	SDL_Quit();
	if (audiohwspec)
		delete audiohwspec;
	if (resampleBuffer)
		delete [] resampleBuffer;
}