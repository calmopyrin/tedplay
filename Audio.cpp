#include "Audio.h"
#include "Tedmem.h"

callbackFunc Audio::callback;
short *Audio::ringBuffer;
size_t Audio::ringBufferSize;
size_t Audio::ringBufferIndex;
bool Audio::paused;
bool Audio::recording;
FILE *Audio::wavFileHandle;
size_t Audio::wavDataLength;
short Audio::lastSample;

static const unsigned int playedBufSize = 441 * 5;
static short playedBuffer[playedBufSize];
static bool usingPlayedBuffer = false;
static bool usingFileHandle = false;

void Audio::audioCallback(void *userData, unsigned char *stream, int len)
{
	TED *ted = reinterpret_cast<TED *>(userData);

	if (ted) {

		short *playerBuffer = (short*) stream;
		unsigned int sampleRate = ted->getSampleRate();
		unsigned int sampleCnt = len / 2;
		static unsigned int remainder = 0;
		const double avw = 0.90;
		static double avg = 0;

		do {
			// since we double buffer, fill only the half if the one we're playing is depleted
			if ((ringBufferIndex + 1) % (ringBufferSize / 2) == 0) {
				size_t half = ringBufferIndex + 1 >= ringBufferSize ? 0 : ringBufferSize / 2;
				ted->ted_process(ringBuffer + half, (unsigned int) ringBufferSize / 2);
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
				avg = avw * avg + (1 - avw) * sample;
				lastSample = int(avg);
				sampleCnt--;
			} else {
				remainder = newCount;
			}
			ringBufferIndex = (ringBufferIndex + 1) % ringBufferSize;
		} while (sampleCnt);
		// FIXME: it is not a very good idea to do this here
		if (!paused) {
			if (recording)
				dumpWavData(wavFileHandle, stream, len);
			//
			if (!usingPlayedBuffer)
				for (unsigned int i = 0; i < ((len / 2) < playedBufSize ? len / 2 : playedBufSize); i++)
					playedBuffer[i] = *(((short*)stream) + i);
			else {
				void;
			}
		}
#if 0
		lastSample = *(((short*)stream) + len / 2 - 1);
#endif
	}
}

short* Audio::getLastBuffer(size_t& size)
{
	usingPlayedBuffer = true;
	size = playedBufSize / 2;
	return playedBuffer;
}

void Audio::stopUsingLastBuffer()
{
	usingPlayedBuffer = false;
}

bool Audio::createWav(const char *fileName)
{
	WAVHEADER wav = {
		'R','I','F','F',
		0,
		'W','A','V','E','f','m','t',' ',
		16,
		1,
		1,
		0,
		0,
		2,
		16,
		'd','a','t','a',
		0
	};

	wav.nSamplesPerSec = sampleFrq;
	wav.nAvgBytesPerSec = wav.nSamplesPerSec * wav.nChannels * (wav.nBitsPerSample/8);

	if (wavFileHandle = std::fopen(fileName, "wb")) {
		if (std::fwrite(&wav, sizeof(wav), 1, wavFileHandle)) {
			recording = true;
		} else {
			std::fclose(wavFileHandle);
		}
	}
	return recording;
}

bool Audio::dumpWavData(FILE *fp, unsigned char *buffer, unsigned int length)
{
	usingFileHandle = true;
	if (fp && std::fwrite(buffer, 1, length, fp)) {
		usingFileHandle = false;
		wavDataLength += length;
		return true;
	}
	usingFileHandle = false;
	return false;
}

void Audio::closeWav()
{
	while (usingFileHandle);
	if (wavFileHandle) {
		unsigned int foo = (unsigned int)(ftell(wavFileHandle) - 8);
		// RIFF chunk size
		std::fseek(wavFileHandle, 4, SEEK_SET);
		std::fwrite(&foo, sizeof(foo), 1, wavFileHandle);

		// data chunk size
		foo = foo - sizeof(WAVHEADER) + 8;
		std::fseek(wavFileHandle, sizeof(WAVHEADER) - sizeof(unsigned long), SEEK_SET);
		std::fwrite(&foo, sizeof(foo), 1, wavFileHandle);

		std::fclose(wavFileHandle);
		wavFileHandle = 0;
	}
	recording = false;	
}
