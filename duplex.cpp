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

typedef struct {
    unsigned int fs;
    unsigned int bufferFrames;
    unsigned int bufferBytes;
    double* filter;
    unsigned int filterSize;
    MY_TYPE* convResult;
    MY_TYPE* buffer;
    double* _fftbuf1;
    double* _fftbuf2;
    double* _fftbuf3;
    double* _fftbuf4;
    double* _fftbuf5;
    double* _fftbuf6;
  } CallbackData;

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

void convolve(MY_TYPE *output, MY_TYPE* block, double* filter, unsigned int blockSize, unsigned int filterSize) {
  // convolue dans le tableau output le tableau block de taille blockSize avec le tableau filter de taille filterSize
  // output de taille blockSize + filterSize - 1
  for (unsigned int i = 0 ; i < blockSize + filterSize - 1 ; i++) {
    output[i] = 0;
    unsigned int const jmn = (i >= blockSize - 1)? i - (blockSize - 1) : 0;
    unsigned int const jmx = (i <  filterSize - 1)? i            : filterSize - 1;
    for (unsigned int j = jmn ; j <= jmx ; ++j) {
      if (i-j < blockSize)
        output[i] += (MY_TYPE)(block[i-j] * filter[j]);
    }
  }
}

void convolve_fft(double *output, MY_TYPE* block, double* filter, double* buf1, double* buf2, double* buf3, double* buf4, double* buf5, double* buf6, unsigned int blockSize, unsigned int filterSize) {
  unsigned int n = get_nextpow2(blockSize + filterSize - 1);

  memset(buf1, 0, n*sizeof(double));
  memset(buf2, 0, n*sizeof(double));
  memset(buf3, 0, n*sizeof(double));
  memset(buf4, 0, n*sizeof(double));
  memset(output, 0, n*sizeof(double));

  memcpy(buf1, block, blockSize*sizeof(double));
  // calcul de sa fft
  if (fftr(buf1, buf2, n) != 0) { std::cout << "erreur fft" << std::endl; exit(1); }
  

  memcpy(buf3, filter, filterSize);
  // calcul de sa fft
  if (fftr(buf3, buf4, n) != 0) { std::cout << "erreur fft" << std::endl; exit(1); }

  // produit dans buf1 (partie réelle) et buf2 (partie imaginaire)
  for (unsigned int i = 0 ; i < n; i++) {
    buf5[i] = buf1[i]*buf3[i] - buf2[i]*buf4[i];
    buf6[i] = buf1[i]*buf4[i] + buf2[i]*buf3[i];
  }

  //ifft
  if (ifftr(buf5, buf6, n) != 0) { std::cout << "erreur ifft" << std::endl; exit(1); }
  memcpy(output, buf5, n*sizeof(double));
  
}

