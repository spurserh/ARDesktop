#ifndef STEREO_CAMERA_H
#define STEREO_CAMERA_H

#include "GL/freeglut.h"
#include "Common/Types.h"

bool StereoCamera_Init();
bool StereoCamera_ToTextures(GLuint tex_left, GLuint tex_right, float *image_aspect, Common::Vec2f *tex_extent);

#endif//STEREO_CAMERA_H
