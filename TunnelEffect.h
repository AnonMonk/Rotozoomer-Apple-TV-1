#ifndef TUNNEL_EFFECT_H
#define TUNNEL_EFFECT_H

#ifdef _WIN32
	#include <GL/freeglut.h>
#elif defined(__APPLE__)
	#include <OpenGL/gl.h>
	#include <GLUT/glut.h>
#else
	#include <OpenGL/gl.h>
	#include <GLUT/glut.h>
#endif

void drawTunnelEffect(GLuint texture, float animTime);

#endif
