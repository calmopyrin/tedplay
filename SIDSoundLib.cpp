#include "SIDSoundLib.h"

#ifdef WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif
#define LIBNAME "sidlib.dll"
#define LOAD_LIBRARY(X) LoadLibrary(X)
#define LOAD_FUNCTION GetProcAddress
#define CLOSE_LIBRARY FreeLibrary
static HMODULE m_hDll = NULL;
#else
#include <dlfcn.h>
#include <stdio.h>
#define LIBNAME "sidlib"
#define LOAD_LIBRARY(X) dlopen(X, RTLD_LOCAL)
#define LOAD_FUNCTION dlsym
#define CLOSE_LIBRARY dlclose
static void *m_hDll = NULL;
#endif

#define LIBNAME_DEBUG LIBNAME

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
		m_hDll = LOAD_LIBRARY(LIBNAME_DEBUG);
#else
		m_hDll = LOAD_LIBRARY(LIBNAME);
#endif
		//HRESULT r = GetLastError();
		if (m_hDll) {
			sidCreate = (_sidCreate)LOAD_FUNCTION(m_hDll, "sidCreate");
			sidDestroy = (_sidDestroy)LOAD_FUNCTION(m_hDll, "sidDestroy");
			sidSetSampleRate = (_sidSetSampleRate)LOAD_FUNCTION(m_hDll, "sidSetSampleRate");
			sidReset = (_sidReset)LOAD_FUNCTION(m_hDll, "sidReset");
			sidPause = (_sidPause)LOAD_FUNCTION(m_hDll, "sidPause");
			sidWriteReg = (_sidWriteReg)LOAD_FUNCTION(m_hDll, "sidWriteReg");
			sidReadReg = (_sidReadReg)LOAD_FUNCTION(m_hDll, "sidReadReg");
			sidCalcSamples = (_sidCalcSamples)LOAD_FUNCTION(m_hDll, "sidCalcSamples");
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
	CLOSE_LIBRARY(m_hDll);
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
