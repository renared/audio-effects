CC=g++
RTAUDIO_DIR = ./rtaudio-5.2.0
CFLAGS= -c -Wall -g -I$(RTAUDIO_DIR)
LIBS = -lpthread -ljack -lpulse-simple -lasound  -lpulse
LDOBJ = $(RTAUDIO_DIR)/librtaudio_la-RtAudio.o

all: duplex

duplex: duplex.o somefunc.o
	$(CC) $(LIBS) -o $@ $^ $(LDOBJ)

%.o: %.cpp
	echo gwa
	$(CC) $(CFLAGS) $<

clean:
	rm -f *.o
