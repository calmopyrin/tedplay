#include <iostream>
#include <fstream>
#include <string>
#include "Audio.h"
#include "psid.h"
#include "CbmTune.h"
#include "Tedmem.h"
#include "Cpu.h"

#define MAX_BUFFER_SIZE 0x10000

static uint8_t inputBuffer[MAX_BUFFER_SIZE];
static const unsigned int playerStartAddress = 0xfe00;
static unsigned char psidPlayer[] = {
	0xA9, 0x00 // song nr (offset 2)
	, 0x20 ,
	0xea , 0xe2, // song init (offset 4)
	0x78 , 0xA9 , 0x00 , 0x8D ,
	0x0B , 0xFF , 0xA9 , 0x38 ,
	0xCD , 0x1D , 0xFF , 0xD0 ,
	0xFB , 0xCE , 0x19 , 0xFF ,
	0x20 , 0xEa , 0xe2  // song play (offset 23)
	, 0xEE , 0x19 , 0xFF , 0x4C , 0x0B , 0xfe , 0x00 , 0x00
};

static unsigned char rsidPlayer[] = {
	0xA9, 0x00, // song nr (offset 2)
	0x20,
	0xea, 0xe2, // song init (offset 3)
	0x18, 0x90, 0xfe // song init (offset 5)
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

unsigned int parsePsid(uint8_t *buf, PsidHeader &psidHdr_)
{
	char *buffer = (char *) buf;

	memset(psidHdr_.title, 0, sizeof(psidHdr_.title));
	strncpy(psidHdr_.title, buffer + PSID_NAME, 32);
	psidHdr_.title[PSID_NAME + 32] = 0;
	strncpy(psidHdr_.author, buffer + PSID_AUTHOR, 32);
	psidHdr_.author[PSID_AUTHOR + 32] = 0;
	strncpy(psidHdr_.copyright, buffer + PSID_COPYRIGHT, 32);
	psidHdr_.copyright[PSID_COPYRIGHT + 32] = 0;
	return 0;
}

void getPsidProperties(PsidHeader &psidHdr_, std::string &text)
{
	text = "Filename:\t\t";// + psidHdr_.fileName;
}

void printPsidInfo(PsidHeader &psidHdr_)
{
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
	}
	std::cout << "Type:\t\t" << mType.c_str() << std::endl;
	std::cout << "Module:\t\t" << psidHdr_.title << std::endl;
	std::cout << "Author:\t\t" << psidHdr_.author << std::endl;
	std::cout << "Copyright:\t" << psidHdr_.copyright << std::endl;
	std::cout << "Total tunes:\t" << psidHdr_.tracks << std::endl;
	std::cout << "Default tune:\t" << (psidHdr_.defaultTune + 1) << std::endl;
}

