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

struct Distance {
  float dist;
  int index;
};

#define MAX_DEPTH 10
#define MAX_OBJECTS 10

typedef struct OctreeNode_t {
  GLfloat x, y, z;
  GLfloat size;
  struct OctreeNode_t *children[8];
  int *mobiles;
  int num_mobiles;
  int is_leaf;
} OctreeNode_t;

extern GLboolean _color_bird;
typedef struct spring_t {
  int a, b;  
  GLfloat rest_length;} spring_t;

extern mobile_t * _mobile;
extern spring_t * _springs;
extern OctreeNode_t * _root;

extern GLboolean useBoids;

void mobileInit(int, GLfloat, GLfloat);
void mobileSetFreeze(GLuint, GLboolean);
void mobileGetCoords(GLuint, GLfloat *);
void mobileSetCoords(GLuint, GLfloat *);
void mobileMove(void);
void mobileDraw(GLuint);
void springInit(int);
void applyBoidsRules(int);
int compareDistances(const void *, const void *);

OctreeNode_t* createOctreeNode(GLfloat, GLfloat, GLfloat, GLfloat);
void insertMobile(OctreeNode_t *, int, int);
void freeOctreeNode(OctreeNode_t *);
void findClosestNeighbors(int, int*);

#endif
