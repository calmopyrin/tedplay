#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <cstdio>
#include "Audio.h"
#include "psid.h"
#include "CbmTune.h"
#include "Tedmem.h"
#include "Cpu.h"
#include "Sid.h"
#include "tedplay.h"

//#define NEWPLAYER
#define MAX_BUFFER_SIZE 0x10000

static unsigned char inputBuffer[MAX_BUFFER_SIZE];
static const unsigned int playerStartAddress = 0xfe00;
static unsigned char psidPlayer[] = {
#ifdef NEWPLAYER
	0x78,0xa9,0xd3,0x8d,0x13,0xff,0xa9,0x00,0x8d,0x06,0xff,0x8d,0x3f,0xff,0x0e,0x09 ,
	0xff,0xa9,0x08,0x8d,0x0A,0xff,0xa9,0x8b,0x8d,0xfe,0xff,0xa9,0xfe,0x8d,0xff,0xff ,
	0x78,0xa9,0x00,0x85,0xd0,0xaa,0xa0,0x03,0xb9,0xa3,0xfe,0x99,0xa7,0xfe,0x88,0x10 ,
	0xf7,0xa6,0xd0,0xf0,0x10,0x6e,0xaa,0xfe,0x6e,0xa9,0xfe,0x6e,0xa8,0xfe,0x6e,0xa7 ,
	0xfe,0xca,0x4c,0x33,0xfe,0xad,0xa7,0xfe,0x29,0x01,0x0a,0xa8,0xb9,0xab,0xfe,0x8d ,
	0x00,0xff,
	//0x04,0xdc,
	0xb9,0xac,0xfe,0x8d,
	0x01,0xff,
	//0x05,0xdc,
	0xa5,0xd0,0x20,0xca,0xfe,0x58,0xa9,0xdf ,
	0x8d,0x30,0xfd,0x8d,0x08,0xff,0xad,0x08,0xff,0x4a,0xb0,0x06,0xce,0x22,0xfe,0x18 ,
	0x90,0x09,0x4a,0x4a,0x4a,0xb0,0xe7,0xee,0x22,0xfe,0x18,0xa9,0x00,0x8d,0x30,0xfd ,
	0x8d,0x08,0xff,0xae,0x08,0xff,0xe8,0xd0,0xf7,0xf0,0x95,0x48,0x8a,0x48,0x98,0x48 ,
	0xee,0x19,0xff,0x0e,0x09,0xff,0x20,0xb0,0xfe,0xce,0x19,0xff,0x68,0xa8,0x68,0xaa ,
	0x68,0x40,0x60,0x01,0x23,0x45,0x67,0x01,0x23,0x45,0x67, 0x78,0x45,0xe4,0x39,0x00
	// $06b0
	, 0x20, 0xbf, 0xfe, 0x60, 
	0xa0, 0x1f, 0xb9, 0x00, 0xd4, 0x99, 0x40, 0xfd, 0x88, 0x10, 0xf7, 0x60
#else
	0xA9, 0x00 // song nr (offset 1)
	,0x20 ,
	0x33 , 0xfe, // song init (offset 3-4)
	0x78 , 0xA9 , 0x00 , 0x8D ,
	0x06 , 0xFF , 0xA9 , 0x38 ,
	0xCD , 0x1D , 0xFF , 0xD0 ,
	0xFB , 0xCE , 0x19 , 0xFF ,
	0x20 , 0x33 , 0xfe  // song play (offset 22-23)
	, 0xEE , 0x19 , 0xFF , 0x4C , 0x0B , 0xfe, 
	0x00 , 0x60 // fake RTS at $FE33
#endif
};

static unsigned char rsidPlayer[] = {
	0xA9, 0x00, // song nr (offset 1)
	0x20,
	0xea, 0xe2, // song init (offset 3-4)
	0x18, 0x90, 0xfe // song play (offset 6-7)
};

static unsigned char tmfPlayer[] = {
	0xA9, 0x00, // song nr (offset 1)
	0x20,
	0x33 ,0xfe, // song init (offset 3-4)
	0xA9, 0x1B, // screen off? (offset 6)
	0x8D, 0x06, 0xFF,
	0xA9, 0x46, // timing LO (offset 11)
	0x8D, 0x00, 0xFF,
	0xA9, 0x45, // timing HI (offset 16)
	0x8D, 0x01, 0xFF,
	0x78,
	0xA9, 0x08,			// timer 1 IRQ flag
	0x2C, 0x09, 0xFF, // BIT $FF09
	0xF0, 0xFB,
	0x8D, 0x09, 0xFF,
	0x20, 
	0x33, 0xFE,  // song play (offset 31-32)
	0xB8, 0x50, 0xF0
};

