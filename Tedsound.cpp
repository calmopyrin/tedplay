#include "Audio.h"
#include "Tedmem.h"
#include "Filter.h"
#include <math.h>
#ifdef _DEBUG
#include <cstdio>
#endif

#define PRECISION 0
#define OSCRELOADVAL (0x3FF << PRECISION)

unsigned int		   TED::masterVolume;
static int             Volume;
static int             Snd1Status;
static int             Snd2Status;
static int             SndNoiseStatus;
static int             DAStatus;
static unsigned short  Freq1;
static unsigned short  Freq2;
static int             NoiseCounter;
static int             FlipFlop[2];
static int             dcOutput[2];
static int             oscCount1;
static int             oscCount2;
static int             OscReload[2];
static int             oscStep;
static unsigned char   noise[256]; // 0-8

inline void TED::setFreq(unsigned int channel, int freq)
{
	dcOutput[channel] = (freq == 0x3FE) ? 1 : 0;
	OscReload[channel] = ((freq + 1)&0x3FF) << PRECISION;
}

void TED::oscillatorReset()
{
	FlipFlop[0] = dcOutput[0] = 0;
	FlipFlop[1] = dcOutput[1] = 0;
	oscCount1 = 0;
	oscCount2 = 0;
	NoiseCounter = 0;
	Freq1 = Freq2 = 0;
	DAStatus = Snd1Status = Snd2Status = 0;
}

// call only once!
void TED::oscillatorInit()
{
	oscillatorReset();
	/* initialise im with 0xa8 */
	int im = 0xa8;
    for (unsigned int i = 0; i<256; i++) {
		noise[i] = im & 1;
		im = (im<<1)+(1^((im>>7)&1)^((im>>5)&1)^((im>>4)&1)^((im>>1)&1));
    }
	oscStep = (1 << PRECISION) << 0;

	// set player specific parameters
	waveForm = 0;
	masterVolume = 8;
	setplaybackSpeed(3);
	enableChannel(0, true);
	enableChannel(1, true);
	enableChannel(2, true);
}

void TED::writeSoundReg(unsigned int reg, unsigned char value)
{
#if defined(_DEBUG) && 1
	static FILE *f = std::fopen("freqlog.txt", "a");
	if (f)
		std::fprintf(f, "%04X <- %02X in cycle %u", 0xff0e + reg, value, CycleCounter);
	fprintf(f, "\n");
#endif

	switch (reg) {
		case 0:
			Freq1 = (Freq1 & 0x300) | value;
			setFreq(0, Freq1);
			break;
		case 1:
			Freq2 = (Freq2 & 0x300) | value;
			setFreq(1, Freq2);
			break;
		case 2:
			Freq2 = (Freq2 & 0xFF) | (value << 8);
			setFreq(1, Freq2);
			break;
		case 3:
			if (DAStatus = value & 0x80) {
				FlipFlop[0] = 1;
				FlipFlop[1] = 1;
				oscCount1 = OscReload[0];
				oscCount2 = OscReload[1];
				NoiseCounter = 0xFF;
			}
			Volume = value & 0x0F;
			if (Volume > 8) Volume = 8;
			Volume = (Volume << 10) * masterVolume / 10;
			Snd1Status = value & 0x10;
			Snd2Status = value & 0x20;
			SndNoiseStatus = value & 0x40;
			break;
		case 4:
			Freq1 = (Freq1 & 0xFF) | (value << 8);
			setFreq(0, Freq1);
			break;
	}
}

