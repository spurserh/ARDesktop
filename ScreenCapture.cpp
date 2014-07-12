
#include "ScreenCapture.h"

// Windows implementation

#include <windows.h>
#include <stdio.h>
#include <assert.h>

using namespace Common;

HDC hScreenDC = 0, hMemoryDC = 0;
HBITMAP hBitmap = 0;

int screen_width;
int screen_height;

int texture_width, texture_height;

BITMAPINFOHEADER bmi;

char *screen_data = 0;
char *texture_data = 0;

namespace {
int NextPOT(int x) {
	int ret = 1;
	for(;ret<x;ret*=2);
	return ret;
}
}

bool ScreenCapture_Init()
{
	// get the device context of the screen
	hScreenDC = GetDC(NULL);//CreateDC("DISPLAY", NULL, NULL, NULL);     
	// and a device context to put it in
	hMemoryDC = CreateCompatibleDC(hScreenDC);

	screen_width = GetSystemMetrics(SM_CXSCREEN);
	screen_height = GetSystemMetrics(SM_CYSCREEN);

	assert(screen_width == GetDeviceCaps(hScreenDC, HORZRES));
	assert(screen_height == GetDeviceCaps(hScreenDC, VERTRES));

	texture_width = NextPOT(screen_width);
	texture_height = NextPOT(screen_height);

	screen_data = new char[screen_width * screen_height * 4];
	texture_data = new char[texture_width * texture_height * 4];

	// maybe worth checking these are positive values
	hBitmap = CreateCompatibleBitmap(hScreenDC, screen_width, screen_height);

	BitBlt(hMemoryDC, 0, 0, screen_width, screen_height, hScreenDC, 0, 0, SRCCOPY);

	memset(&bmi, 0, sizeof(bmi));
    bmi.biSize = sizeof(BITMAPINFOHEADER);
    bmi.biPlanes = 1;
    bmi.biBitCount = 32;
    bmi.biWidth = screen_width;
    bmi.biHeight = -screen_height;
    bmi.biCompression = BI_RGB;
    bmi.biSizeImage = 0;// 3 * ScreenX * ScreenY;

	return true;
}

bool ScreenCapture_ToTexture(GLuint desktop_tex_id, float *screen_aspect, Vec2f *tex_extent)
{
	// get a new bitmap
	HBITMAP hOldBitmap = (HBITMAP)SelectObject(hMemoryDC, hBitmap);

	BitBlt(hMemoryDC, 0, 0, screen_width, screen_height, hScreenDC, 0, 0, SRCCOPY);

	// Cursor
	{
		//Draw the cursor
		CURSORINFO m_GlobalCursor;
		m_GlobalCursor.cbSize = sizeof(CURSORINFO);
		int m_intCursorWidth = GetSystemMetrics(SM_CXCURSOR);
		int m_intCursorHeight = GetSystemMetrics(SM_CYCURSOR);
		GetCursorInfo(&m_GlobalCursor);
		if (m_GlobalCursor.flags == CURSOR_SHOWING)	{
			ICONINFO CursorInfo;
			GetIconInfo((HICON)m_GlobalCursor.hCursor, &CursorInfo);
			DrawIconEx(hMemoryDC,
					   int((m_GlobalCursor.ptScreenPos.x - CursorInfo.xHotspot)), int((m_GlobalCursor.ptScreenPos.y - CursorInfo.yHotspot)),
					   m_GlobalCursor.hCursor,
					   int(m_intCursorWidth), int(m_intCursorHeight),
					   0, NULL, DI_COMPAT | DI_NORMAL);		
			DeleteObject(CursorInfo.hbmColor);
			DeleteObject(CursorInfo.hbmMask);
		}
	}

	hBitmap = (HBITMAP)SelectObject(hMemoryDC, hOldBitmap);

	GetDIBits(hMemoryDC, hBitmap, 0, screen_height, (LPVOID)screen_data, (BITMAPINFO*)&bmi, DIB_RGB_COLORS);

	for(int height=0;height<screen_height;++height) {
		memcpy(texture_data + height * texture_width * 4,
			   screen_data + height * screen_width * 4,
			   screen_width * 4);
		// Fill in alpha = 1.0
		for(int width=0;width<screen_width;++width)
			texture_data[(height * texture_width + width) * 4 + 3] = 255;
	}

	*screen_aspect = float(screen_width) / float(screen_height);
	*tex_extent = Vec2f(float(screen_width) / float(texture_width), float(screen_height) / float(texture_height));

	glBindTexture(GL_TEXTURE_2D, desktop_tex_id);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, texture_width, texture_height, 0, GL_BGRA_EXT, GL_UNSIGNED_BYTE, (void*)texture_data);
	return true;
}