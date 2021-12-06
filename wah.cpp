/******************************************/
/*
  duplex.cpp
  by Gary P. Scavone, 2006-2007.

  This program opens a duplex stream and passes
  input directly through to the output.
*/
/******************************************/

#include "RtAudio.h"
#include <iostream>
#include <cstdlib>
#include <cstring>
#include <cmath>

/*
typedef char MY_TYPE;
#define FORMAT RTAUDIO_SINT8
*/

// typedef signed short MY_TYPE;
// #define FORMAT RTAUDIO_SINT16

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

unsigned int __i = 0;

double a0, a1, a2, b0, b1, b2, s, fn, wn, K, _x1=0, _x2=0, _y1=0, _y2=0;

double buffer[8192];

#define PI 3.14159265359

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
  // Since the number of input and output channels is equal, we can do
  // a simple buffer copy operation here.
  if ( status ) std::cout << "Stream over/underflow detected." << std::endl;

  unsigned int *bytes = (unsigned int *) data;
  // //memcpy( outputBuffer, inputBuffer, *bytes );
  // for (int i = 0; i < nBufferFrames ; i++) {
  //   *((double*)outputBuffer + i) = sin(2*3.141592*440*(i+__i)*(0.001)*i/44100);
  // }
  double *outputBufferD = (double*)outputBuffer, *inputBufferD = (double*)inputBuffer;

//  for (int i = 0; i < nBufferFrames ; i++) {
    //buffer[i] = 1*sin(2*PI*440.0*(i+__i)/44100.0);
  //  buffer[i] = ((double)rand()/(double)RAND_MAX);
 // }

  // s = 1;
  // fn = 2000.0/44100.0;
  // wn = 2*PI*fn;
  // K = 1;


  // a0 = 1 + (1/wn)*(2*s + 1/wn);
  // a1 = - 2 / pow(wn, 2);
  // a2 = (1/wn)*(1/wn - 2*s) ;
  // b0 = K;
  // b1 = 2*K;
  // b2 = K;

  // std::cout << s << fn << wn << K << a0 << a1 << a2 << b0 << b1 << b2 << std::endl ;

  double amplitude = 0;
  for (int i = 0 ; i < nBufferFrames ; i++) {
    amplitude += pow(inputBufferD[i], 2) / nBufferFrames;
  }
  amplitude = sqrt(amplitude);
  

  for (int i = 0; i < nBufferFrames ; i++) {
    fn = 500.0/22050.0 + 300.0/22050.0*sin(2*PI*(i+__i)/44100.0); //* 0.3 / (amplitude+0.01) );
    //fn = 150.0/22050.0 + amplitude/0.1*8000/22050.0;
    if (i==0) std::cout << amplitude << "\t" << fn*22050.0 << std::endl;
    s = 0.01;
    wn = 2*PI*fn;
    K = 0.5;
    a0 = 1 + (1/wn)*(2*s + 1/wn);
    a1 = - 2 / pow(wn, 2);
    a2 = (1/wn)*(1/wn - 2*s) ;
    b0 = K;
    b1 = 2*K;
    b2 = K;

    if (i == 0) {
      outputBufferD[i] = ( -a1*_y1 - a2*_y2 + b0*inputBufferD[i] + b1*_x1 + b2*_x2 ) / a0 ;
    }
    else if (i == 1) {
      outputBufferD[i] = ( -a1*outputBufferD[i-1] - a2*_y1 + b0*inputBufferD[i] + b1*inputBufferD[i-1] + b2*_x1 ) / a0 ;
    }
    else {
      outputBufferD[i] = ( -a1*outputBufferD[i-1] - a2*outputBufferD[i-2] + b0*inputBufferD[i] + b1*inputBufferD[i-1] + b2*inputBufferD[i-2] ) / a0 ;
    }
    //outputBufferD[i] = (outputBufferD[i] > 0) ? 1 : - 1;
  }
  _x1 = inputBufferD[nBufferFrames-1];
  _x2 = inputBufferD[nBufferFrames-2];
  _y1 = outputBufferD[nBufferFrames-1];
  _y2 = outputBufferD[nBufferFrames-2];
  __i += nBufferFrames;
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
  unsigned int bufferFrames = 256;
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

  try {
    adac.openStream( &oParams, &iParams, FORMAT, fs, &bufferFrames, &inout, (void *)&bufferBytes, &options );
  }
  catch ( RtAudioError& e ) {
    std::cout << '\n' << e.getMessage() << '\n' << std::endl;
    exit( 1 );
  }

  bufferBytes = bufferFrames * channels * sizeof( MY_TYPE );

  // Test RtAudio functionality for reporting latency.
  std::cout << "\nStream latency = " << adac.getStreamLatency() << " frames" << std::endl;

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
