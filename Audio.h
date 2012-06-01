#pragma once

class Audio {
public:
	Audio(unsigned int sampleFrq_) : bufferLength(4096) { // 2048
	}
	virtual ~Audio() {};
	virtual void play() = 0;
	virtual void pause() = 0;
	virtual void stop() = 0;
	virtual void setSampleRate(unsigned int newSampleRate) {
		sampleFrq = newSampleRate;
	}
	bool isPaused() { return paused; };
protected:
	unsigned int bufferLength;
	bool paused;
private:
	unsigned int sampleFrq;
};
