#include "Audio.h"
#include "Tedmem.h"
#include "Filter.h"
#include <math.h>
#ifdef _DEBUG
#include <cstdio>
#endif

#define PRECISION 0
#define OSCRELOADVAL (0x3FF << PRECISION)

static unsigned int		masterVolume;
static int             Volume;
static int             Snd1Status;
static int             Snd2Status;
static int             SndNoiseStatus;
static int             DAStatus;
static unsigned short  Freq1;
static unsigned short  Freq2;
static int             NoiseCounter;
static int             FlipFlop[2];
static int             oscCount1;
static int             oscCount2;
static int             OscReload[2];
static int             oscStep;
static unsigned char   noise[256]; // 0-8

inline void TED::setFreq(unsigned int channel, int freq)
{
	if (freq == 0x3FE) {
		FlipFlop[channel] = 1;
	}
	OscReload[channel] = ((freq + 1)&0x3FF) << PRECISION;
}

void TED::oscillatorReset()
{
	FlipFlop[0] = 0;
	FlipFlop[1] = 0;
	oscCount1 = 0;
	oscCount2 = 0;
	NoiseCounter = 0;
	Freq1 = Freq2 = 0;
	DAStatus = 0;
}

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
}

void TED::writeSoundReg(ClockCycle cycle, unsigned int reg, unsigned char value)
{
#if defined(_DEBUG) && 1
	static FILE *f = std::fopen("freqlog.txt", "a");
	if (f)
		std::fprintf(f, "%04X <- %02X in cycle %u", 0xff0e + reg, value, cycle);
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
				NoiseCounter = 0x100;
			}
			Volume = value & 0x0F;
			if (Volume > 8) Volume = 8;
			Volume = (Volume << 10) * masterVolume / 10;
			setMasterVolume(masterVolume);
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

inline void TED::storeToBuffer(short *buffer, short sample)
{
	static double			lp_accu = 0;
	static double			hp_accu = 0;

	// TODO: a proper windowed lowpass FIR filter
#if 1
	const double lpc = 1.0 - exp( - double(sampleRate) / 2.0 / double(TED_SOUND_CLOCK));
	const double hptc=4000.0/1000000;		// 6000us (est) maybe 7000 ?
	const double hpc=1.0/(hptc * sampleRate * 2.0);	// 2*pi*fc=1/tau..
	double accu = (double) sample;
	// apply low pass filter -> lp_accu = lpc*accu + (1-lpc)*lp_accu
	lp_accu += lpc * (accu - lp_accu);
	accu = lp_accu - hp_accu;
	// update hp filter pole
	hp_accu +=  hpc * accu;
	// fill the buffer
	*buffer = ((short)accu);
#else
	*buffer = sample;
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
			storeToBuffer(buffer++, sample);
			//*buffer++ = sample;
		}
	} else {
		unsigned int result;
		for (;nrsamples--;) {
			// Channel 1
			if ((oscCount1 += oscStep) >= OSCRELOADVAL) {
				if (OscReload[0] != (0x3FE << PRECISION)) 
					FlipFlop[0] ^= 1;
				oscCount1 = OscReload[0] + (oscCount1 - OSCRELOADVAL);
			}
			// Channel 2
			if ((oscCount2 += oscStep) >= OSCRELOADVAL) {
				if (OscReload[1] != (0x3FE << PRECISION)) {
					FlipFlop[1] ^= 1;
					if (NoiseCounter++==256) 
						NoiseCounter=0;
				}
				oscCount2 = OscReload[1] + (oscCount2 - OSCRELOADVAL);
			}
			result = (FlipFlop[0] && Snd1Status) ? Volume & mod1 : 0;
			if (Snd2Status && FlipFlop[1]) {
				result += Volume & mod2;
			} else if (SndNoiseStatus && noise[NoiseCounter]) {
				result += Volume;
			}
			storeToBuffer(buffer++, result);
			//*buffer++ = result;
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
