#ifndef SCREEN_CATURE_H
#define SCREEN_CATURE_H

#include "GL/freeglut.h"

bool ScreenCapture_Init();
bool ScreenCapture_ToTexture(GLuint tex_id, float *x_scale, float *y_scale);

#endif//SCREEN_CATURE_H
