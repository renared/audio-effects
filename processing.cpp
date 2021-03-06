#include <cstring>
#include <iostream>
#include "somefunc.h"
#include "processing.h"
#include <cmath>

ComplexSignal::ComplexSignal(unsigned int _length) {
  length = _length;
  real = new double[length]();
  imag = new double[length]();
}

void ComplexSignal::set_zero() {
  memset(real, 0, length*sizeof(double));
  memset(imag, 0, length*sizeof(double));
}

ConvolveBuf::ConvolveBuf(unsigned int filterSize, unsigned int inputSize, double* filter) {
  this->inputSize = inputSize;
  this->filterSize = filterSize;
  this->N = get_nextpow2(inputSize + filterSize - 1);
  //std::cout << "##### " << inputSize << " " << filterSize << " " << N << " #####" << std::endl;
  this->fft1 = new ComplexSignal(N);
  this->fft2 = new ComplexSignal(N);
  this->fft3 = new ComplexSignal(N);
  this->out = new double[N]();
  this->filter = filter;
  this->in = NULL;
  this->out = this->fft3->real;
}

// ConvolveBuf::ConvolveBuf(unsigned int _x_len, unsigned int _h_len) 
// : ConvolveBuf(_x_len, _h_len, NULL, NULL) {
//   this->x = new double[inputSize]();
//   this->h = new double[filterSize]();
//   // memset(this->x, 0, inputSize*sizeof(double));
//   // memset(this->h, 0, filterSize*sizeof(double));
// }



// ConvolveBuf::~ConvolveBuf()
// {
//     delete fft1;
//     delete fft2;
//     delete fft3;
//     delete x;
//     delete h;
//     delete output;
// }

Effect::Effect(unsigned int bufferSize, double* inputBuffer) {
  this->bufferSize = bufferSize;
  this->in = inputBuffer;
  this->out = new double[bufferSize]();
}

void Effect::processBuffer() {
  memcpy(out, in, bufferSize*sizeof(double));
}

void Effect::setInput(double* input) {
  this->in = input;
}

void Effect::copyOutput(double* dest) {
  memcpy(dest, this->out, sizeof(double)*this->bufferSize);
}

ConvolveEffect::ConvolveEffect(unsigned int bufferSize, double* inputBuffer, unsigned int filterSize, double* filter, bool useFFT) 
: Effect(bufferSize, inputBuffer){ 
  this->useFFT = useFFT;
  convbuf = new ConvolveBuf(filterSize, bufferSize, filter);
  overlapBuffer = new double[convbuf->N]();
}

void ConvolveEffect::processBuffer() {
  //std::cout << "kek " << Effect::bufferSize << std::endl;
  convbuf->in = this->in;
  //std::cout << this->in << std::endl;
  if (useFFT) convolve_fft(convbuf);
  else convolve(convbuf);
  //for (size_t i = 0 ; i < bufferSize ; i++) std::cout << convbuf->output[i];

  // on a les r??sidus pr??c??dents et la convolution du bloc actuel, plus qu'?? sommer dans le overlapBuffer :
  for (unsigned int i = 0 ; i < convbuf->inputSize + convbuf->filterSize - 1; i++) {
    overlapBuffer[i] = overlapBuffer[i] + convbuf->out[i];
  }
  memcpy(out, overlapBuffer, bufferSize*sizeof(double));

  // maintenant on shift le overlapBuffer en avance pour le prochain bloc
  // memmove(overlapBuffer, overlapBuffer + (nBufferFrames, data_p->convbuf->hSize -1); // mauvaise id??e car alloue un tableau temporaire pour move de fa??on safe
  for (unsigned int i = 0 ; i < convbuf->filterSize -1 ; i++) {
    overlapBuffer[i] = overlapBuffer[bufferSize + i];
  }
  memset(overlapBuffer + convbuf->filterSize - 1, 0, bufferSize*sizeof(double));
}

FxChain::FxChain(unsigned int bufferSize, double* inputBuffer)
: Effect(bufferSize, inputBuffer) {
  this->bufferSize = bufferSize;
  this->in = inputBuffer;
  this->out = NULL;
}

void FxChain::push_back(Effect* fx) {
  this->fxVector.push_back(fx);
  this->fxVector.front()->in = this->in;
  this->connectChain();
}

