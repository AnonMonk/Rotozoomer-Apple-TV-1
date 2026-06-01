#define STBI_NO_THREAD_LOCALS

#define STB_IMAGE_IMPLEMENTATION

#include "stb_image.h"

#include <cmath>
#include <cstring>
#include <cstdio>
#include <cstdlib>
#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <limits.h>
#include <stdint.h>

#ifdef _WIN32
	#include <windows.h>
	#include <direct.h>
	#define getcwd _getcwd
#else
	#include <unistd.h>
#endif

#ifdef __APPLE__
	#include <mach-o/dyld.h>
#endif

#ifdef _WIN32
	#include <GL/freeglut.h>
#elif defined(__APPLE__)
	#include <OpenGL/gl.h>
	#include <GLUT/glut.h>
#else
	#include <OpenGL/gl.h>
	#include <GLUT/glut.h>
#endif

GLuint tex = 0;

//rotozoom
float animTime = 0.0f;

//Scenesteuerung
const int DEMO_FPS = 60;

enum
{
	SCENE_LOGO = 0,
	SCENE_ROTOZOOM = 1,
	SCENE_COUNT = 2
};

int sceneLen[SCENE_COUNT];

//Logo-Intro
GLuint texFishHead = 0;
GLuint texFishTail = 0;

//Programmordner holen
std::string getExecutableDir(char** argv)
{
#ifdef _WIN32
	char path[MAX_PATH];

	DWORD len = GetModuleFileNameA(NULL, path, MAX_PATH);
	if (len > 0 && len < MAX_PATH) {
		std::string fullPath = path;
		size_t pos = fullPath.find_last_of("/\\");
		if (pos != std::string::npos)
			return fullPath.substr(0, pos);
	}
#endif

#ifdef __APPLE__
	char path[PATH_MAX];
	uint32_t size = sizeof(path);

	if (_NSGetExecutablePath(path, &size) == 0) {
		std::string fullPath = path;
		size_t pos = fullPath.find_last_of("/\\");
		if (pos != std::string::npos)
			return fullPath.substr(0, pos);
	}
#endif

	std::string fallback = argv[0];
	size_t pos = fallback.find_last_of("/\\");
	if (pos != std::string::npos)
		return fallback.substr(0, pos);

	return ".";
}

//Textur laden
//repeatMode true = Rotozoomer
//repeatMode false = Logo
GLuint loadTextureRGBA(const std::string& filename, bool repeatMode)
{
	int w, h, c;

	printf("Lade Bild: %s\n", filename.c_str());

	unsigned char* img = stbi_load(filename.c_str(), &w, &h, &c, 4);
	if (!img)
	{
		printf("Konnte PNG nicht laden\n");
		printf("Gesuchter Pfad: %s\n", filename.c_str());
		printf("STB Fehler: %s\n", stbi_failure_reason());
		return 0;
	}

	printf("Bild geladen: %d x %d\n", w, h);

	GLuint t;
	glGenTextures(1, &t);
	glBindTexture(GL_TEXTURE_2D, t);

	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	if (repeatMode)
	{
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	}
	else
	{
#ifdef GL_CLAMP_TO_EDGE
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
#else
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
#endif
	}

	glTexImage2D(
		GL_TEXTURE_2D,
		0,
		GL_RGBA,
		w,
		h,
		0,
		GL_RGBA,
		GL_UNSIGNED_BYTE,
		img
	);

	stbi_image_free(img);

	return t;
}

//Scene berechnen
void getSceneState(int totalFrame, int* scene, int* localFrame)
{
	int f = totalFrame;

	for (int i = 0; i < SCENE_COUNT; i++)
	{
		// -1 bedeutet: Scene läuft endlos
		if (sceneLen[i] < 0)
		{
			*scene = i;
			*localFrame = f;
			return;
		}

		if (f < sceneLen[i])
		{
			*scene = i;
			*localFrame = f;
			return;
		}

		f -= sceneLen[i];
	}

	//Falls Demo vorbei: letzte Scene halten
	*scene = SCENE_COUNT - 1;
	*localFrame = 0;
}

