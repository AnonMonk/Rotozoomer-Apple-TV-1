#define STBI_NO_THREAD_LOCALS

#define STB_IMAGE_IMPLEMENTATION

#include "stb_image.h"
#include "TunnelEffect.h"

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
GLuint texTunnel = 0;

float animTime = 0.0f;
float rotoAnimTime = 0.0f;
float tunnelAnimTime = 0.0f;
float creditsAnimTime = 0.0f;

const int DEMO_FPS = 60;

enum
{
	SCENE_LOGO = 0,
	SCENE_LOGO_WAVE = 1,
	SCENE_ROTOZOOM = 2,
	SCENE_TUNNEL = 3,
	SCENE_CREDITS = 4,
	SCENE_GREETS = 5,
	SCENE_COUNT = 6
};

int sceneLen[SCENE_COUNT];

GLuint texFishHead = 0;
GLuint texFishTail = 0;

struct LogoLayout
{
	float headX;
	float headY;
	float headW;
	float headH;
	float tailX;
	float tailY;
	float tailW;
	float tailH;
	float bodyStartX;
	float bodyStartY;
	float bodyEndX;
	float bodyEndY;
};

const char* creditsLines[] =
{
	"CREDITS",
	"",
	"ANON MONK PRESENTS",
	"",
	"ROTOZOOMER APPLE TV 1 TEST",
	"",
	"CODE:",
	"ANON MONK",
	"",
	"DESIGN:",
	"ANON MONK",
	"",
	"GRAPHICS:",
	"ANON MONK",
	"",
	"SUPPORT:",
	"KEY REAL",
	"",
	"C++ / OPENGL",
	"",
	"MADE FOR FUN.",
	"MADE FOR THE APPLE TV",
	"1ST GENERATION."
};

const char* greetsLines[] =
{
	"GREETS",
	"TO EVERYONE STILL CODING DEMOS",
	"TO PIXEL PUSHERS",
	"TO TRACKER MUSICIANS",
	"TO OLD HARDWARE FANS",
	"TO THE SCENE"
};

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

void getSceneState(int totalFrame, int* scene, int* localFrame)
{
	int f = totalFrame;

	for (int i = 0; i < SCENE_COUNT; i++)
	{
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

	*scene = SCENE_COUNT - 1;
	*localFrame = sceneLen[SCENE_COUNT - 1] - 1;
}

void rotoTextCoord(float x, float y)
{
	float angle = rotoAnimTime * 0.60f;
	float zoom = 0.78f + 0.15f * sinf(rotoAnimTime * 0.90f);

	float cx = 0.5f + 0.14f * sinf(rotoAnimTime * 0.36f);
	float cy = 0.5f + 0.14f * cosf(rotoAnimTime * 0.48f);

	float ca = cosf(angle);
	float sa = sinf(angle);

	float u = cx + (x * ca - y * sa) * zoom;
	float v = cy + (x * sa + y * ca) * zoom;

	glTexCoord2f(u, v);
}

void drawRotozoomer()
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

	glBegin(GL_QUADS);
		rotoTextCoord(-0.5f,  0.5f); glVertex2f(-1, -1);
		rotoTextCoord( 0.5f,  0.5f); glVertex2f( 1, -1);
		rotoTextCoord( 0.5f, -0.5f); glVertex2f( 1,  1);
		rotoTextCoord(-0.5f, -0.5f); glVertex2f(-1,  1);
	glEnd();
}

void drawRotozoomerBase()
{
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();

	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();

	glDisable(GL_BLEND);
	glEnable(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D, tex);

	glColor3f(1, 1, 1);

	glBegin(GL_QUADS);
		rotoTextCoord(-0.5f,  0.5f); glVertex2f(-1, -1);
		rotoTextCoord( 0.5f,  0.5f); glVertex2f( 1, -1);
		rotoTextCoord( 0.5f, -0.5f); glVertex2f( 1,  1);
		rotoTextCoord(-0.5f, -0.5f); glVertex2f(-1,  1);
	glEnd();
}

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

void drawTexturedQuadWavy(GLuint t, float x, float y, float w, float h, float alpha, float waveAmp)
{
	if (t == 0) return;

	const int bands = 24;

	glEnable(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D, t);

	for (int i = 0; i < bands; i++)
	{
		float t0 = (float)i / (float)bands;
		float t1 = (float)(i + 1) / (float)bands;
		float y0 = y + h * t0;
		float y1 = y + h * t1;
		float center = (t0 + t1) * 0.5f;

		float shift0 = sinf(animTime * 13.0f + center * 18.0f) * waveAmp;
		float shift1 = sinf(animTime * 13.0f + center * 18.0f) * waveAmp;

		glColor4f(1.0f, 1.0f, 1.0f, alpha);
		glBegin(GL_QUADS);
			glTexCoord2f(0.0f, t0); glVertex2f(x + shift0,     y0);
			glTexCoord2f(1.0f, t0); glVertex2f(x + w + shift0, y0);
			glTexCoord2f(1.0f, t1); glVertex2f(x + w + shift1, y1);
			glTexCoord2f(0.0f, t1); glVertex2f(x + shift1,     y1);
		glEnd();
	}
}

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

