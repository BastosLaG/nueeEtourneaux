#ifndef MOBILE_H
#define MOBILE_H

#include <GL4D/gl4du.h>
#include <GL4D/gl4dg.h>

#define HAUTEUR_SEUIL 7.0f

void mobileInit(int n, GLfloat width, GLfloat depth);
void mobileMove(void);
void mobileDraw(GLuint obj);
void mobileSetFreeze(GLuint id, GLboolean freeze);
void mobileGetCoords(GLuint id, GLfloat * coords);
void mobileSetCoords(GLuint id, GLfloat * coords);

extern GLfloat _plan_s;

#endif // MOBILE_H

