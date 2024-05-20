#ifndef PREDATEUR_H
#define PREDATEUR_H

#include <GL4D/gl4du.h>
#include <GL4D/gl4dg.h>

typedef struct {
  GLfloat x, y, z, r;
  GLfloat vx, vy, vz;
  GLfloat color[4];
} predator_t;

static predator_t _predator;

void predatorInit(GLfloat width, GLfloat height, GLfloat depth);
void predatorMove(GLfloat width, GLfloat height, GLfloat depth);
void predatorDraw(GLuint obj);

#endif