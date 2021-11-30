#ifndef SOMEFUNC_H
#define SOMEFUNC_H

#include <sys/time.h>
#include <sys/resource.h>

#define MIN(a,b) (((a)<(b))?(a):(b))
#define MAX(a,b) (((a)>(b))?(a):(b))

int fft(double *x, double *y, const int m);
  int ifft(double *x, double *y, const int m);
int fftr(double *x, double *y, const int m);
int ifftr(double *x, double *y, const int l);
  static int checkm(const int m);
int get_nextpow2(int n);
char *getmem(int leng, unsigned size);
double *dgetmem(int leng);
double get_process_time();

#endif