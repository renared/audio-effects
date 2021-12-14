#ifndef PROCESSING_H
#define PROCESSING_H

class ComplexSignal {
  public:
  ComplexSignal(unsigned int _length);
  void set_zero();
  unsigned int length;
  double *real, *imag;
};

class ConvolveBuf {
  
  public:
  //ConvolveBuf(unsigned int _x_len, unsigned int _h_len);
  ConvolveBuf(unsigned int filterSize, unsigned int inputSize, double* filter);
  //~ConvolveBuf();
  //void setInput(double* input);
  //double* getOutput();
  ComplexSignal *fft1, *fft2, *fft3;
  unsigned int inputSize, filterSize;
  double *filter, *in, *out;
  unsigned int N;

};

class Effect {
  public:
  Effect(unsigned int bufferSize, double* inputBuffer);
  unsigned int bufferSize;
  double *in;
  double *out;
  virtual void processBuffer();
};

class ConvolveEffect : public Effect {
  public:
  ConvolveEffect(unsigned int bufferSize, double* inputBuffer, unsigned int filterSize, double* filter, bool useFFT = true);
  double* overlapBuffer;
  ConvolveBuf* convbuf;
  bool useFFT;
  void processBuffer();
};


void convolve(ConvolveBuf *convbuf);
void convolve_fft(ConvolveBuf *convbuf);

#endif