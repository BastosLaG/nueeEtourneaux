#ifndef PREDATEUR_H
#define PREDATEUR_H

#include <GL4D/gl4du.h>
#include <GL4D/gl4dg.h>

typedef struct {
  GLfloat x, y, z, r;
  GLfloat vx, vy, vz;
  GLfloat color[4];
  GLuint nb_mobiles;
  GLfloat changeDirectionCounter;
  GLfloat targetX, targetY, targetZ;
} predator_t;

extern predator_t _predator;

void predatorInit(int nb_mobiles, GLfloat width, GLfloat height, GLfloat depth);
void predatorMove(GLfloat width, GLfloat height, GLfloat depth);
void predatorDraw(GLuint obj);

#endif