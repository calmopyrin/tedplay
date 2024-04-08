#pragma once
#include "Sid.h"

class SIDSoundLib :
    public SIDsound
{
public:
	SIDSoundLib(unsigned int model);
	virtual ~SIDSoundLib();
	virtual void reset();
	virtual void setSampleRate(unsigned int sampleRate);
	virtual void setModel(unsigned int model);
	virtual void setFrequency(unsigned int);
	unsigned char read(unsigned int adr);
	void write(unsigned int adr, unsigned char byte);
	static bool connect();
	virtual const char* getSidEngineName();

protected:
	virtual void calcSamples(short* buf, long count);
	static bool detected;
};