void TED::storeToBuffer(short *buffer, unsigned int count)
{
	static double			lp_accu = 0;
	static double			hp_accu = 0;

	const double hptc=4000.0/1000000;		// 6000us (est) maybe 7000 ?
	const double hpc=1.0/(hptc * sampleRate * 2.0);	// 2*pi*fc=1/tau..
	
	// TODO: a proper windowed lowpass FIR filter
#if 0
	const double lpc = 1.0 - exp( - double(sampleRate) / 2.0 / double(TED_SOUND_CLOCK));
	double accu = (double) sample;
	// apply low pass filter -> lp_accu = lpc*accu + (1-lpc)*lp_accu
	lp_accu += lpc * (accu - lp_accu);
	accu = lp_accu - hp_accu;
	// update hp filter pole
	hp_accu +=  hpc * accu;
	// fill the buffer
	*buffer = ((short)accu);
#else
	do {
		double accu = (double) filter->lowPass(*buffer);
		accu = accu - hp_accu;
		// update hp filter pole
		hp_accu +=  hpc * accu;
		// fill the buffer
		*buffer++ = (short)accu;
	} while(--count);
#endif
}

void TED::renderSound(unsigned int nrsamples, short *buffer)
{
	unsigned int mod1, mod2, msb;
	switch (waveForm) {
		default:
			mod1 = channelMask[0];
			mod2 = channelMask[1];
			break;
		case 1: // sawtooth
			mod1 = ((OscReload[0] - oscCount1) << 3) & channelMask[0];
			mod2 = ((OscReload[1] - oscCount2) << 3) & channelMask[1];
			break;
		case 2: // triangle
			msb = 1 << 9;
			mod1 = (OscReload[0] - oscCount1) << 3;
			if (mod1 & msb) mod1 = ~mod1;
			mod2 = (OscReload[1] - oscCount2) << 3;
			if (mod2 & msb) mod2 = ~mod2;
			mod1 &= channelMask[0];
			mod2 &= channelMask[1];
			break;
	}

	// Calculate the buffer...
	if (DAStatus) {// digi?
		short sample = 0;//audiohwspec->silence;
		if (Snd1Status) sample = Volume & mod1;
		if (Snd2Status) sample += Volume & mod2;
		for (;nrsamples--;) {
			*buffer++ = sample & channelMask[2];
		}
	} else {
		unsigned int result;
		for (;nrsamples--;) {
			// Channel 1
			if (dcOutput[0]) {
				FlipFlop[0] = 1;
			} else if ((oscCount1 += oscStep) >= OSCRELOADVAL) {
				FlipFlop[0] ^= 1;
				oscCount1 = OscReload[0] + (oscCount1 - OSCRELOADVAL);
			}
			// Channel 2
			if (dcOutput[1]) {
				FlipFlop[1] = 1;
			} else if ((oscCount2 += oscStep) >= OSCRELOADVAL) {
				NoiseCounter = (NoiseCounter + 1) & 0xFF;
				FlipFlop[1] ^= 1;
				oscCount2 = OscReload[1] + (oscCount2 - OSCRELOADVAL);
			}
			result = (FlipFlop[0] && Snd1Status) ? Volume & mod1 : 0;
			if (Snd2Status && FlipFlop[1]) {
				result += Volume & mod2;
			} else if (SndNoiseStatus && noise[NoiseCounter] & channelMask[2]) {
				result += Volume;
			}
			*buffer++ = result;
		}   // for
	}
}

void TED::setMasterVolume(unsigned int shift)
{
	unsigned int vol = Ram[0xFF11] & 0x0f;
	if (vol > 8) vol = 8;
	Volume = (vol << 10) * shift / 10;
	masterVolume = shift;
}

void TED::selectWaveForm(unsigned int wave)
{
	waveForm = wave;
}

void TED::setplaybackSpeed(unsigned int speed)
{
	unsigned int speeds[] = { 16, 8, 4, 3, 2 };
	playbackSpeed = speeds[(speed - 1) % 5];
}

void TED::getTimeSinceLastReset(int hour, int min, int sec)
{
	ClockCycle elapsedCycles = CycleCounter - lastResetCycle;
	int secondsPlayed = int(double(elapsedCycles) / double(TED_SOUND_CLOCK * 4) + 0.5);
}

void TED::setSampleRate(unsigned int value)
{
	if (value != sampleRate) {
		if (filter)
			delete filter;
		filter = new Filter(value / 2, TED_SOUND_CLOCK, 12);
		filter->reCalcWindowTable();
		sampleRate = value;
	}
}