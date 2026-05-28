#define GL_SILENCE_DEPRECATION

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

//#include <GL/freeglut.h>//nur windows 

//apple
#include <OpenGL/gl.h>

#include <GLUT/glut.h>

#include <cstdio>
#include <cstdlib>

GLuint tex;

void draw()
{

	glClearColor(0, 0, 0, 1);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT); //clear

	glEnable(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D, tex); //binden

	glColor3f(1, 1, 1);

	glBegin(GL_QUADS);//quadrat positionen
		glTexCoord2f(0, 1); glVertex2f(-1, -1);
		glTexCoord2f(1, 1); glVertex2f(1, -1);
		glTexCoord2f(1, 0); glVertex2f(1, 1);
		glTexCoord2f(0, 0); glVertex2f(-1, 1);
	glEnd();

	glutSwapBuffers();
}

void key(unsigned char k, int x, int y)
{
	if (k == 27 || k == 'q')
		exit(0);
}

int main(int argc, char** argv )
{
	glutInit(&argc, argv);
	glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGBA);
	glutInitWindowSize(640,480);
	glutCreateWindow("Texture");

	glEnable(GL_TEXTURE_2D);

	int w, h, c;
	unsigned char* img = stbi_load("/Users/julius_munch/Documents/Code/Apple TV 1/Testbild.png",&w, &h, &c, 4);

	if (!img)
	{
		printf("Konnte png nicht ladnen\n");
		return 1;
	}

	printf("Bild geladen %d x %d\n", w, h);

	glGenTextures(1, &tex);
	glBindTexture(GL_TEXTURE_2D, tex);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

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