//Rotozoomer definition
void rotoTextCoord(float x, float y)
{
	float angle = animTime * 0.60f;
	float zoom = 0.78f + 0.15f * sinf(animTime * 0.90f);

	float cx = 0.5f + 0.14f * sinf(animTime * 0.36f);
	float cy = 0.5f + 0.14f * cosf(animTime * 0.48f);

	float ca = cosf(angle);
	float sa = sinf(angle);

	float u = cx + (x * ca - y * sa) * zoom;
	float v = cy + (x * sa + y * ca) * zoom;

	glTexCoord2f(u, v);
}

void drawRotozoomer()//zeichnet Rotozoomer
{
	glClearColor(0, 0, 0, 1);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();

	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();

	glDisable(GL_BLEND);
	glEnable(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D, tex);

	glColor3f(1, 1, 1);

	glBegin(GL_QUADS);//quadrat positionen
		rotoTextCoord(-0.5f,  0.5f); glVertex2f(-1, -1);
		rotoTextCoord( 0.5f,  0.5f); glVertex2f( 1, -1);
		rotoTextCoord( 0.5f, -0.5f); glVertex2f( 1,  1);
		rotoTextCoord(-0.5f, -0.5f); glVertex2f(-1,  1);
	glEnd();
}

//2D Modus für Logo
void begin2D()
{
	int w = glutGet(GLUT_WINDOW_WIDTH);
	int h = glutGet(GLUT_WINDOW_HEIGHT);

	glMatrixMode(GL_PROJECTION);
	glPushMatrix();
	glLoadIdentity();

	glOrtho(0, w, h, 0, -1, 1);

	glMatrixMode(GL_MODELVIEW);
	glPushMatrix();
	glLoadIdentity();

	glDisable(GL_DEPTH_TEST);
	glEnable(GL_TEXTURE_2D);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}

void end2D()
{
	glDisable(GL_BLEND);

	glMatrixMode(GL_MODELVIEW);
	glPopMatrix();

	glMatrixMode(GL_PROJECTION);
	glPopMatrix();

	glMatrixMode(GL_MODELVIEW);
}

void drawTexturedQuad(GLuint t, float x, float y, float w, float h)
{
	if (t == 0) return;

	glEnable(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D, t);
	glColor4f(1.0f, 1.0f, 1.0f, 1.0f);

	glBegin(GL_QUADS);
		glTexCoord2f(0.0f, 0.0f); glVertex2f(x,     y);
		glTexCoord2f(1.0f, 0.0f); glVertex2f(x + w, y);
		glTexCoord2f(1.0f, 1.0f); glVertex2f(x + w, y + h);
		glTexCoord2f(0.0f, 1.0f); glVertex2f(x,     y + h);
	glEnd();
}

//Oszi-Körper vom Fisch
float fishOscSample(float t)
{
	float v = 0.0f;

	v += sinf(animTime * 5.0f + t * 18.0f) * 0.60f;
	v += sinf(animTime * 8.5f + t * 33.0f) * 0.25f;
	v += sinf(animTime * 2.0f + t * 11.0f) * 0.15f;

	return v;
}

void drawOscFishBody(float x0, float y0, float x1, float y1)
{
	const int samples = 96;

	float amp = 22.0f;
	float thickness = 13.0f;

	glDisable(GL_TEXTURE_2D);

	//grauer Oszi-Körper
	glColor4f(0.55f, 0.55f, 0.55f, 1.0f);

	glBegin(GL_TRIANGLE_STRIP);
	for (int i = 0; i < samples; i++)
	{
		float t = (float)i / (float)(samples - 1);

		float x = x0 + (x1 - x0) * t;
		float y = y0 + (y1 - y0) * t;

		float envelope = sinf(t * 3.1415926f);
		float v = fishOscSample(t);

		y += v * amp * envelope;

		float thick = thickness * envelope;

		glVertex2f(x, y - thick);
		glVertex2f(x, y + thick);
	}
	glEnd();

	//helle Oszi-Linie
	glLineWidth(3.0f);
	glColor4f(1.0f, 1.0f, 1.0f, 1.0f);

	glBegin(GL_LINE_STRIP);
	for (int i = 0; i < samples; i++)
	{
		float t = (float)i / (float)(samples - 1);

		float x = x0 + (x1 - x0) * t;
		float y = y0 + (y1 - y0) * t;

		float envelope = sinf(t * 3.1415926f);
		float v = fishOscSample(t);

		y += v * amp * envelope;

		glVertex2f(x, y);
	}
	glEnd();

	glLineWidth(1.0f);
	glEnable(GL_TEXTURE_2D);
}