int inout( void *outputBuffer, void *inputBuffer, unsigned int /*nBufferFrames*/,
           double /*streamTime*/, RtAudioStreamStatus status, void *data )
{

  if ( status ) std::cout << "Stream over/underflow detected." << std::endl;
  double tic = get_process_time();
  double toc;

  MY_TYPE* workBuffer = ((CallbackData*)data)->buffer; // workBuffer contient les résidus des convolutions précédentes, à compter du même début que inputBuffer
  if (USE_FFT) {
    convolve_fft(((CallbackData*)data)->convResult,
                  (MY_TYPE*)inputBuffer, ((CallbackData*)data)->filter, 
                  ((CallbackData*)data)->_fftbuf1, 
                  ((CallbackData*)data)->_fftbuf2, 
                  ((CallbackData*)data)->_fftbuf3, 
                  ((CallbackData*)data)->_fftbuf4,
                  ((CallbackData*)data)->_fftbuf5, 
                  ((CallbackData*)data)->_fftbuf6,
                  ((CallbackData*)data)->bufferFrames, 
                  ((CallbackData*)data)->filterSize
                  );
  }
  else
    convolve(((CallbackData*)data)->convResult, 
              (MY_TYPE*)inputBuffer, 
              ((CallbackData*)data)->filter, 
              ((CallbackData*)data)->bufferFrames, 
              ((CallbackData*)data)->filterSize
              );
  // on a les résidus précédents et la convolution du bloc actuel, plus qu'à sommer dans le workBuffer :
  //std::cout << "\n%%%%%%\n";
  for (unsigned int i = 0 ; i < ((CallbackData*)data)->bufferFrames + ((CallbackData*)data)->filterSize - 1; i++) {
    *(workBuffer+i) = workBuffer[i] + ((CallbackData*)data)->convResult[i];
    //if (i<64) std::cout << workBuffer[i] << " ";
  }
  memcpy(outputBuffer, workBuffer, ((CallbackData*)data)->bufferBytes);
  
  // maintenant on shift le workBuffer en avance pour le prochain bloc
  // memmove(workBuffer, workBuffer + ((CallbackData*)data)->bufferFrames, ((CallbackData*)data)->filterSize -1); // mauvaise idée car alloue un tableau temporaire pour move de façon safe
  for (unsigned int i = 0 ; i < ((CallbackData*)data)->filterSize -1 ; i++) {
    workBuffer[i] = workBuffer[((CallbackData*)data)->bufferFrames + i];
  }
  memset(workBuffer + ((CallbackData*)data)->filterSize - 1, 0, ((CallbackData*)data)->bufferBytes);
  
  toc = get_process_time();
  std::cout << "Time elapsed: " << toc-tic << "\tBlock duration: " << (double)((CallbackData*)data)->bufferFrames / ((CallbackData*)data)->fs << std::endl;
  if (toc - tic > (double)((CallbackData*)data)->bufferFrames / ((CallbackData*)data)->fs) {
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
  unsigned int bufferFrames = 1024*3;
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
  data.bufferFrames = bufferFrames;
  data.bufferBytes = bufferFrames * channels * sizeof( MY_TYPE );

  /*
  FILE* impres = fopen("impres", "rb");
  if (impres==NULL) {fputs ("File error",stderr); exit (1);}
  fseek (impres, 0 , SEEK_END);
  unsigned int lSize = ftell (impres);
  rewind (impres);
  char* buffer = (char*) malloc (sizeof(char)*lSize);
  if (buffer == NULL) {fputs ("Memory error",stderr); exit (2);}
  size_t result = fread (buffer,1,lSize,impres);
  if (result != lSize) {fputs ("Reading error",stderr); exit (3);}
  fclose(impres);

  data.filter = (double*) buffer;
  data.filterSize = lSize * sizeof(char) / sizeof(double);
  std::cout << data.filterSize << std::endl;
  for (int i = 0 ; i < 64 ; i++) std::cout << data.filter[i] << " ";
  */

 std::ifstream is ("impres", std::ifstream::binary);
  if (is) {
    // get length of file:
    is.seekg (0, is.end);
    int length = is.tellg();
    is.seekg (0, is.beg);

    char * buffer = new char [length];

    std::cout << "Reading " << length << " characters or " << length*sizeof(char)/sizeof(double) << " doubles... ";
    // read data as a block:
    is.read (buffer,length);

    if (is)
      std::cout << "all characters read successfully.";
    else
      std::cout << "error: only " << is.gcount() << " could be read";
    is.close();

    // ...buffer contains the entire file...

    data.filter = (double*)buffer;
    data.filterSize = length * sizeof(char) / sizeof(double);
    
    // unsigned int m = get_nextpow2(data.filterSize);
    // if (data.filterSize != m) {
    //   double* zp_filter = dgetmem(m);
    //   memcpy(zp_filter, data.filter, data.filterSize);
    //   data.filterSize = m;
    // }

  }
  else {
    std::cout << "Impossible d'ouvrir le fichier impres." << std::endl;
    exit(1);
  }


  /*
  data.filter = (double*) calloc(1000, sizeof(double));
  for (int i = 0 ; i < 1000 ; i++) {
      data.filter[i] = 1.0/1000.0;
    }
  data.filterSize = 1000;
  */
  
  std::cout << data.filterSize << std::endl;
  for (int i = 0 ; i < 128 ; i++)
    printf ("%lf ", data.filter[i]);

  unsigned int n = get_nextpow2(bufferFrames+data.filterSize-1);
  std::cout << std::endl << n << std::endl;

  data.buffer = (MY_TYPE*) calloc(n, sizeof(MY_TYPE));

  data.convResult = (MY_TYPE*) calloc(n, sizeof(MY_TYPE));

  
  data._fftbuf1 = (MY_TYPE*) calloc(n, sizeof(MY_TYPE));
  data._fftbuf2 = (MY_TYPE*) calloc(n, sizeof(MY_TYPE));
  data._fftbuf3 = (MY_TYPE*) calloc(n, sizeof(MY_TYPE));
  data._fftbuf4 = (MY_TYPE*) calloc(n, sizeof(MY_TYPE));
  data._fftbuf5 = (MY_TYPE*) calloc(n, sizeof(MY_TYPE));
  data._fftbuf6 = (MY_TYPE*) calloc(n, sizeof(MY_TYPE));

  

  if (!data.filter || !data.buffer || !data.convResult) {
    std::cout << "Erreur d'allocation" << std::endl;
    exit(1);
  }

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
  free(data.filter);
  free(data.buffer);
  free(data.convResult);
  free(data._fftbuf1);
  free(data._fftbuf2);
  free(data._fftbuf3);
  free(data._fftbuf4);
  free(data._fftbuf5);
  free(data._fftbuf6);

  return 0;
}
