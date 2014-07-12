
#include "GL/freeglut.h"

#include "ScreenCapture.h"

#include <stdio.h>

unsigned desktop_tex_id = 0;

void display()
{
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	gluOrtho2D(0, 1, 1, 0);
	glMatrixMode(GL_MODELVIEW);

	glClearColor(1,0,1,1);
	glClear(GL_COLOR_BUFFER_BIT);
	glLoadIdentity();

	float x_scale, y_scale;
	if(!ScreenCapture_ToTexture(desktop_tex_id, &x_scale, &y_scale))
	{
		fprintf(stderr, "Warning: ScreenCapture_ToTexture() failed\n");
		return;
	}

	// TODO: NPOT
	glBindTexture(GL_TEXTURE_2D, desktop_tex_id);
	glColor4f(1,1,1,1);
	glBegin(GL_QUADS);
	glTexCoord2f(0,0);
	glVertex2f(0,0);
	glTexCoord2f(x_scale,0);
	glVertex2f(1,0);
	glTexCoord2f(x_scale, y_scale);
	glVertex2f(1,1);
	glTexCoord2f(0,y_scale);
	glVertex2f(0,1);
	glEnd();

	glutSwapBuffers();
}

void idle()
{
	// TODO: Limit FPS?
	glutPostRedisplay();
}

int main(int argc, char **argv)
{
	if(!ScreenCapture_Init())
		return 1;

	glutInit(&argc, argv);
	glutInitWindowSize(800, 600);
	glutInitDisplayMode(GLUT_RGB | GLUT_DOUBLE);

	glutCreateWindow("ARDesktop");
	glutDisplayFunc(display);
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