//Logo: Kopf links, Oszi Mitte, Schwanz rechts
void drawOscFishLogo(float centerX, float centerY, float scale)
{
	float headW = 106.0f * scale;
	float headH = 94.0f  * scale;

	float tailW = 70.0f * scale;
	float tailH = 94.0f * scale;

	float bodyLen = 130.0f * scale;

	float totalW = headW + bodyLen + tailW;

	//Kopf links
	float headX = centerX - totalW * 0.5f;
	float headY = centerY - headH * 0.5f;

	//Oszi beginnt rechts vom Kopf
	float bodyStartX = headX + headW - 7.0f * scale;
	float bodyStartY = centerY;

	//Oszi endet vor dem Schwanz
	float bodyEndX = bodyStartX + bodyLen;
	float bodyEndY = centerY;

	//Schwanz rechts
	float tailX = bodyEndX - 3.0f * scale;

	//Kopf zeichnen
	drawTexturedQuad(texFishHead, headX, headY, headW, headH);

	//Oszi zeichnen
	drawOscFishBody(bodyStartX, bodyStartY, bodyEndX, bodyEndY);

	//Schwanz zeichnen
	float tailY = centerY -tailH * 0.5f;
	drawTexturedQuad(
		texFishTail,
		tailX,
		tailY,
		tailW,
		tailH
	);
}

void drawLogoIntro()
{
	glClearColor(0, 0, 0, 1);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	begin2D();

	int w = glutGet(GLUT_WINDOW_WIDTH);
	int h = glutGet(GLUT_WINDOW_HEIGHT);

	float scale = 2.0f;

	if (w <= 400 || h <= 300)
	{
		scale = 0.6f;
	}

	drawOscFishLogo(w * 0.5f, h * 0.5f, scale);

	end2D();
}

void draw()
{
	float now = glutGet(GLUT_ELAPSED_TIME) * 0.001f;
	int totalFrame = (int)(now * DEMO_FPS);

	int scene = 0;
	int localFrame = 0;

	getSceneState(totalFrame, &scene, &localFrame);

	animTime = (float)localFrame / (float)DEMO_FPS;

	//Scene berechnen
	switch (scene)
	{
		case SCENE_LOGO:
		{
			drawLogoIntro();
			glutSwapBuffers();
			return;
		}

		case SCENE_ROTOZOOM:
		default:
		{
			drawRotozoomer();
			glutSwapBuffers();
			return;
		}
	}
}

void key(unsigned char k, int x, int y)//esc key exit
{
	if (k == 27 || k == 'q')
		exit(0);
}

int main(int argc, char** argv)
{
	//Scene-Längen
	sceneLen[SCENE_LOGO]     = 60 * 5;   //0 Logo
	sceneLen[SCENE_ROTOZOOM] = -1;       //1 Rotozoomer endlos

	glutInit(&argc, argv);
	glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGBA);
	glutInitWindowSize(640,480);
	glutCreateWindow("Texture");

	glutFullScreen();

	glEnable(GL_TEXTURE_2D);

	std::string exeDir = getExecutableDir(argv);

	std::string imagePath = exeDir + "/Testbild.png";
	std::string headPath  = exeDir + "/Kopf_transparent.png";
	std::string tailPath  = exeDir + "/Schwanz_transparent.png";

	tex = loadTextureRGBA(imagePath, true);

	if (tex == 0)
	{
		printf("Testbild.png konnte nicht geladen werden.\n");
		printf("Testbild.png muss im gleichen Ordner wie die Programmdatei liegen.\n");
		getchar();
		return 1;
	}

	texFishHead = loadTextureRGBA(headPath, false);
	texFishTail = loadTextureRGBA(tailPath, false);

	if (texFishHead == 0)
	{
		printf("Kopf_transparent.png konnte nicht geladen werden.\n");
		printf("Kopf_transparent.png muss im gleichen Ordner wie die Programmdatei liegen.\n");
		getchar();
		return 1;
	}

	if (texFishTail == 0)
	{
		printf("Schwanz_transparent.png konnte nicht geladen werden.\n");
		printf("Schwanz_transparent.png muss im gleichen Ordner wie die Programmdatei liegen.\n");
		getchar();
		return 1;
	}

	glutDisplayFunc(draw);
	glutIdleFunc(draw);
	glutKeyboardFunc(key);

	glutMainLoop();

	return 0;
}