#include "tools.h"


static float window[W];
static int idx = 0, filled = 0;


static void push_sample(float ax, float ay, float az){
    float mag_sq = ax*ax + ay*ay + az*az;
    window[idx++] = mag_sq;
    if (idx >= W) idx = 0;
    if (filled < W) filled++;
}

static float compute_rms_sq(void)
{
    float sum = 0.0f;
    for (int i=0;i<filled;i++) sum += window[i];
    return (filled==0) ? 0.0f : (sum / filled);
}

