#ifndef _UTILS_H
#define _UTILS_H

#include <sys/time.h>

#ifdef __cplusplus
extern "C" {
#endif

static double _gettime()
{
  struct timeval time;

  gettimeofday(&time, NULL);
  return (double)time.tv_sec + ((double)time.tv_usec)/1000000.0;
}

#ifdef __cplusplus
}
#endif

#endif
