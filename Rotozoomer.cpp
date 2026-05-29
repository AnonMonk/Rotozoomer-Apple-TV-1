#define STBI_NO_THREAD_LOCALS

#define STB_IMAGE_IMPLEMENTATION

#include "stb_image.h"

#include <cmath>
#include <cstring>
#include <cstdio>
#include <cstdlib>
#include <stdio.h>
#include <stdlib.h>

#ifdef _WIN32
	#include <direct.h>
	#define getcwd _getcwd
#else
	#include <unistd.h>
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

GLuint tex;

//rotozoom
float animTime= 0.0f;

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

void draw()
{

	animTime = glutGet(GLUT_ELAPSED_TIME) * 0.001f;

	glClearColor(0, 0, 0, 1);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT); //clear

	glEnable(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D, tex); //binden

	glColor3f(1, 1, 1);

	glBegin(GL_QUADS);//quadrat positionen
		rotoTextCoord(-0.5f,  0.5f); glVertex2f(-1, -1);
		rotoTextCoord( 0.5f,  0.5f); glVertex2f(1, -1);
		rotoTextCoord( 0.5f, -0.5f); glVertex2f(1, 1);
		rotoTextCoord(-0.5f, -0.5f); glVertex2f(-1, 1);
	glEnd();

	glutSwapBuffers();
}

void key(unsigned char k, int x, int y)//esc key exit
{
	if (k == 27 || k == 'q')
		exit(0);
}

 int main(int argc, char** argv )
/* {
char *path = getcwd(NULL, 0);

    if (path) {
        printf("%s\n", path);
        free(path);
    } else {
        printf("getcwd");
    }

    return 0;
}
*/
 {
	glutInit(&argc, argv);
	glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGBA);
	glutInitWindowSize(640,480);
	glutCreateWindow("Texture");

	glutFullScreen();

	glEnable(GL_TEXTURE_2D);

	int w, h, c;
	unsigned char* img = stbi_load("Testbild.png", &w, &h, &c, 4);
	if (!img)
	{
		printf("Konnte png nicht ladnen\n");
		printf("STB Fehler: %s\n", stbi_failure_reason());
		printf("Aktueller Arbeitsordner muss der Projektordner sein.\n");
		getchar();
		return 1;
	}

	printf("Bild geladen %d x %d\n", w, h);

	glGenTextures(1, &tex);
	glBindTexture(GL_TEXTURE_2D, tex);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

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

	glutDisplayFunc(draw);
	glutIdleFunc(draw);
	glutKeyboardFunc(key);

	glutMainLoop();

	return 0;
}