static unsigned char prgPlayer[] = {
	0x20, 0xbe, 0x8b,
	0x4c, 0xdc, 0x8b
};

static TED *ted;
static CPU *cpu;
static Audio *player;
static PsidHeader psidHdr;
static int playState = 0;

PsidHeader &getPsidHeader()
{
	return psidHdr;
}

static char convertToAscii(unsigned char c, bool isCaps)
{
	char rv = 0x20;
	
	if (c == 0)
		rv = ' ';
	else if (isCaps) {
		if (c <= 0x1F)
			rv = c ^ 0x40;
		else if (c >= 0x60 && c <= 0x7F)
			rv = c - 0x20;
		else
			rv = c;
	}
	else {
		if (c >= 0x01 && c <= 0x1A)
			rv = c ^ 0x60;
		else if (c >= 0x1B && c <= 0x1F)
			rv = c ^ 0x40;
		else if (c == 0)
			rv = 64;
		else if (c >= 0x41 && c <= 0x5A)
			rv = c ^ 0x20;
		else if (c >= 0xC1 && c <= 0xDA)
			rv = c ^ 0x80;
		else
			rv = c;
	}
	return rv;
}

static void copyPetsciiToAsc(char* to, char* from, unsigned int size)
{
	if (size) {
		unsigned int i = size - 1;
		do {
			to[i] = convertToAscii((unsigned char)from[i], false);
		} while (i--);
	}
}

char *trimTrailingSpace(char* str, unsigned int maxSize)
{
	size_t sln = strlen(str);
	size_t actualLen = maxSize < sln ? maxSize : sln;

	// Trim trailing space
	char *end = str + actualLen - 1;
	while (end > str && isspace(*end)) end--;

	// Write new null terminator character
	end[1] = '\0';

	return str;
}

unsigned int parsePsid(unsigned char *buf, PsidHeader &psidHdr_)
{
	char *buffer = (char *) buf;

	if (buf) {
		memset(psidHdr_.title, 0, sizeof(psidHdr_.title));
		strncpy(psidHdr_.title, trimTrailingSpace(buffer + PSID_NAME, 32), 32);
		psidHdr_.title[PSID_NAME + 32] = 0;
		strncpy(psidHdr_.author, trimTrailingSpace(buffer + PSID_AUTHOR, 32), 32);
		psidHdr_.author[PSID_AUTHOR + 32] = 0;
		strncpy(psidHdr_.copyright, trimTrailingSpace(buffer + PSID_COPYRIGHT, 32), 32);
		psidHdr_.copyright[PSID_COPYRIGHT + 32] = 0;
	} else {
		memset(psidHdr_.title, 0, sizeof(psidHdr_.title));
		memset(psidHdr_.author, 0, 32);
		memset(psidHdr_.copyright, 0, 32);
		memset(psidHdr_.copyright, 0, 32);
	}
	return 0;
}

void getPsidProperties(PsidHeader &psidHdr_, char *os)
{
	char temp[256];

	if (!psidHdr_.tracks) {
		strcat(os, "");
		return;
	}
	std::string mType;
	switch (psidHdr_.type) {
		default:
			mType = "Other";
			break;
		case 0:
			mType = "PSID";
			break;
		case 1:
			mType = "RSID";
			break;
		case 2:
			mType = "CBM8M";
			break;
		case 3:
			mType = "TMF";
			break;
	}
	sprintf(os,   "Type:         %s\r\n", mType.c_str());
	sprintf(temp, "Chip:         %s\r\n", psidHdr_.model);
	strcat(os, temp);
	sprintf(temp, "Module:       %s\r\n", psidHdr_.title);
	strcat(os, temp);
	sprintf(temp, "Author:       %s\r\n", psidHdr_.author);
	strcat(os, temp);
	sprintf(temp, "Released:     %s\r\n", psidHdr_.copyright);
	strcat(os, temp);
	sprintf(temp, "Total tunes:  %u\r\n", psidHdr_.tracks);
	strcat(os, temp);
	sprintf(temp, "Default tune: %u\r\n", psidHdr_.defaultTune);
	strcat(os, temp);
	sprintf(temp, "Init        : $%04X\r\n", psidHdr_.initAddress);
	strcat(os, temp);
	sprintf(temp, "Play address: $%04X\r\n", psidHdr_.replayAddress);
	strcat(os, temp);
}

