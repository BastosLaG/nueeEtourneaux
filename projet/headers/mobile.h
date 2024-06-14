#ifndef MOBILE_H
#define MOBILE_H

#include <GL4D/gl4du.h>
#include <GL4D/gl4dg.h>

#include "assimp.h"
#include "predateur.h"

extern GLfloat _plan_s;

#define HAUTEUR_SEUIL 7.0f
#define EPSILON 0.00001f
#define K_RESSORT 1.5f
#define MAX_FORCE 0.5f
#define NUM_NEIGHBORS 6
#define DAMPING 0.99f
#define ALIGNMENT_WEIGHT 0.5f
#define COHESION_WEIGHT 1.0f 
#define SEPARATION_WEIGHT 1.0f
#define AVOIDANCE_WEIGHT 1.5f
#define TARGET_WEIGHT 0.05f
#define VELOCITY_LIMIT 5.0f
#define TARGET_VELOCITY 2.0f
#define REPULSION_MULTIPLIER 2.0f
#define PREDATOR_AVOIDANCE_WEIGHT 2.0f

typedef struct mobile_t {
  GLuint id;
  GLfloat x, y, z, r;
  GLfloat vx, vy, vz;
  GLboolean enAlerte;
  GLfloat color[4];
  GLboolean freeze;
  GLboolean y_direction_inversee;
  GLfloat targetX, targetY, targetZ;
} mobile_t;

extern GLboolean _color_bird;
typedef struct spring_t {
  int a, b;  
  GLfloat rest_length;} spring_t;

extern mobile_t * _mobile;
extern spring_t * _springs;
extern GLboolean useBoids;

void mobileInit(int n, GLfloat width, GLfloat depth);
void mobileSetFreeze(GLuint id, GLboolean freeze);
void mobileGetCoords(GLuint id, GLfloat * coords);
void mobileSetCoords(GLuint id, GLfloat * coords);
void mobileMove(void);
void mobileDraw(GLuint obj);
void springInit(int n);
void applyBoidsRules(int i);
#endif
