
#include "GL/freeglut.h"
#include "ScreenCapture.h"
#include "Common/Types.h"

#include <stdio.h>

#include <OVR.h>	//Oculus SDK

using namespace Common;

unsigned desktop_tex_id = 0;

// 15 inches
float world_screen_width_meters = 0.381f;
float world_meters_per_unit = world_screen_width_meters / 2.0f;

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
					Vec2f const&tex_extent) {
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	gluPerspective(60.0, // fov
				   //0.5f * float(m_hmdInfo.HResolution) / float(m_hmdInfo.VResolution),
				   0.5f * window_width / window_height,
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

void display()
{
	float screen_aspect;
	Vec2f tex_extent;
	if(!ScreenCapture_ToTexture(desktop_tex_id, &screen_aspect, &tex_extent))
	{
		fprintf(stderr, "Warning: ScreenCapture_ToTexture() failed\n");
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

	// TODO: Ovrvision

	glViewport(0,0,window_width / 2,window_height);
	DrawDesktopEye(left_eye, dir, up, screen_aspect, tex_extent);
	glViewport(window_width / 2,0,window_width / 2,window_height);
	DrawDesktopEye(right_eye, dir, up, screen_aspect, tex_extent);

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

int main(int argc, char **argv)
{
	if(!InitRift())
		return 1;

	if(!ScreenCapture_Init())
		return 1;

	glutInit(&argc, argv);
	glutInitWindowSize(800, 600);
	glutInitDisplayMode(GLUT_RGB | GLUT_DOUBLE);

	glutCreateWindow("ARDesktop");
	glutDisplayFunc(display);
	glutReshapeFunc(reshape);
	glutIdleFunc(idle);

	glGenTextures(1, &desktop_tex_id);
	glEnable(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D, desktop_tex_id);
	glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR );
	glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
	glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
	glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);

	glutMainLoop();
	return 0;
}
