/*******************************************************************************************
*
*   mymath - starter example lib for the mylibs unity build
*
*   Drop .h/.c pairs into mylibs/ and they get amalgamated into mylib.h
*
********************************************************************************************/

#ifndef MYMATH_H
#define MYMATH_H

int MyClampInt(int value, int min, int max);
float MyLerpF(float start, float end, float amount);

#endif // MYMATH_H
