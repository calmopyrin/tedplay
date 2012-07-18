#pragma once

typedef void (*callbackFunc)(unsigned char *stream, int len);

class Audio {
public:
	Audio(unsigned int sampleFrq_) : bufferLength(4096) { // 2048
	}
	virtual ~Audio() {};
	virtual void play() = 0;
	virtual void pause() = 0;
	virtual void stop() = 0;
	virtual void sleep(unsigned int msec) = 0;
	virtual void flush() {
		unsigned int msec = (unsigned int)(1000.f * double(bufferLength)/double(sampleFrq) + 1);
		sleep(msec);
	}
	virtual void lock() = 0;
	virtual void unlock() = 0;
	virtual void setSampleRate(unsigned int newSampleRate) {
		sampleFrq = newSampleRate;
	}
	bool isPaused() { return paused; };
protected:
	unsigned int bufferLength;
	bool paused;
	static callbackFunc callback;
	static void audioCallback(void *userData, unsigned char *stream, int len);
	static short *ringBuffer;
	static size_t ringBufferSize;
	static size_t ringBufferIndex;
private:
	unsigned int sampleFrq;
};
