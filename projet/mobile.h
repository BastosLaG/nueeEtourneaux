#ifndef MOBILE_H
#define MOBILE_H

#include <GL4D/gl4du.h>
#include <GL4D/gl4dg.h>

extern GLfloat _plan_s;

#define HAUTEUR_SEUIL 7.0f
#define EPSILON 0.00001f
#define K_RESSORT 1.5f // Constante de raideur du ressort
#define MAX_FORCE 0.1f // Force maximale pour limiter l'accélération
#define NUM_NEIGHBORS 6 // Nombre de voisins les plus proches
#define DAMPING 0.99f // Facteur d'amortissement pour stabiliser le mouvement
#define ALIGNMENT_WEIGHT 0.5f // Poids pour l'alignement
#define COHESION_WEIGHT 1.0f // Poids pour la cohésion
#define SEPARATION_WEIGHT 1.0f // Poids pour la séparation
#define AVOIDANCE_WEIGHT 1.5f // Poids pour éviter les murs
#define TARGET_WEIGHT 0.05f // Poids pour la direction cible
#define VELOCITY_LIMIT 3.0f // Limite de la vitesse
#define TARGET_VELOCITY 2.0f // Vitesse cible
#define REPULSION_MULTIPLIER 2.0f
#define PREDATOR_AVOIDANCE_WEIGHT 2.0f // Poids pour éviter le prédateur

typedef struct mobile_t {
  GLuint id;
  GLfloat x, y, z, r;
  GLfloat vx, vy, vz;
  GLboolean enAlerte;
  GLfloat color[4];
  GLboolean freeze;
  GLboolean y_direction_inversee;
  GLfloat targetX, targetY, targetZ; // Direction cible
} mobile_t;

void mobileInit(int n, GLfloat width, GLfloat depth);
void mobileSetFreeze(GLuint id, GLboolean freeze);
void mobileGetCoords(GLuint id, GLfloat * coords);
void mobileSetCoords(GLuint id, GLfloat * coords);
void mobileMove(void);
void mobileDraw(GLuint obj);
void attraction(GLuint id, GLfloat * coords);
void repulsion(GLuint id, GLfloat * coords);
void applySpringForce(GLuint id, GLuint neighborId); 
#endif