#ifndef PROCESSING_H
#define PROCESSING_H

class ComplexSignal {
  public:
  ComplexSignal(unsigned int _length);
  ~ComplexSignal();
  void set_zero();
  unsigned int length;
  double *real, *imag;
};

class ConvolveBuf {
  
  public:
  ConvolveBuf(unsigned int _x_len, unsigned int _h_len);
  ConvolveBuf(unsigned int _x_len, unsigned int _h_len, double* x, double* h);
  ~ConvolveBuf();
  ComplexSignal *fft1, *fft2, *fft3;
  unsigned int x_len, h_len;
  double *x, *h;
  double *output;
  unsigned int N;  

};

class Effect {
  public:
  Effect(unsigned int bufferSize, double* inputBuffer);
  unsigned int bufferSize;
  double *in, *out;
  virtual void processBuffer();
};

class ConvolveEffect : public Effect {
  public:
  ConvolveEffect(unsigned int bufferSize, double* inputBuffer, unsigned int filterSize, double* filter, bool useFFT = false);
  double* overlapBuffer;
  ConvolveBuf* convbuf;
  bool useFFT;
  void processBuffer();
};


void convolve(ConvolveBuf *convbuf);
void convolve_fft(ConvolveBuf *convbuf);

#endif