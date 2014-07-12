
#include "GL/freeglut.h"
#include "ScreenCapture.h"
#include "StereoCamera.h"
#include "Common/Types.h"

#include <stdio.h>

#include <OVR.h>	//Oculus SDK

using namespace Common;

unsigned desktop_tex_id = 0;

// Left/right
unsigned ovrvision_tex_ids[2] = {0, 0};

// 15 inches
float world_screen_width_meters = 0.381f;
float world_meters_per_unit = world_screen_width_meters / 2.0f;

float ovrvision_intra_ocular_offset_norm = 0.28f;

int window_width = 1, window_height = 1;

// Oculus Rift stuff
OVR::DeviceManager* m_pManager;
OVR::HMDDevice* m_pHMD;
OVR::SensorDevice* m_pSensor;
OVR::SensorFusion* m_sFusion;
OVR::HMDInfo m_hmdInfo;

bool InitRift() {
	OVR::System::Init();
	m_pManager = OVR::DeviceManager::Create();
	m_pHMD = m_pManager->EnumerateDevices<OVR::HMDDevice>().CreateDevice();
	
	if (!m_pHMD)
		return false;

	m_pHMD->GetDeviceInfo(&m_hmdInfo);

	m_pSensor = m_pHMD->GetSensor();
	m_sFusion = new OVR::SensorFusion;
	m_sFusion->AttachToSensor(m_pSensor);

	return true;
}

Vec3f Multiply(Matrix4f const&matrix, Vec3f const&in) {
	Vec4d ret = Vec4d(in.x, in.y, in.z, 0);
	ret = matrix.Multiply(ret);
	return Vec3f(ret.x, ret.y, ret.z);
}

void DrawDesktopEye(Vec3f const&eye,
					Vec3f const&dir,
					Vec3f const&up,
					float screen_aspect,
					float one_eye_aspect,
					Vec2f const&tex_extent) {
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	gluPerspective(60.0, // fov
				   //0.5f * float(m_hmdInfo.HResolution) / float(m_hmdInfo.VResolution),
				   one_eye_aspect,
				   0.1, 100.0);
				   
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();

	Vec3f center = eye + dir;

	gluLookAt(eye.x,eye.y,eye.z, // eye
			  center.x,center.y,center.z, // center
			  up.x,up.y,up.z // up
			  );

	glBindTexture(GL_TEXTURE_2D, desktop_tex_id);
	glTranslatef(0,0,-2);
	glColor4f(1,1,1,1);
	glBegin(GL_QUADS);
	glTexCoord2f(0,tex_extent.y);
	glVertex2f(-screen_aspect,-1);

	glTexCoord2f(tex_extent.x,tex_extent.y);
	glVertex2f(screen_aspect,-1);

	glTexCoord2f(tex_extent.x, 0);
	glVertex2f(screen_aspect,1);

	glTexCoord2f(0,0);
	glVertex2f(-screen_aspect,1);
	glEnd();
}

void DrawCameraEye(float eye_x_coeff, GLuint tex_id, float screen_aspect, float image_aspect, Vec2f const&tex_extent) {
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	gluOrtho2D(-1,1,-1,1);
				   
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	glTranslatef(ovrvision_intra_ocular_offset_norm * eye_x_coeff,0,0);

	float y_scale = screen_aspect / image_aspect;
	glScalef(1,y_scale,1);

	glBindTexture(GL_TEXTURE_2D, tex_id);
	glColor4f(1,1,1,1);
	glBegin(GL_QUADS);
	glTexCoord2f(0,tex_extent.y);
	glVertex2f(-1,-1);

	glTexCoord2f(tex_extent.x,tex_extent.y);
	glVertex2f(1,-1);

	glTexCoord2f(tex_extent.x, 0);
	glVertex2f(1,1);

	glTexCoord2f(0,0);
	glVertex2f(-1,1);
	glEnd();
}

