#ifndef _SID_H
#define _SID_H

#define SOUND_FREQ_PAL_C64 985248

enum { 
	SID6581 = 0,
	SID8580,
	SID8580DB,
	SID6581R1
};

// SID class
class SIDsound 
{
public:
	SIDsound(unsigned int model, unsigned int chnlDisableMask);
	virtual ~SIDsound();
	virtual void reset();
	virtual void setReplayFreq() {
		calcEnvelopeTable();
	};
	virtual void setModel(unsigned int model);
	virtual void setFrequency(unsigned int sid_frequency);
	virtual void setSampleRate(unsigned int sampleRate_);
	void calcEnvelopeTable();
	virtual unsigned char read(unsigned int adr);
	virtual void write(unsigned int adr, unsigned char byte);
	virtual void calcSamples(short* buf, long count);
	void enableDisableChannel(unsigned int ch, bool enabled) {
		voice[ch].disabled = !enabled;
	}
	virtual const char* getSidEngineName() {
		return "yape";
	}

private:

	// SIDsound waveforms
	enum {
		WAVE_NONE, WAVE_TRI, WAVE_SAW, WAVE_TRISAW, WAVE_PULSE, 
		WAVE_TRIPULSE, WAVE_SAWPULSE, WAVE_TRISAWPULSE,	WAVE_NOISE
	};
	// Envelope Genarator states
	enum {
		EG_FROZEN, EG_ATTACK, EG_DECAY, EG_RELEASE
	};
	// Filter types
	enum { 
		FILTER_NONE, FILTER_LP, FILTER_BP, FILTER_LPBP, FILTER_HP, FILTER_NOTCH, FILTER_HPBP, FILTER_ALL
	};
	// Class for SID voices
	class SIDVoice {
	public:
		unsigned int index;		// index of voice
		int wave;				// the selected waveform
		int egState;			// state of the EG
		SIDVoice *modulatedBy;	// the voice that modulates this one
		SIDVoice *modulatesThis;// the voice that is modulated by this one

		unsigned int accu;		// accumulator of the waveform generator, 8.16 fixed
		unsigned int accPrev;	// previous accu value (for ring modulation)
		unsigned int shiftReg;	// shift register for noise waveform
		unsigned int waveNoiseOut; // stored value of noise output

		unsigned int freq;	// voice frequency
		unsigned int pw;		// pulse-width value

		unsigned int envAttackAdd;
		unsigned int envDecaySub;
		unsigned int envSustainLevel;
		unsigned int envReleaseSub;
		unsigned int envCurrLevel;
		unsigned int envCounter;
		unsigned int envExpCounter;
		unsigned int envCounterCompare;

		unsigned int gate;		// EG gate flag
		unsigned int ring;		// ring modulation flag
		unsigned int test;		// test flag
		unsigned int filter;	// voice filtered flag
		unsigned int muted;		// voice muted flag (only for 3rd voice)
		bool		disabled;	// voice disabled

		// This bit is set for the modulating voice, 
		// not for the modulated one (compared to the real one)
		unsigned int sync; // sync modulation flag
	} voice[3];			// array for the 3 channels
	int volume;			// SID Master volume
	unsigned int sidCyclesPerSampleInt;
	unsigned int clockDeltaRemainder; // Accumulator for frequency conversion
	unsigned int clockDeltaFraction; // Fractional component for frequency conversion
	int dcMixer; // different for 6581 and 8580 (constant level output for digi)
	int dcVoice;
	int dcWave;
	int dcDigiBlaster;
	int extIn;
	//
	unsigned int clock();
	// Wave generator functions
	inline static int waveTriangle(SIDVoice &v);
	inline static int waveSaw(SIDVoice &v);
	inline static int wavePulse(SIDVoice &v);
	inline static int waveTriSaw(SIDVoice &v);
	inline static int waveTriPulse(SIDVoice &v);
	inline static int waveSawPulse(SIDVoice &v);
	inline static int waveTriSawPulse(SIDVoice &v);
	inline static int waveNoise(SIDVoice &v);
	inline static int getWaveSample(SIDVoice &v);
	inline void updateShiftReg(SIDVoice &v);
	// Envelope
	inline int doEnvelopeGenerator(unsigned int cycles, SIDVoice &v);
	static const unsigned int RateCountPeriod[16]; // Factors for A/D/S/R Timing
	static const unsigned char envGenDRdivisors[256]; // For exponential approximation of D/R
	static unsigned int masterVolume;
	// filter stuff
	unsigned char	filterType; // filter type
	unsigned int	filterCutoff;	// SID filter frequency
	unsigned char	filterResonance;	// filter resonance (0..15)
	double cutOffFreq[2048];	// filter cutoff frequency register
	int resonanceCoeffDiv1024;		// filter resonance * 1024
	int w0;					// filter cutoff freq
	void setResonance();
	void setFilterCutoff();
	int filterOutput(unsigned int cycles, int Vi);
	int Vhp; // highpass
	int Vbp; // bandpass
	int Vlp; // lowpass
	//
	unsigned char lastByteWritten;// Last value written to the SID
	bool enableDigiBlaster;
	unsigned int sampleRate;

protected:
	int model_;
	unsigned int sidBaseFreq;	// SID base frequency
};

/*
	Wave outputs
*/
inline int SIDsound::waveTriangle(SIDVoice &v)
{
	unsigned int msb = (v.ring ? v.accu ^ v.modulatedBy->accu : v.accu)
		& 0x800000;
	// triangle wave 15 bit only
 	return ((msb ? ~v.accu : v.accu) >> 11) & 0xFFF;
}

inline int SIDsound::waveSaw(SIDVoice &v)
{
	return v.accu >> 12;
}

inline int SIDsound::wavePulse(SIDVoice &v)
{
	// square wave starts high
	return (v.test | (v.accu >= v.pw ? 0xFFF : 0x000));
}

inline int SIDsound::waveTriSaw(SIDVoice &v)
{
	unsigned int sm = (waveTriangle(v)) & (waveSaw(v));
	return (sm >> 1) & (sm << 1);
}

inline int SIDsound::waveTriPulse(SIDVoice &v)
{
	unsigned int sm = (waveTriangle(v)) & (wavePulse(v));
	return (sm >> 1) & (sm << 1);
}

inline int SIDsound::waveSawPulse(SIDVoice &v)
{
	unsigned int sm = (waveSaw(v)) & (wavePulse(v));
	return (sm >> 1) & (sm << 1);
}

inline int SIDsound::waveTriSawPulse(SIDVoice &v)
{
	unsigned int sm = (waveTriangle(v)) & (waveSaw(v)) & (wavePulse(v));
	return (sm >> 1) & (sm << 1);
}

#endif