void drawOscFishBodyTransition(float x0, float y0, float x1, float y1, float alpha, float waveAmp)
{
	const int samples = 96;

	float amp = 22.0f + waveAmp * 0.18f;
	float thickness = 13.0f;

	glDisable(GL_TEXTURE_2D);

	glColor4f(0.55f, 0.55f, 0.55f, alpha);

	glBegin(GL_TRIANGLE_STRIP);
	for (int i = 0; i < samples; i++)
	{
		float t = (float)i / (float)(samples - 1);

		float x = x0 + (x1 - x0) * t;
		float y = y0 + (y1 - y0) * t;

		float envelope = sinf(t * 3.1415926f);
		float v = fishOscSample(t);

		float xShift = sinf(animTime * 11.0f + t * 19.0f) * waveAmp * envelope;
		float yShift = sinf(animTime * 7.0f + t * 15.0f) * waveAmp * 0.25f * envelope;

		y += v * amp * envelope + yShift;

		float thick = thickness * envelope;

		glVertex2f(x + xShift, y - thick);
		glVertex2f(x + xShift, y + thick);
	}
	glEnd();

	glLineWidth(3.0f);
	glColor4f(1.0f, 1.0f, 1.0f, alpha);

	glBegin(GL_LINE_STRIP);
	for (int i = 0; i < samples; i++)
	{
		float t = (float)i / (float)(samples - 1);

		float x = x0 + (x1 - x0) * t;
		float y = y0 + (y1 - y0) * t;

		float envelope = sinf(t * 3.1415926f);
		float v = fishOscSample(t);

		float xShift = sinf(animTime * 11.0f + t * 19.0f) * waveAmp * envelope;
		float yShift = sinf(animTime * 7.0f + t * 15.0f) * waveAmp * 0.25f * envelope;

		y += v * amp * envelope + yShift;

		glVertex2f(x + xShift, y);
	}
	glEnd();

	glLineWidth(1.0f);
	glEnable(GL_TEXTURE_2D);
}

LogoLayout getLogoLayout(float centerX, float centerY, float scale)
{
	LogoLayout layout;

	layout.headW = 106.0f * scale;
	layout.headH = 94.0f * scale;
	layout.tailW = 70.0f * scale;
	layout.tailH = 94.0f * scale;

	float bodyLen = 130.0f * scale;
	float totalW = layout.headW + bodyLen + layout.tailW;

	layout.headX = centerX - totalW * 0.5f;
	layout.headY = centerY - layout.headH * 0.5f;
	layout.bodyStartX = layout.headX + layout.headW - 7.0f * scale;
	layout.bodyStartY = centerY;
	layout.bodyEndX = layout.bodyStartX + bodyLen;
	layout.bodyEndY = centerY;
	layout.tailX = layout.bodyEndX - 3.0f * scale;
	layout.tailY = centerY - layout.tailH * 0.5f;

	return layout;
}