void printPsidInfo(PsidHeader &psidHdr_)
{
	char output[1024];
	getPsidProperties(psidHdr_, output);
	std::cout << std::string(output) << std::endl;
}

unsigned int readFile(char *fName, unsigned char **bufferPtr, size_t *bLen)
{
	std::FILE *file = (std::FILE *) 0;

	if (!fName)
		return 2;

	try {
		size_t flen;
		file = std::fopen(fName, "rb");
		if (!file)
			return 3;
		std::fseek(file, 0, SEEK_END);
		flen = std::ftell(file);
		if (flen > MAX_BUFFER_SIZE) flen = MAX_BUFFER_SIZE;
		std::fseek(file, 0, SEEK_SET);
		*bufferPtr = inputBuffer;
		size_t r = std::fread(*bufferPtr, flen, 1, file);
		*bLen = flen;
		fclose(file);
	} catch(char *str) {
		std::cerr << "Error opening " << fName << std::endl;
		std::cerr << "    exception: " << str << std::endl;
		return 1;
	}
	return 0;
}

bool psidChangeTrack(int direction)
{
#ifdef NEWPLAYER
	unsigned int tuneOffset = 0x22;
#else
	unsigned int tuneOffset = 1;
#endif
	// SID
	if (direction > 0) {
		if (psidHdr.tracks > psidHdr.current) {
			psidPlayer[tuneOffset] += direction;
			rsidPlayer[tuneOffset] += direction;
			tmfPlayer[tuneOffset] += direction;
			psidHdr.current += direction;
		}
		else {
			std::cerr << "No more tracks." << std::endl;
			return false;
		}
	}
	else {
		if (1 < psidHdr.current) {
			psidPlayer[tuneOffset] += direction;
			rsidPlayer[tuneOffset] += direction;
			psidHdr.current += direction;
			tmfPlayer[tuneOffset] += direction;
		}
		else {
			std::cerr << "No more tracks." << std::endl;
			return false;
		}
	}
	if (psidHdr.type == 1 || (psidHdr.type == 2 && !psidHdr.replayAddress)) {
		ted->writeProtectedPlayerMemory(playerStartAddress, rsidPlayer, sizeof(rsidPlayer));
	}
	else if (psidHdr.type == 3) {
		ted->writeProtectedPlayerMemory(playerStartAddress, tmfPlayer, sizeof(tmfPlayer));
	}
	else {
		ted->writeProtectedPlayerMemory(playerStartAddress, psidPlayer, sizeof(psidPlayer));
	}
	cpu->setPC(playerStartAddress);
	return true;
}

void machineReset()
{
	ted->forcedReset();
	cpu->Reset();
}

TED *machineInit(unsigned int sampleRate, unsigned int filterOrder)
{
	ted = new TED;
	ted->initFilter(sampleRate, filterOrder);
	cpu = new CPU(ted, ted->Ram + 0xff09, ted->Ram + 0x100);
	ted->cpuptr = cpu;
	ted->loadroms();
	machineReset();
	return ted;
}

void tedPlaySetFilterOrder(unsigned int filterOrder)
{
	if (ted) {
		ted->setFilterOrder(filterOrder);
	}
}

void dumpMem(std::string name)
{
	FILE *fp = fopen(name.c_str(), "wb");
	if (!fp)
		return;
	for(unsigned int i = 0; i <= 0xffff; i++) {
		fputc(ted->Read(i), fp);
	}
	fclose(fp);
}

void machineDoSomeFrames(unsigned int count)
{
	if (player)
		player->sleep(count * 20);
}

void machineShutDown()
{
	if(ted) {
		delete ted;
		ted = 0;
	}
	if (cpu) {
		delete cpu;
		cpu = 0;
	}
}

void tedplayPause()
{
	if (player && playState) {
		player->pause();
		player->lock();
	}
	playState = 0;
}

