#ifndef SCREEN_CATURE_H
#define SCREEN_CATURE_H

#include "GL/freeglut.h"
#include "Common/Types.h"

bool ScreenCapture_Init();
bool ScreenCapture_ToTexture(GLuint tex_id, float *screen_aspect, Common::Vec2f *tex_extent);

#endif//SCREEN_CATURE_H
