#include "SIDSoundLib.h"
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif

static HMODULE m_hDll = NULL;
typedef void (*_sidCreate)(unsigned int clock_type, unsigned int model, unsigned int replayFreq);
typedef void (*_sidDestroy)();
typedef void (*_sidSetSampleRate)(unsigned int sampleRate_);
typedef void (*_sidReset)();
typedef void (*_sidPause)();
typedef void (*_sidWriteReg)(unsigned int reg, unsigned char value);
typedef unsigned char (*_sidReadReg)(unsigned int reg);
typedef void (*_sidCalcSamples)(short* buf, long count);
static _sidCreate sidCreate = NULL;
static _sidDestroy sidDestroy = NULL;
static _sidSetSampleRate sidSetSampleRate = NULL;
static _sidReset sidReset = NULL;
static _sidPause sidPause = NULL;
static _sidWriteReg sidWriteReg = NULL;
static _sidReadReg sidReadReg = NULL;
static _sidCalcSamples sidCalcSamples = NULL;
bool SIDSoundLib::detected = false;

bool SIDSoundLib::connect()
{
	if (!detected) {
#ifdef _DEBUG
		m_hDll = LoadLibrary("sidlib.dll");
#else
		m_hDll = LoadLibrary("sidlib.dll");
#endif
		HRESULT r = GetLastError();
		if (m_hDll) {
			sidCreate = (_sidCreate)GetProcAddress(m_hDll, "sidCreate");
			sidDestroy = (_sidDestroy)GetProcAddress(m_hDll, "sidDestroy");
			sidSetSampleRate = (_sidSetSampleRate)GetProcAddress(m_hDll, "sidSetSampleRate");
			sidReset = (_sidReset)GetProcAddress(m_hDll, "sidReset");
			sidPause = (_sidPause)GetProcAddress(m_hDll, "sidPause");
			sidWriteReg = (_sidWriteReg)GetProcAddress(m_hDll, "sidWriteReg");
			sidReadReg = (_sidReadReg)GetProcAddress(m_hDll, "sidReadReg");
			sidCalcSamples = (_sidCalcSamples)GetProcAddress(m_hDll, "sidCalcSamples");
			if (!(sidCreate && sidDestroy && sidSetSampleRate && sidReset && sidPause && sidWriteReg && sidReadReg && sidCalcSamples))
				return false;
		} else
			return false;
	}
	detected = true;
	return detected;
}

SIDSoundLib::SIDSoundLib(unsigned int model) : SIDsound(model, 0)
{
	if (!detected)
		connect();
	if (detected) {
		sidCreate(sidBaseFreq, model, sidBaseFreq);
	}
}

SIDSoundLib::~SIDSoundLib()
{
	sidDestroy();
	FreeLibrary(m_hDll);
	m_hDll = NULL;
	detected = false;
}

void SIDSoundLib::reset()
{
	sidReset();
}

void SIDSoundLib::setSampleRate(unsigned int sampleRate)
{
	sidSetSampleRate(sampleRate);
}

void SIDSoundLib::SetModel(unsigned int model)
{
	sidDestroy();
	sidCreate(sidBaseFreq, model, sidBaseFreq);
	model_ = model;
}

void SIDSoundLib::setFrequency(unsigned int frq)
{
	SIDsound::setFrequency(frq);
}

void SIDSoundLib::calcSamples(short* buf, long count)
{
	sidCalcSamples(buf, count);
}

void SIDSoundLib::write(unsigned int adr, unsigned char byte)
{
	sidWriteReg(adr & 0x1F, byte);
}

unsigned char SIDSoundLib::read(unsigned int adr)
{
	return sidReadReg(adr & 0x1F);
}
