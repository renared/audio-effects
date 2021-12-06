#include <cstring>
#include <iostream>
#include "somefunc.h"
#include "processing.h"

ComplexSignal::ComplexSignal(unsigned int _length) {
  length = _length;
  real = new double[length];
  imag = new double[length];
}

ComplexSignal::~ComplexSignal() {
  delete real;
  delete imag;
}

void ComplexSignal::set_zero() {
  memset(real, 0, length*sizeof(double));
  memset(imag, 0, length*sizeof(double));
}

ConvolveBuf::ConvolveBuf(unsigned int _x_len, unsigned int _h_len) {
  x_len = _x_len;
  h_len = _h_len;
  N = get_nextpow2(x_len + h_len - 1);
  std::cout << "##### " << x_len << " " << h_len << " " << N << " #####" << std::endl;
  fft1 = new ComplexSignal(N);
  fft2 = new ComplexSignal(N);
  fft3 = new ComplexSignal(N);
  x = new double[x_len];
  h = new double[h_len];
  output = new double[N];
}

ConvolveBuf::~ConvolveBuf()
{
    delete fft1;
    delete fft2;
    delete fft3;
    delete x;
    delete h;
    delete output;
}

void convolve(ConvolveBuf *convbuf) {
  // convolue dans le tableau output le tableau convbuf->x de taille convbuf->x_len avec le tableau convbuf->h de taille convbuf->h_len
  // output de taille convbuf->x_len + convbuf->h_len - 1
  for (unsigned int i = 0 ; i < convbuf->x_len + convbuf->h_len - 1 ; i++) {
    convbuf->output[i] = 0;
    unsigned int const jmn = (i >= convbuf->x_len - 1)? i - (convbuf->x_len - 1) : 0;
    unsigned int const jmx = (i <  convbuf->h_len - 1)? i            : convbuf->h_len - 1;
    for (unsigned int j = jmn ; j <= jmx ; ++j) {
      if (i-j < convbuf->x_len)
        convbuf->output[i] += (double)(convbuf->x[i-j] * convbuf->h[j]);
    }
  }
}


void convolve_fft(ConvolveBuf *convbuf) {

  unsigned int N = convbuf->N;

  convbuf->fft1->set_zero();
  convbuf->fft2->set_zero();
  convbuf->fft3->set_zero();
  memset(convbuf->output, 0, N*sizeof(double));

  memcpy(convbuf->fft1->real, convbuf->x, convbuf->x_len*sizeof(double));
  // calcul de sa fft
  if (fftr(convbuf->fft1->real, convbuf->fft1->imag, N) != 0) { std::cout << "erreur fft" << std::endl; exit(1); }
  

  memcpy(convbuf->fft2->real, convbuf->h, convbuf->h_len*sizeof(double));
  // calcul de sa fft
  if (fftr(convbuf->fft2->real, convbuf->fft2->imag, N) != 0) { std::cout << "erreur fft" << std::endl; exit(1); }

  // produit dans buf1 (partie réelle) et buf2 (partie imaginaire)
  for (unsigned int i = 0 ; i < N; i++) {
    convbuf->fft3->real[i] = convbuf->fft1->real[i]*convbuf->fft2->real[i] - convbuf->fft1->imag[i]*convbuf->fft2->imag[i];
    convbuf->fft3->imag[i] = convbuf->fft1->real[i]*convbuf->fft2->imag[i] + convbuf->fft1->imag[i]*convbuf->fft2->real[i];
  }

  //ifft
  if (ifftr(convbuf->fft3->real, convbuf->fft3->imag, N) != 0) { std::cout << "erreur ifft" << std::endl; exit(1); }
  memcpy(convbuf->output, convbuf->fft3->real, N*sizeof(double));
  
}

