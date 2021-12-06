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
  ~ConvolveBuf();
  ComplexSignal *fft1, *fft2, *fft3;
  unsigned int x_len, h_len;
  double *x, *h;
  double *output;
  unsigned int N;  

};

void convolve(ConvolveBuf *convbuf);
void convolve_fft(ConvolveBuf *convbuf);

#endif