void display()
{
	float screen_aspect;
	Vec2f tex_extent;
	if(!ScreenCapture_ToTexture(desktop_tex_id, &screen_aspect, &tex_extent))
	{
		fprintf(stderr, "Warning: ScreenCapture_ToTexture() failed\n");
		return;
	}

	float camera_image_aspect;
	Vec2f camera_tex_extent;
	if(!StereoCamera_ToTextures(ovrvision_tex_ids[0], ovrvision_tex_ids[1], &camera_image_aspect, &camera_tex_extent))
	{
		fprintf(stderr, "Warning: StereoCamera_ToTextures() failed\n");
		return;
	}

	float yaw;				// yaw from Rift sensor in radians
	float pitch;			// pitch from Rift sensor in radians
	float roll;			// roll from Rift sensor in radians
	m_sFusion->GetOrientation().GetEulerAngles<OVR::Axis_Y, OVR::Axis_X, OVR::Axis_Z>(&yaw, &pitch, &roll);

	Matrix4f pitch_matrix = Matrix4f::MakeRotationMatrix(-pitch, Vec3f(1,0,0));
	Matrix4f yaw_matrix = Matrix4f::MakeRotationMatrix(-yaw, Vec3f(0,1,0));
	Matrix4f roll_matrix = Matrix4f::MakeRotationMatrix(-roll, Vec3f(0,0,1));

	Vec3f dir(0,0,-1);
	dir = Multiply(yaw_matrix, Multiply(pitch_matrix, Multiply(roll_matrix, dir)));
	Vec3f up(0,1,0);
	up = Multiply(yaw_matrix, Multiply(pitch_matrix, Multiply(roll_matrix, up)));

	const float ipd_world_units = m_hmdInfo.InterpupillaryDistance / world_meters_per_unit;

	Vec3f left_eye(-ipd_world_units / 2.0f,0,0);
	Vec3f right_eye(ipd_world_units / 2.0f,0,0);

	left_eye = Multiply(yaw_matrix, Multiply(pitch_matrix, Multiply(roll_matrix, left_eye)));
	right_eye = Multiply(yaw_matrix, Multiply(pitch_matrix, Multiply(roll_matrix, right_eye)));

	glClearColor(1,0,1,1);
	glClear(GL_COLOR_BUFFER_BIT);

	const float one_eye_aspect = 0.5f * window_width / window_height;

	// Draw the desktop with the real world behind
	glViewport(0,0,window_width / 2,window_height);
	DrawCameraEye(1.0f, ovrvision_tex_ids[0], one_eye_aspect, camera_image_aspect, camera_tex_extent);
	DrawDesktopEye(left_eye, dir, up, screen_aspect, one_eye_aspect, tex_extent);

	glViewport(window_width / 2,0,window_width / 2,window_height);
	DrawCameraEye(-1.0f, ovrvision_tex_ids[1], one_eye_aspect, camera_image_aspect, camera_tex_extent);
	DrawDesktopEye(right_eye, dir, up, screen_aspect, one_eye_aspect, tex_extent);

	glutSwapBuffers();
}

void idle()
{
	// TODO: Limit FPS?
	glutPostRedisplay();
}

void reshape(int w, int h)
{
	window_width = w;
	window_height = h;
}

void keyboard(unsigned char key, int x, int y)
{
	if(key == '[')
		ovrvision_intra_ocular_offset_norm -= 0.01f;
	else if(key == ']')
		ovrvision_intra_ocular_offset_norm += 0.01f;

	fprintf(stderr, "ovrvision_intra_ocular_offset_norm %f\n" , ovrvision_intra_ocular_offset_norm);
}

GLuint MakeTexture() 
{
	GLuint ret = 0;
	glEnable(GL_TEXTURE_2D);
	glGenTextures(1, &ret);
	glBindTexture(GL_TEXTURE_2D, ret);
	glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR );
	glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
	glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
	glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
	return ret;
}

int main(int argc, char **argv)
{
	if(!InitRift())
		return 1;

	if(!StereoCamera_Init())
		return 1;

	if(!ScreenCapture_Init())
		return 1;

	glutInit(&argc, argv);
	glutInitWindowSize(800, 600);
	glutInitDisplayMode(GLUT_RGB | GLUT_DOUBLE);

	glutCreateWindow("ARDesktop");
	glutDisplayFunc(display);
	glutReshapeFunc(reshape);
	glutKeyboardFunc(keyboard);
	glutIdleFunc(idle);

	desktop_tex_id = MakeTexture();
	ovrvision_tex_ids[0] = MakeTexture();
	ovrvision_tex_ids[1] = MakeTexture();

	glutMainLoop();
	return 0;
}
