/******************************************/
/*
  duplex.cpp
  by Gary P. Scavone, 2006-2007.

  This program opens a duplex stream and passes
  input directly through to the output.
*/
/******************************************/

#include "rtaudio-5.2.0/RtAudio.h"
#include <iostream>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include "somefunc.h"
#include "processing.h"
#include <vector>

#define USE_FFT 1

/*
typedef char MY_TYPE;
#define FORMAT RTAUDIO_SINT8
*/

/*
typedef signed short MY_TYPE;
#define FORMAT RTAUDIO_SINT16
*/

/*
typedef S24 MY_TYPE;
#define FORMAT RTAUDIO_SINT24

typedef signed long MY_TYPE;
#define FORMAT RTAUDIO_SINT32

typedef float MY_TYPE;
#define FORMAT RTAUDIO_FLOAT32
*/

typedef double MY_TYPE;
#define FORMAT RTAUDIO_FLOAT64


class CallbackData {
  public:
  unsigned int fs;
  FxChain* fxChain;
};

void usage( void ) {
  // Error function in case of incorrect command-line
  // argument specifications
  std::cout << "\nuseage: duplex N fs <iDevice> <oDevice> <iChannelOffset> <oChannelOffset>\n";
  std::cout << "    where N = number of channels,\n";
  std::cout << "    fs = the sample rate,\n";
  std::cout << "    iDevice = optional input device to use (default = 0),\n";
  std::cout << "    oDevice = optional output device to use (default = 0),\n";
  std::cout << "    iChannelOffset = an optional input channel offset (default = 0),\n";
  std::cout << "    and oChannelOffset = optional output channel offset (default = 0).\n\n";
  exit( 0 );
}


int inout( void *outputBuffer, void *inputBuffer, unsigned int nBufferFrames,
           double /*streamTime*/, RtAudioStreamStatus status, void *data )
{

  if ( status ) std::cout << "Stream over/underflow detected." << std::endl;
  double tic = get_process_time();
  double toc;

  CallbackData* data_p = (CallbackData*) data;

  size_t bytes = nBufferFrames * sizeof(double);

  //for (int i = 0 ; i < nBufferFrames ; i++) std::cout << ((double*)inputBuffer)[i] << std::endl;
  
  data_p->fxChain->setInput((double*)inputBuffer);
  data_p->fxChain->processBuffer();
  data_p->fxChain->copyOutput((double*)outputBuffer);
  
  toc = get_process_time();
  std::cout << "Time elapsed: " << toc-tic << "\tBlock duration: " << (double)nBufferFrames / data_p->fs << std::endl;
  if (toc - tic > (double)nBufferFrames / data_p->fs) {
    std::cout << "Callback underrun detected!" << std::endl;
  }

  return 0;
}

int main( int argc, char *argv[] )
{
  unsigned int channels, fs, bufferBytes, oDevice = 0, iDevice = 0, iOffset = 0, oOffset = 0;

  // Minimal command-line checking
  if (argc < 3 || argc > 7 ) usage();

  RtAudio adac;
  if ( adac.getDeviceCount() < 1 ) {
    std::cout << "\nNo audio devices found!\n";
    exit( 1 );
  }

  channels = (unsigned int) atoi(argv[1]);
  if (channels > 1) {
    std::cout << "Erreur seul le traitement mono (channels=1) est implémenté." << std::endl;
    exit(EXIT_FAILURE);
  }
  fs = (unsigned int) atoi(argv[2]);
  if ( argc > 3 )
    iDevice = (unsigned int) atoi(argv[3]);
  if ( argc > 4 )
    oDevice = (unsigned int) atoi(argv[4]);
  if ( argc > 5 )
    iOffset = (unsigned int) atoi(argv[5]);
  if ( argc > 6 )
    oOffset = (unsigned int) atoi(argv[6]);

  // Let RtAudio print messages to stderr.
  adac.showWarnings( true );

  // Set the same number of channels for both input and output.
  unsigned int bufferFrames = 4096;
  RtAudio::StreamParameters iParams, oParams;
  iParams.deviceId = iDevice;
  iParams.nChannels = channels;
  iParams.firstChannel = iOffset;
  oParams.deviceId = oDevice;
  oParams.nChannels = channels;
  oParams.firstChannel = oOffset;

  if ( iDevice == 0 )
    iParams.deviceId = adac.getDefaultInputDevice();
  if ( oDevice == 0 )
    oParams.deviceId = adac.getDefaultOutputDevice();

  RtAudio::StreamOptions options;
  //options.flags |= RTAUDIO_NONINTERLEAVED;

  CallbackData data;

  try {
    adac.openStream( &oParams, &iParams, FORMAT, fs, &bufferFrames, &inout, (void *)&data, &options );
  }
  catch ( RtAudioError& e ) {
    std::cout << '\n' << e.getMessage() << '\n' << std::endl;
    exit( 1 );
  }

  // Test RtAudio functionality for reporting latency.
  std::cout << "\nStream latency = " << adac.getStreamLatency() << " frames" << std::endl;


  // ALLOCATIONS ET INITIALISATION
  // -----------------------------
  data.fs = fs;

  char* filter_c;
  double* filter;
  unsigned int filterSize;
  std::ifstream is ("impres", std::ifstream::binary);
    if (is) {
      // get length of file:
      is.seekg (0, is.end);
      int length = is.tellg();
      is.seekg (0, is.beg);

      filter_c = new char [length];

      std::cout << "Reading " << length << " characters or " << length*sizeof(char)/sizeof(double) << " doubles... ";
      // read data as a block:
      is.read (filter_c,length);

      if (is)
        std::cout << "all characters read successfully.";
      else
        std::cout << "error: only " << is.gcount() << " could be read";
      is.close();

      // ...buffer contains the entire file...

      filter = (double*)filter_c;
      filterSize = length * sizeof(char) / sizeof(double);

    }
    else {
      std::cout << "Impossible d'ouvrir le fichier impres." << std::endl;
      exit(1);
    }
    
  std::cout << filterSize << std::endl;
  for (int i = 0 ; i < 128 ; i++)
    printf ("%lf ", filter[i]);

  FxChain fxChain(bufferFrames);
  WahEffect wah(bufferFrames);
  ConvolveEffect reverb(bufferFrames, NULL, filterSize, filter);
  fxChain.push_back(&wah);
  fxChain.push_back(&reverb);
  data.fxChain = &fxChain;

  try {
    adac.startStream();

    char input;
    std::cout << "\nRunning ... press <enter> to quit (buffer frames = " << bufferFrames << ").\n";
    std::cin.get(input);

    // Stop the stream.
    adac.stopStream();
  }
  catch ( RtAudioError& e ) {
    std::cout << '\n' << e.getMessage() << '\n' << std::endl;
    goto cleanup;
  }

 cleanup:
  if ( adac.isStreamOpen() ) adac.closeStream();

  return 0;
}