void drawOscFishLogo(float centerX, float centerY, float scale)
{
	LogoLayout layout = getLogoLayout(centerX, centerY, scale);

	drawTexturedQuad(texFishHead, layout.headX, layout.headY, layout.headW, layout.headH);
	drawOscFishBody(layout.bodyStartX, layout.bodyStartY, layout.bodyEndX, layout.bodyEndY);
	drawTexturedQuad(
		texFishTail,
		layout.tailX,
		layout.tailY,
		layout.tailW,
		layout.tailH
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

void drawLogoWaveTransition()
{
	glClearColor(0, 0, 0, 1);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	drawRotozoomerBase();

	begin2D();

	int w = glutGet(GLUT_WINDOW_WIDTH);
	int h = glutGet(GLUT_WINDOW_HEIGHT);

	float scale = 2.0f;
	if (w <= 400 || h <= 300)
	{
		scale = 0.6f;
	}

	float duration = (float)sceneLen[SCENE_LOGO_WAVE] / (float)DEMO_FPS;
	float progress = 0.0f;
	if (duration > 0.0f)
		progress = animTime / duration;

	if (progress < 0.0f)
		progress = 0.0f;
	if (progress > 1.0f)
		progress = 1.0f;

	float eased = progress * progress * (3.0f - 2.0f * progress);
	float alpha = 1.0f - eased;
	float waveAmp = 6.0f + eased * 90.0f;
	float logoShiftX = sinf(animTime * 9.0f) * (4.0f + eased * 16.0f);

	LogoLayout layout = getLogoLayout(w * 0.5f + logoShiftX, h * 0.5f, scale);

	drawTexturedQuadWavy(
		texFishHead,
		layout.headX,
		layout.headY,
		layout.headW,
		layout.headH,
		alpha,
		waveAmp
	);

	drawOscFishBodyTransition(
		layout.bodyStartX,
		layout.bodyStartY,
		layout.bodyEndX,
		layout.bodyEndY,
		alpha,
		waveAmp * 0.45f
	);

	drawTexturedQuadWavy(
		texFishTail,
		layout.tailX,
		layout.tailY,
		layout.tailW,
		layout.tailH,
		alpha,
		waveAmp
	);

	end2D();
}

float strokeTextWidth(const char* text)
{
	float width = 0.0f;

	for (const char* p = text; *p != '\0'; ++p)
	{
		width += (float)glutStrokeWidth(GLUT_STROKE_ROMAN, (int)(unsigned char)(*p));
	}

	return width;
}

void drawStrokeTextLine(const char* text)
{
	for (const char* p = text; *p != '\0'; ++p)
	{
		glutStrokeCharacter(GLUT_STROKE_ROMAN, (int)(unsigned char)(*p));
	}
}

float maxStrokeTextWidth(const char* const* lines, int lineCount)
{
	float maxWidth = 0.0f;

	for (int i = 0; i < lineCount; i++)
	{
		float width = strokeTextWidth(lines[i]);
		if (width > maxWidth)
			maxWidth = width;
	}

	return maxWidth;
}

void renderTextCrawlBlock(
	const char* const* lines,
	int lineCount,
	float effectTime,
	float cycleSeconds,
	bool loopScroll,
	float alpha,
	float wavePixels,
	float scaleWaveAmount,
	float startLeadLines,
	float endLeadLines
)
{
	int w = glutGet(GLUT_WINDOW_WIDTH);
	int h = glutGet(GLUT_WINDOW_HEIGHT);

	float timeInCycle = effectTime;
	if (loopScroll && cycleSeconds > 0.0f)
		timeInCycle = fmodf(effectTime, cycleSeconds);

	float progress = 0.0f;
	if (cycleSeconds > 0.0f)
	{
		progress = timeInCycle / cycleSeconds;
		if (progress > 1.0f)
			progress = 1.0f;
	}

	const float strokeHeight = 119.05f;
	float maxWidth = maxStrokeTextWidth(lines, lineCount);
	float horizontalMargin = (float)w * 0.06f;
	float usableWidth = (float)w - horizontalMargin * 2.0f;
	float scale = usableWidth / maxWidth;
	float lineStep = strokeHeight * scale * 1.18f;
	float blockHeight = lineCount * lineStep;

	float startTop = (float)h + strokeHeight * scale * startLeadLines;
	float endTop = -blockHeight - strokeHeight * scale * endLeadLines;
	float blockTop = startTop + (endTop - startTop) * progress;
	float blockOffsetX = sinf(timeInCycle * 2.6f) * wavePixels;
	float blockScale = 1.0f + scaleWaveAmount * sinf(timeInCycle * 2.1f);

	glLineWidth(3.0f);

	for (int i = 0; i < lineCount; i++)
	{
		float lineTop = blockTop + (float)i * lineStep;
		float lineBottom = lineTop + strokeHeight * scale;
		if (lineBottom < -20.0f || lineTop > (float)h + 20.0f)
			continue;

		float width = strokeTextWidth(lines[i]) * scale;
		float x = ((float)w - width) * 0.5f + blockOffsetX;
		float y = lineTop + strokeHeight * scale;

		glPushMatrix();
		glTranslatef(x, y, 0.0f);
		glScalef(scale * blockScale, -scale * blockScale, 1.0f);
		glColor4f(0.0f, 254.0f / 255.0f, 0.0f, alpha);
		drawStrokeTextLine(lines[i]);
		glPopMatrix();
	}

	glLineWidth(1.0f);
}

void drawTextCrawlScene(
	const char* const* lines,
	int lineCount,
	float effectTime,
	float cycleSeconds,
	bool loopScroll,
	float startLeadLines,
	float endLeadLines
)
{
	glClearColor(0, 0, 0, 1);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	begin2D();
	renderTextCrawlBlock(
		lines,
		lineCount,
		effectTime,
		cycleSeconds,
		loopScroll,
		1.0f,
		(float)glutGet(GLUT_WINDOW_WIDTH) * 0.08f,
		0.03f,
		startLeadLines,
		endLeadLines
	);
	end2D();
}

void drawCredits()
{
	drawTextCrawlScene(
		creditsLines,
		(int)(sizeof(creditsLines) / sizeof(creditsLines[0])),
		creditsAnimTime,
		9.0f,
		false,
		0.7f,
		0.9f
	);
}

void drawGreets()
{
	drawTextCrawlScene(
		greetsLines,
		(int)(sizeof(greetsLines) / sizeof(greetsLines[0])),
		animTime,
		9.0f,
		false,
		0.6f,
		0.8f
	);
}

void draw()
{
	float now = glutGet(GLUT_ELAPSED_TIME) * 0.001f;
	int totalFrame = (int)(now * DEMO_FPS);
	int totalDemoFrames = 0;

	for (int i = 0; i < SCENE_COUNT; i++)
	{
		if (sceneLen[i] > 0)
			totalDemoFrames += sceneLen[i];
	}

	if (totalFrame >= totalDemoFrames)
		exit(0);

	int scene = 0;
	int localFrame = 0;

	getSceneState(totalFrame, &scene, &localFrame);

	animTime = (float)localFrame / (float)DEMO_FPS;

	int rotoStartFrame = sceneLen[SCENE_LOGO];
	int rotoFrame = totalFrame - rotoStartFrame;
	if (rotoFrame < 0)
		rotoFrame = 0;
	rotoAnimTime = (float)rotoFrame / (float)DEMO_FPS;

	int tunnelStartFrame = sceneLen[SCENE_LOGO]
		+ sceneLen[SCENE_LOGO_WAVE]
		+ sceneLen[SCENE_ROTOZOOM];
	int tunnelFrame = totalFrame - tunnelStartFrame;
	if (tunnelFrame < 0)
		tunnelFrame = 0;
	tunnelAnimTime = (float)tunnelFrame / (float)DEMO_FPS;

	int creditsStartFrame = sceneLen[SCENE_LOGO]
		+ sceneLen[SCENE_LOGO_WAVE]
		+ sceneLen[SCENE_ROTOZOOM]
		+ sceneLen[SCENE_TUNNEL];
	int creditsFrame = totalFrame - creditsStartFrame;
	if (creditsFrame < 0)
		creditsFrame = 0;
	creditsAnimTime = (float)creditsFrame / (float)DEMO_FPS;

	switch (scene)
	{
		case SCENE_LOGO:
		{
			drawLogoIntro();
			glutSwapBuffers();
			return;
		}

		case SCENE_LOGO_WAVE:
		{
			drawLogoWaveTransition();
			glutSwapBuffers();
			return;
		}

		case SCENE_ROTOZOOM:
		{
			drawRotozoomer();
			glutSwapBuffers();
			return;
		}

		case SCENE_TUNNEL:
		{
			drawTunnelEffect(texTunnel, tunnelAnimTime);
			glutSwapBuffers();
			return;
		}

		case SCENE_CREDITS:
		{
			drawCredits();
			glutSwapBuffers();
			return;
		}

		case SCENE_GREETS:
		default:
		{
			drawGreets();
			glutSwapBuffers();
			return;
		}
	}
}

void key(unsigned char k, int x, int y)
{
	if (k == 27 || k == 'q')
		exit(0);
}

int main(int argc, char** argv)
{
	sceneLen[SCENE_LOGO]     = 60 * 5;
	sceneLen[SCENE_LOGO_WAVE] = 60 * 2;
	sceneLen[SCENE_ROTOZOOM] = 60 * 8;
	sceneLen[SCENE_TUNNEL]   = 60 * 7;
	sceneLen[SCENE_CREDITS]  = 60 * 9;
	sceneLen[SCENE_GREETS]   = 60 * 9;

	glutInit(&argc, argv);
	glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGBA);
	glutInitWindowSize(640,480);
	glutCreateWindow("Texture");

	glutFullScreen();

	glEnable(GL_TEXTURE_2D);

	std::string exeDir = getExecutableDir(argv);

	std::string imagePath = exeDir + "/Testbild.png";
	std::string tunnelPath = exeDir + "/bdl.png";
	std::string headPath  = exeDir + "/Kopf_transparent.png";
	std::string tailPath  = exeDir + "/Schwanz_transparent.png";

	tex = loadTextureRGBA(imagePath, true);
	texTunnel = loadTextureRGBA(tunnelPath, true);

	if (tex == 0)
	{
		printf("Testbild.png konnte nicht geladen werden.\n");
		printf("Testbild.png muss im gleichen Ordner wie die Programmdatei liegen.\n");
		getchar();
		return 1;
	}

	if (texTunnel == 0)
	{
		printf("bdl.png konnte nicht geladen werden.\n");
		printf("bdl.png muss im gleichen Ordner wie die Programmdatei liegen.\n");
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