void tedplayPlay()
{
	if (player && !playState) {
		player->unlock();
		ted->oscillatorReset();
		player->play();
		playState = 1;
	}
}

void tedplayStop()
{
	if (player && playState) {
		player->stop();
		player->lock();
	}
	cpu->setPC(playerStartAddress);
	playState = 0;
}

unsigned int tedplayGetSecondsPlayed()
{
	unsigned int sec = ted->getTimeSinceLastReset();
	return sec;
}

short tedPlayGetLastSample()
{
	if (!playState)
		return 0;
	return player->getLastSample();
}

short *tedPlayGetLastBuffer(size_t& size)
{
	if (!playState)
		return 0;
	return player->getLastBuffer(size);
}

void tedPlayStopUsingLastBuffer()
{
	if (player)
		player->stopUsingLastBuffer();
}

void tedPlayResetCycleCounter()
{
	ted->resetCycleCounter();
}

int tedPlayGetState()
{
	return playState;
}

unsigned int tedPlayGetWaveform(unsigned int channel)
{
	if (ted) {
		return ted->getWaveForm(channel);
	}
	return 0;
}

void tedPlaySetWaveform(unsigned int channel, unsigned int wave)
{
	bool wasPlaying = (playState == 1);
	
	if (wasPlaying)
		tedplayPause();
	if (ted) {
		ted->selectWaveForm(channel, wave);
	}
	if (wasPlaying)
		tedplayPlay();
}

void tedplayClose()
{
	if (player) {
		player->stop();
		delete player;
		player = 0;
	}
	machineShutDown();
}

void tedPlayGetInfo(void *file, PsidHeader &hdr)
{
	unsigned char buf[256];
	FILE *fp = reinterpret_cast<FILE *>(file);

	memset(hdr.title, 0, sizeof(hdr.title));
	strcpy(hdr.title, "Unknown");
	memset(hdr.author, 0, sizeof(hdr.author));
	strcpy(hdr.author, "Unknown");
	memset(hdr.copyright, 0, sizeof(hdr.copyright));
	strcpy(hdr.copyright, "Unknown");

	if (fread(buf, 1, 256, fp) >= 64) {
		if (!strncmp((const char *) buf + 1, "SID", 3)) {
			parsePsid(buf, hdr);
			hdr.loadAddress = buf[PSID_START + 1] + (buf[PSID_START] << 8);
			if (!hdr.loadAddress)
				hdr.loadAddress = buf[PSID_MAX_HEADER_LENGTH] + (buf[PSID_MAX_HEADER_LENGTH + 1] << 8);
			if (buf[0] == 'P') {
				hdr.typeName = "PSID";
			} else {
				hdr.typeName = "RSID";
			}
		}
		else if (!strncmp((const char*)buf + 17, "TEDMUSIC", 8)) {
			hdr.typeName = "TMF";
			hdr.loadAddress = buf[28] + (buf[29] << 8);
			copyPetsciiToAsc(hdr.title, trimTrailingSpace((char*)buf + 65, 32), 32);
			copyPetsciiToAsc(hdr.author, trimTrailingSpace((char*)buf + 97, 32), 32);
			copyPetsciiToAsc(hdr.copyright, trimTrailingSpace((char*)buf + 129, 32), 32);
		} else if (!strncmp((const char *) buf, "CBM8M", 5)) {
			CbmTune tune;
			//tune.parse(
			hdr.typeName = "CBM8M";
		} else {
			hdr.typeName = "PRG";
			hdr.loadAddress = buf[0] + (buf[1] << 8);
		}
	}
}

