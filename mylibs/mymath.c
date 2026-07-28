#include "mymath.h"

int MyClampInt(int value, int min, int max)
{
    if (value < min) return min;
    if (value > max) return max;
    return value;
}

float MyLerpF(float start, float end, float amount)
{
    return start + amount*(end - start);
}
