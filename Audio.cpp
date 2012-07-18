#include "Audio.h"
#include "Tedmem.h"

callbackFunc Audio::callback;
short *Audio::ringBuffer;
size_t Audio::ringBufferSize;
size_t Audio::ringBufferIndex;

void Audio::audioCallback(void *userData, unsigned char *stream, int len)
{
	TED *ted = reinterpret_cast<TED *>(userData);

	if (ted) {

		short *playerBuffer = (short*) stream;
		unsigned int sampleRate = ted->getSampleRate();
		unsigned int sampleCnt = len / 2;
		static unsigned int remainder = 0;

		do {
			// since we double buffer, fill only the half if the one we're playing is depleted
			if ((ringBufferIndex + 1) % (ringBufferSize / 2) == 0) {
				unsigned int half = ringBufferIndex + 1 >= ringBufferSize ? 0 : ringBufferSize / 2;
				ted->ted_process(ringBuffer + half, ringBufferSize / 2);
			}
			//
			unsigned int newCount = remainder + sampleRate;
			if ( newCount >= TED_SOUND_CLOCK) {
				remainder = newCount - TED_SOUND_CLOCK;
				double weightPrev = double(remainder)/double(TED_SOUND_CLOCK);
				double weightCurr = 1.00 - weightPrev;
				// interpolate sample
				short sample = short(weightPrev * double(ringBuffer[ringBufferIndex]) + 
						weightCurr * double(ringBuffer[(ringBufferIndex + 1) % ringBufferSize]) );
				*playerBuffer++ = sample;
				sampleCnt--;
			} else {
				remainder = newCount;
			}
			ringBufferIndex = (ringBufferIndex + 1) % ringBufferSize;
		} while (sampleCnt);
	}
}
