
#include "StereoCamera.h"

#ifdef WIN32
#include <ovrvision.h>		//Ovrvision SDK
#else
#include <OvrvisionSDK/ovrvision.h>		//Ovrvision SDK
#define ShowCursor(x)
#endif

using namespace Common;

//Camera image size
#define CAM_WIDTH			(640)
#define CAM_HEIGHT			(480)

// Ovrvision stuff
OVR::Ovrvision* g_pOvrvision;

Vec2i camera_image_size;
char *camera_image_buffer = 0;

namespace {
int NextPOT(int x) {
	int ret = 1;
	for(;ret<x;ret*=2);
	return ret;
}
}

bool StereoCamera_Init()
{
	//Create ovrvision object
	g_pOvrvision = new OVR::Ovrvision();
	g_pOvrvision->Open(0,OVR::OV_CAMVGA_FULL);	//Open
	if(!g_pOvrvision)
		return false;
	camera_image_size = Vec2i(NextPOT(CAM_WIDTH), NextPOT(CAM_HEIGHT));
	camera_image_buffer = new char[camera_image_size.width * camera_image_size.height * 3];
	return true;
}

bool StereoCamera_ToTextures(GLuint tex_left, GLuint tex_right, float *image_aspect, Common::Vec2f *tex_extent)
{
	g_pOvrvision->PreStoreCamData();

	unsigned char* p_raw[2] = {
		g_pOvrvision->GetCamImage(OVR::OV_CAMEYE_LEFT), 
		g_pOvrvision->GetCamImage(OVR::OV_CAMEYE_RIGHT)};

	GLuint tex[2] = {tex_left, tex_right};

	for(int i=0;i<2;++i) {
		for(int row=0;row<CAM_HEIGHT;++row) {
			memcpy(camera_image_buffer + row * camera_image_size.width * 3,
				   p_raw[i] + row * CAM_WIDTH * 3,
				   CAM_WIDTH * 3);
		}
		glBindTexture(GL_TEXTURE_2D, tex[i]);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, camera_image_size.width, camera_image_size.height, 0, GL_RGB, GL_UNSIGNED_BYTE, (void*)camera_image_buffer);
	}

	*image_aspect = float(CAM_WIDTH) / float(CAM_HEIGHT);
	*tex_extent = Vec2f(float(CAM_WIDTH) / float(camera_image_size.width), float(CAM_HEIGHT) / float(camera_image_size.height));
	return true;
}
