#ifndef TOOLS_H
#define TOOLS_H

#include "stdint.h"
#include "math.h"

#define FS_HZ       100
#define W           20     
#define K_SIGMA     4.0f
#define N_CONSEC    3
#define CALIB_SAMPLES 200 //100hz * 2

uint8_t status_verify(void);

#endif