void FxChain::connectChain() {
  this->fxVector.front()->in = this->in;
  for (int i = 1 ; i < this->fxVector.size() ; i++) {
    this->fxVector[i]->in = this->fxVector[i-1]->out;
  }
  this->out = this->fxVector.back()->out;
}

void FxChain::processBuffer() {
  this->connectChain();
  for (Effect* fx : this->fxVector) {
    fx->processBuffer();
  }
}

WahEffect::WahEffect(unsigned int bufferSize) : Effect(bufferSize) {}

void WahEffect::processBuffer() {
  for (int i = 0; i < bufferSize ; i++) {
    fn = 500.0/22050.0 + 300.0/22050.0*sin(2*M_PI*(i+_i)/44100.0); 

    s = 0.01;
    wn = 2*M_PI*fn;
    K = 0.5;
    a0 = 1 + (1/wn)*(2*s + 1/wn);
    a1 = - 2 / pow(wn, 2);
    a2 = (1/wn)*(1/wn - 2*s) ;
    b0 = K;
    b1 = 2*K;
    b2 = K;

    if (i == 0) {
      out[i] = ( -a1*_y1 - a2*_y2 + b0*in[i] + b1*_x1 + b2*_x2 ) / a0 ;
    }
    else if (i == 1) {
      out[i] = ( -a1*out[i-1] - a2*_y1 + b0*in[i] + b1*in[i-1] + b2*_x1 ) / a0 ;
    }
    else {
      out[i] = ( -a1*out[i-1] - a2*out[i-2] + b0*in[i] + b1*in[i-1] + b2*in[i-2] ) / a0 ;
    }
  }
  _x1 = in[bufferSize-1];
  _x2 = in[bufferSize-2];
  _y1 = out[bufferSize-1];
  _y2 = out[bufferSize-2];
  _i += bufferSize;
}

void convolve(ConvolveBuf *convbuf) {
  // convolue dans le tableau output le tableau convbuf->x de taille convbuf->inputSize avec le tableau convbuf->h de taille convbuf->filterSize
  // output de taille convbuf->inputSize + convbuf->filterSize - 1
  for (unsigned int i = 0 ; i < convbuf->inputSize + convbuf->filterSize - 1 ; i++) {
    convbuf->out[i] = 0;
    unsigned int const jmn = (i >= convbuf->inputSize - 1)? i - (convbuf->inputSize - 1) : 0;
    unsigned int const jmx = (i <  convbuf->filterSize - 1)? i : convbuf->filterSize - 1;
    for (unsigned int j = jmn ; j <= jmx ; ++j) {
      if (i-j < convbuf->inputSize)
        convbuf->out[i] += (double)(convbuf->in[i-j] * convbuf->filter[j]);
    }
  }
}


void convolve_fft(ConvolveBuf *convbuf) {

  unsigned int N = convbuf->N;

  convbuf->fft1->set_zero();
  convbuf->fft2->set_zero();
  convbuf->fft3->set_zero();
  memset(convbuf->out, 0, N*sizeof(double));
  memcpy(convbuf->fft1->real, convbuf->in, convbuf->inputSize*sizeof(double));
  // calcul de sa fft
  if (fftr(convbuf->fft1->real, convbuf->fft1->imag, N) != 0) { std::cout << "erreur fft" << std::endl; exit(1); }
  

  memcpy(convbuf->fft2->real, convbuf->filter, convbuf->filterSize*sizeof(double));
  // calcul de sa fft
  if (fftr(convbuf->fft2->real, convbuf->fft2->imag, N) != 0) { std::cout << "erreur fft" << std::endl; exit(1); }

  // produit dans buf1 (partie r??elle) et buf2 (partie imaginaire)
  for (unsigned int i = 0 ; i < N; i++) {
    convbuf->fft3->real[i] = convbuf->fft1->real[i]*convbuf->fft2->real[i] - convbuf->fft1->imag[i]*convbuf->fft2->imag[i];
    convbuf->fft3->imag[i] = convbuf->fft1->real[i]*convbuf->fft2->imag[i] + convbuf->fft1->imag[i]*convbuf->fft2->real[i];
  }

  //ifft
  if (ifftr(convbuf->fft3->real, convbuf->fft3->imag, N) != 0) { std::cout << "erreur ifft" << std::endl; exit(1); }
  memcpy(convbuf->out, convbuf->fft3->real, N*sizeof(double));
  
  // for (int i = 0 ; i < 10 ; i++) std::cout << convbuf->out[i] ;
}

