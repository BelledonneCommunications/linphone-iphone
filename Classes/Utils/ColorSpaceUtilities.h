/*
 ColorConverter
 
 you can use it to convert color from RGB space to HSL space and back.
 
 HSL2RGB copied from GLPaint Apple sample: http://developer.apple.com/library/ios/#samplecode/GLPaint/Introduction/Intro.html
 
 RGB2HSL translated from http://www.geekymonkey.com/Programming/CSharp/RGB2HSL_HSL2RGB.htm
 
 From: https://github.com/alessani/ColorConverter
*/

void HSL2RGB(float h, float s, float l, float* outR, float* outG, float* outB);
void RGB2HSL(float r, float g, float b, float* outH, float* outS, float* outL);