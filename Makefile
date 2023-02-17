#CXX := g++
CXXFLAGS += -Wall -Os
LIBS += -lm -lSDL2

OBJECTS := Audio.o AudioSDL.o CbmTune.o Cpu.o Filter.o Sid.o SIDSoundLib.o Tedmem.o Tedsound.o main.o tedplay.o

tedplay:	$(OBJECTS)
	$(CXX) $(LDFLAGS) $(OBJECTS) -o $@ $(LIBS)

clean:
	rm -f tedplay $(OBJECTS)
