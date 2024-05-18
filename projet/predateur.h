// predateur.h
#ifndef PREDATEUR_H
#define PREDATEUR_H

#include <GL4D/gl4du.h>
#include <GL4D/gl4dg.h>

// Structure du prédateur
typedef struct predator_t {
  GLfloat x, y, z, r;
  GLfloat vx, vy, vz;
  GLfloat color[4];
} predator_t;

extern predator_t _predator; // Déclaration de l'instance de prédateur

// Prototypes des fonctions
void predatorInit(GLfloat width, GLfloat height, GLfloat depth);
void predatorMove(GLfloat width, GLfloat height, GLfloat depth);
void predatorDraw(GLuint obj);

#endif // PREDATEUR_H