unsigned int readFile(char *fName, uint8_t **bufferPtr, size_t *bLen)
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
	if (direction > 0) {
		if (psidHdr.tracks > psidHdr.current) {
			psidPlayer[1] += direction;
			rsidPlayer[1] += direction;
			psidHdr.current += direction;
		} else {
			std::cerr << "No more tracks." << std::endl;
			return false;
		}
	} else {
		if (1 < psidHdr.current) {
			psidPlayer[1] += direction;
			rsidPlayer[1] += direction;
			psidHdr.current += direction;
		} else {
			std::cerr << "No more tracks." << std::endl;
			return false;
		}
	}
	if (psidHdr.type == 1 || (psidHdr.type == 2 && !psidHdr.replayAddress)) {
		ted->writeProtectedPlayerMemory(playerStartAddress, rsidPlayer, sizeof(rsidPlayer));
	} else {
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

TED *machineInit(unsigned int sampleRate)
{
	ted = new TED;
	ted->setSampleRate(sampleRate);
	cpu = new CPU(ted, ted->Ram + 0xff09, ted->Ram + 0x100);
	ted->cpuptr = cpu;
	ted->loadroms();
	machineReset();
	return ted;
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
	while (count--) {
		ted->ted_process(NULL, 0);
	}
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
	if (player) {
		player->pause();
	}
	playState = 0;
}

void tedplayPlay()
{
	if (player) {
		player->play();
		playState = 1;
	}
}

void tedplayStop()
{
	if (player)
		player->pause();
	cpu->setPC(playerStartAddress);
	playState = 0;
}

int tedPlayGetState()
{
	return playState;
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

#define STARTUP (10 * 1775000 / 8)

int tedplayMain(char *fileName, Audio *player_)
{
	size_t bufLength;
	uint8_t *buf;

	if (!player) {
		player = player_;
		if (!player)
			return 1;
	}

	if (!readFile(fileName, &buf, &bufLength)) {
		
		player->lock();
		machineReset();
		player->unlock();
		player->sleep(200); // some SIDs want 0 delay... who knows why
		player->lock();

		psidHdr.fileName = fileName;

		unsigned short addr;
		if (!strncmp((const char *) buf + 1, "SID", 3)) {
			addr = buf[PSID_START + 1] + (buf[PSID_START] << 8);
			unsigned int corr = 0;
			// zero load address means PRG module
			if (!addr) {
				addr = buf[PSID_MAX_HEADER_LENGTH] + (buf[PSID_MAX_HEADER_LENGTH + 1] << 8);
				corr = 2;
			}
			unsigned short initAddr = buf[PSID_INIT + 1] + (buf[PSID_INIT] << 8);
			unsigned short replayAddr = buf[PSID_MAIN + 1] + (buf[PSID_MAIN] << 8);
			psidHdr.initAddress = initAddr;
			psidHdr.replayAddress = replayAddr;
			psidHdr.defaultTune = readPsid16(buf, PSID_DEFSONG);
			psidHdr.current = psidHdr.defaultTune;
			parsePsid(buf, psidHdr);
			ted->injectCodeToRAM(addr, buf + PSID_MAX_HEADER_LENGTH + corr,
				bufLength - PSID_MAX_HEADER_LENGTH - corr);
			psidHdr.tracks = readPsid16(buf, PSID_NUMBER);
			if (buf[0] == 'P') { // PSID
				psidHdr.type = 0;
				psidPlayer[1] = psidHdr.defaultTune - 1;
				psidPlayer[3] = initAddr & 0xff;
				psidPlayer[4] = initAddr >> 8;
				psidPlayer[22] = replayAddr & 0xff;
				psidPlayer[23] = replayAddr >> 8;
				ted->writeProtectedPlayerMemory(playerStartAddress, psidPlayer, 33);
			} else if (buf[0] == 'R') { // RSID
				psidHdr.type = 1;
				rsidPlayer[1] = psidHdr.defaultTune - 1;
				rsidPlayer[3] = initAddr & 0xff;
				rsidPlayer[4] = initAddr >> 8;
				if (replayAddr != 0) {
					rsidPlayer[5] = 0x4C; // JMP
					rsidPlayer[6] = replayAddr & 0xff;
					rsidPlayer[7] = replayAddr >> 8;
				} else {
					rsidPlayer[5] = 0x18; // CLC
					rsidPlayer[6] = 0x90; // BCC *-2
					rsidPlayer[7] = 0xfe;
				}
				ted->writeProtectedPlayerMemory(playerStartAddress, rsidPlayer, 8);
			}
		} else if (!strncmp((const char *) buf, "CBM8M", 5)) {
			CbmTune tune;
			tune.parse(fileName);
			psidHdr.type = 2;
			strcpy(psidHdr.title, tune.getName());
			strcpy(psidHdr.author, tune.getAuthor());
			strcpy(psidHdr.copyright, tune.getReleaseDate());
			unsigned int dataLen = tune.getDumpLength(0);
			unsigned short loadAddr = tune.getLoadAddress(0);
			unsigned short initAddr = tune.getInitAddress(0);
			unsigned short replayAddr = tune.getPlayAddress(0);
			unsigned char *playerData;
			unsigned int playerLength;
			psidHdr.tracks = tune.getNrOfSubtunes() + 1;
			psidHdr.defaultTune = tune.getDefaultSubtune();
			psidHdr.current = psidHdr.defaultTune + 1;
			psidHdr.replayAddress = replayAddr;
			if (replayAddr) {
				psidPlayer[1] = psidHdr.defaultTune;
				psidPlayer[3] = loadAddr & 0xff;
				psidPlayer[4] = loadAddr >> 8;
				psidPlayer[22] = replayAddr & 0xff;
				psidPlayer[23] = replayAddr >> 8;
				playerLength = 33;
				playerData = psidPlayer;
			} else {
				rsidPlayer[1] = psidHdr.defaultTune;
				rsidPlayer[2] = 0x4C; // JMP
				rsidPlayer[3] = loadAddr & 0xff;
				rsidPlayer[4] = loadAddr >> 8;
				playerLength = 8;
				playerData = rsidPlayer;
			}
			ted->injectCodeToRAM(loadAddr, tune.getBinaryData(0), dataLen - 2);
			ted->writeProtectedPlayerMemory(playerStartAddress, playerData, playerLength);
		} else { // PRG
			psidHdr.type = -1;
			psidHdr.tracks = psidHdr.defaultTune = 1;
			psidHdr.current = 1;
			strcpy(psidHdr.title, fileName);
			strcpy(psidHdr.author, "Unknown");
			strcpy(psidHdr.copyright, "Unknown");
			addr = buf[0] + (buf[1] << 8);
			ted->injectCodeToRAM(addr, buf + 2, bufLength - 2);
			ted->writeProtectedPlayerMemory(playerStartAddress, prgPlayer, 8);
		}
#if 1
		cpu->setPC(playerStartAddress);
#else
		char start[64];
		ted->copyToKbBuffer("M\317:\r");
		machineDoSomeFrames(35 * 900000 /8);
		sprintf(start, "G%04X\r", playerStartAddress);
		ted->copyToKbBuffer(start);
#endif
		try {
			player->unlock();
			{
				tedplayPlay();
			}
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
	if (player && playState)
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

void tedPlayChannelEnable(unsigned int channel, unsigned int enable)
{
	ted->enableChannel(channel, enable);
}


