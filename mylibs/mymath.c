/*******************************************************************************************
*
*   mymath - starter example lib for the mylibs unity build
*
*   Just a .c file, no header needed: amalgamate.cmake hoists everything above
*   the first function (includes, defines, types) plus auto-generated prototypes
*   into mylib.h's declaration section; function bodies go behind MYLIB_IMPLEMENTATION
*
********************************************************************************************/

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