int tedplayMain(char *fileName, Audio *player_)
{
	size_t bufLength;
	unsigned char *buf = 0;

	if (!player) {
		player = player_;
		if (!player)
			return 1;
	}

	if (!readFile(fileName, &buf, &bufLength)) {

		tedplayPause();
		machineReset();
		tedplayPlay();
		// if we do not flush the buffer, the reset may not get executed
		player->flush();
		// let it run so that the reset sequence can finish
		player->sleep(150);
		tedplayStop();

		psidHdr.fileName = fileName;

		if (!strncmp((const char *) buf + 1, "SID", 3) || !strncmp((const char *) buf + 1, "TED", 3)) {
			psidHdr.loadAddress = buf[PSID_START + 1] + (buf[PSID_START] << 8);
			unsigned int corr = 0;
			// zero load address means PRG module
			if (!psidHdr.loadAddress) {
				psidHdr.loadAddress = buf[PSID_MAX_HEADER_LENGTH] + (buf[PSID_MAX_HEADER_LENGTH + 1] << 8);
				corr = 2;
			}
			unsigned short initAddr = buf[PSID_INIT + 1] + (buf[PSID_INIT] << 8);
			unsigned short replayAddr = buf[PSID_MAIN + 1] + (buf[PSID_MAIN] << 8);
			// zero init address means equal to load address
			psidHdr.initAddress = initAddr ? initAddr : psidHdr.loadAddress;
			// 0 means an IRQ handler will be installed in the init routine
			// (most likely nothing happens)
			psidHdr.replayAddress = replayAddr ? replayAddr : 0xfe33;
			psidHdr.defaultTune = readPsid16(buf, PSID_DEFSONG);
			psidHdr.current = psidHdr.defaultTune;
			psidHdr.version = readPsid16(buf, PSID_VERSION);
			psidHdr.tracks = readPsid16(buf, PSID_NUMBER);
			psidHdr.speed = readPsid32(buf, PSID_SPEED);
			parsePsid(buf, psidHdr);

			// setup the SID card
			SIDsound *sid = ted->getSidCard();
			if (sid) {
				// Disable the ROM & IRQ's, so almost the entire memory is RAM
				// more SID's are likely to play
				ted->cpuptr->setST(ted->cpuptr->getST() | 4);
				ted->ChangeMemBankSetup(true);
				// v2 SID files have flags
				if (psidHdr.version == 2 && (readPsid16(buf, PSID_FLAGS) & 0x20)) {
					sid->setModel(SID8580);
					strcpy(psidHdr.model, "SID8580");
				} else {
					sid->setModel(SID6581);
					strcpy(psidHdr.model, "SID6581");
				}
			} else {
				strcpy(psidHdr.model, "TED8360?");
			}
			if (buf[0] == 'P') { // PSID/PTED
				psidHdr.type = 0;
#ifdef NEWPLAYER
				psidPlayer[0x22] = psidHdr.defaultTune - 1;
				psidPlayer[0x5b] = psidHdr.initAddress & 0xff;
				psidPlayer[0x5c] = psidHdr.initAddress >> 8;
				if (psidHdr.replayAddress) {
					psidPlayer[0xb1] = psidHdr.replayAddress & 0xff;
					psidPlayer[0xb2] = psidHdr.replayAddress >> 8;
				}
				psidPlayer[0xA3] = psidHdr.speed & 0xFF;
				psidPlayer[0xA4] = (psidHdr.speed >> 8) & 0xFF;
				psidPlayer[0xA5] = (psidHdr.speed >> 16) & 0xFF;
				psidPlayer[0xA6] = (psidHdr.speed >> 24) & 0xFF;
#else
				psidPlayer[1] = psidHdr.defaultTune - 1;
				psidPlayer[3] = psidHdr.initAddress & 0xff;
				psidPlayer[4] = psidHdr.initAddress >> 8;
				psidPlayer[22] = psidHdr.replayAddress & 0xff;
				psidPlayer[23] = psidHdr.replayAddress >> 8;
				ted->Write(0xFF13, 0xD3); // slow mode to match C64 frequency better
				ted->Write(0xFF0A, 0x00); // no IRQ allowed
				ted->Write(0xFF09, ted->Read(0xFF09)); // ack IRQ
#endif
				ted->writeProtectedPlayerMemory(playerStartAddress, psidPlayer, sizeof(psidPlayer));
			} else if (buf[0] == 'R') { // RSID/RTED
				psidHdr.type = 1;
				rsidPlayer[1] = psidHdr.defaultTune - 1;
				rsidPlayer[3] = psidHdr.initAddress & 0xff;
				rsidPlayer[4] = psidHdr.initAddress >> 8;
				if (replayAddr != 0) {
					rsidPlayer[5] = 0x4C; // JMP
					rsidPlayer[6] = psidHdr.replayAddress & 0xff;
					rsidPlayer[7] = psidHdr.replayAddress >> 8;
				} else {
					rsidPlayer[5] = 0x18; // CLC
					rsidPlayer[6] = 0x90; // BCC *-2
					rsidPlayer[7] = 0xfe;
				}
				ted->writeProtectedPlayerMemory(playerStartAddress, rsidPlayer, sizeof(rsidPlayer));
			}
			ted->injectCodeToRAM(psidHdr.loadAddress, buf + PSID_MAX_HEADER_LENGTH + corr,
				bufLength - PSID_MAX_HEADER_LENGTH - corr);
		}
		else if (!strncmp((const char*)buf + 17, "TEDMUSIC", 8)) {
			// specs http://siz.hu/en/tedmusiccollection
			psidHdr.type = 3;
			psidHdr.version = buf[25];
			psidHdr.tracks = buf[34];
			psidHdr.defaultTune = psidHdr.current = 1;
			psidHdr.replayAddress = playerStartAddress;
			memset(psidHdr.title, 0, sizeof(psidHdr.title));
			memset(psidHdr.author, 0, sizeof(psidHdr.author));
			memset(psidHdr.copyright, 0, sizeof(psidHdr.copyright));
			copyPetsciiToAsc(psidHdr.title, trimTrailingSpace((char*)buf + 65, 32), 32);
			copyPetsciiToAsc(psidHdr.author, trimTrailingSpace((char*)buf + 97, 32), 32);
			copyPetsciiToAsc(psidHdr.copyright, trimTrailingSpace((char*)buf + 129, 32), 32);
			psidHdr.loadAddress = buf[0] + (buf[1] << 8);
			psidHdr.initAddress = buf[30] + (buf[31] << 8);
			psidHdr.replayAddress = buf[32] + (buf[33] << 8);
			// The offset where song data starts from the beginning of the file (load address included)
			unsigned int musicDataOffset = buf[26] + (buf[27] << 8);
			// Memory address where the real music data should be loaded (in case of BASIC load this is the address where the music should be relocated to)
			unsigned int relocatedAddress = buf[28] + (buf[29] << 8);
			// Music data size
			size_t musicDataSize = bufLength - musicDataOffset;
			// Timing
			unsigned char timingLo = buf[36];
			unsigned char timingHi = buf[37];

			tmfPlayer[1] = psidHdr.defaultTune - 1;
			tmfPlayer[3] = psidHdr.initAddress & 0xff;
			tmfPlayer[4] = psidHdr.initAddress >> 8;
			tmfPlayer[6] = (buf[38] & 1) ? 0x00 : 0x1B;
			// timer based timing? FIXME: PAL only
			if (buf[35] >= 2 && buf[35] < 6) {
				tmfPlayer[11] = timingLo;
				tmfPlayer[16] = timingHi;
			}
			// Vblank based - we use timers anyway
			else {
				unsigned int frameRate = (buf[35] == 1 || buf[35] == 7) ? 60 : 50;
				unsigned short timer = (TED_SOUND_CLOCK * 4) / frameRate;
				// doubleSpeed?
				if (buf[35] >= 6)
					timer /= 2;
				tmfPlayer[11] = timer & 0xFF;
				tmfPlayer[16] = timer >> 8;
			}
			tmfPlayer[32] = psidHdr.replayAddress & 0xff;
			tmfPlayer[33] = psidHdr.replayAddress >> 8;

			// if offset is illegal fall back to normal load
			if (int(musicDataSize) - 257 < 0) {
				ted->injectCodeToRAM(psidHdr.loadAddress, buf + 2, bufLength - 2);
			}
			else {
				psidHdr.loadAddress = relocatedAddress;
				ted->injectCodeToRAM(relocatedAddress, buf + musicDataOffset, musicDataSize);
			}
			ted->writeProtectedPlayerMemory(playerStartAddress, tmfPlayer, sizeof(tmfPlayer));

			strcpy(psidHdr.model, "TED8360");
			if (buf[38] & 2) {
				SIDsound* sid = ted->getSidCard();
				if (sid)
					sid->setModel(SID8580);
			}

		} else if (!strncmp((const char *) buf, "CBM8M", 5)) {
			CbmTune tune;
			tune.parse(fileName);
			psidHdr.type = 2;
			strcpy(psidHdr.title, tune.getName());
			strcpy(psidHdr.author, tune.getAuthor());
			strcpy(psidHdr.copyright, tune.getReleaseDate());
			unsigned int dataLen = tune.getDumpLength(0);
			psidHdr.loadAddress = tune.getLoadAddress(0);
			unsigned short initAddr = tune.getInitAddress(0);
			unsigned short replayAddr = tune.getPlayAddress(0);
			unsigned char *playerData;
			unsigned int playerLength;
			psidHdr.version = 1;
			psidHdr.tracks = tune.getNrOfSubtunes() + 1;
			psidHdr.defaultTune = tune.getDefaultSubtune() + 1;
			psidHdr.current = psidHdr.defaultTune;
			psidHdr.initAddress = initAddr;
			psidHdr.replayAddress = replayAddr;
			if (replayAddr) {
				psidPlayer[1] = psidHdr.defaultTune;
				psidPlayer[3] = psidHdr.loadAddress & 0xff;
				psidPlayer[4] = psidHdr.loadAddress >> 8;
				psidPlayer[22] = psidHdr.replayAddress & 0xff;
				psidPlayer[23] = psidHdr.replayAddress >> 8;
				playerLength = 33;
				playerData = psidPlayer;
			} else {
				rsidPlayer[1] = psidHdr.defaultTune;
				rsidPlayer[2] = 0x4C; // JMP
				rsidPlayer[3] = psidHdr.loadAddress & 0xff;
				rsidPlayer[4] = psidHdr.loadAddress >> 8;
				playerLength = 8;
				playerData = rsidPlayer;
			}
			strcpy(psidHdr.model, "TED8360");

			ted->injectCodeToRAM(psidHdr.loadAddress, tune.getBinaryData(0), dataLen - 2);
			ted->writeProtectedPlayerMemory(playerStartAddress, playerData, playerLength);
		} else { // PRG
			psidHdr.type = -1;
			psidHdr.version = 0;
			psidHdr.tracks = psidHdr.defaultTune = 1;
			psidHdr.current = 1;
			psidHdr.replayAddress = playerStartAddress;
			strcpy(psidHdr.title, fileName);
			strcpy(psidHdr.author, "Unknown");
			strcpy(psidHdr.copyright, "Unknown");
			strcpy(psidHdr.model, "Unknown");
			psidHdr.loadAddress = buf[0] + (buf[1] << 8);

			ted->injectCodeToRAM(psidHdr.loadAddress, buf + 2, bufLength - 2);
			ted->writeProtectedPlayerMemory(playerStartAddress, prgPlayer, sizeof(prgPlayer));
			SIDsound *sid = ted->getSidCard();
			if (sid)
				sid->setModel(SID8580);
		}
#if 1
		cpu->setPC(playerStartAddress);
#else
		char start[64];
		ted->copyToKbBuffer("M\317:\r");
		player->sleep(200);
		sprintf(start, "G%04X\r", playerStartAddress);
		ted->copyToKbBuffer(start);
#endif
		try {
			tedplayPlay();
		} catch (char *txt) {
			std::cerr << "Exception: " << txt << std::endl;
			exit(2);
		}
#if 0 //_DEBUG
		//Sleep(3000);
		if (fileName) {
			std::string fn = fileName;
			fn += ".bin";
			dumpMem(fn);
		}
#endif
	}
	return 0;
}

void tedPlaySetVolume(unsigned int masterVolume)
{
	if (player)
		player->pause();
	if (ted)
		ted->setMasterVolume(masterVolume);
	if (player && tedPlayGetState() == 1)
		player->play();
}

void tedPlaySetSpeed(unsigned int speedPct)
{
	if (ted)
		ted->setplaybackSpeed(speedPct);
}

void tedPlayGetSongs(unsigned int &current, unsigned int &total)
{
	current = psidHdr.current;
	total = psidHdr.tracks;
}

bool tedPlayIsChannelEnabled(unsigned int channel)
{
	return ted->isChannelEnabled(channel);
}

void tedPlayChannelEnable(unsigned int channel, bool enable)
{
	ted->enableChannel(channel, enable);
}

void tedPlaySidEnable(bool enable, unsigned int disableMask)
{
	ted->enableSidCard(enable, disableMask);
}

bool tedPlayCreateWav(const char *fileName)
{
	return player->createWav(fileName);
}

void tedPlayCloseWav()
{
	if (player)
		player->closeWav();
}
