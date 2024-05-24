#include "headers/mobile.h"
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <math.h>
#include <GL4D/gl4du.h>
#include <GL4D/gl4dg.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

// Déclaration des variables globales et constantes
mobile_t * _mobile = NULL;
spring_t * _springs = NULL;
static int _nb_mobiles = 0;
static int _nb_springs = 0;
static GLfloat _width = 1, _depth = 1;

// Déclaration des fonctions
static void quit(void);
static void frottements(int i, GLfloat kx, GLfloat ky, GLfloat kz);
static double get_dt(void);
static void applyBoidsRules(int i);
static void updateTargetDirection(int i);
static void avoidPredator(int i);
static void update_move(int i, float dt);
static void collision_collback(int i, float d);
static void applySpringForces(void);

static void normalize(GLfloat *v);
static GLfloat dotProduct(GLfloat *a, GLfloat *b);
static void crossProduct(GLfloat *a, GLfloat *b, GLfloat *result);
static void orientMobile(int i);

// Initialisation des mobiles
void mobileInit(int n, GLfloat width, GLfloat depth) {
  int i;
  _width = width; 
  _depth = depth;
  _nb_mobiles = n;
  if(_mobile) {
    free(_mobile);
    _mobile = NULL;
  } else {
    atexit(quit);
  }
  _mobile = malloc(_nb_mobiles * sizeof * _mobile);
  assert(_mobile);
  for(i = 0; i < _nb_mobiles; i++) {
    _mobile[i].r = 0.2f;
    _mobile[i].x = gl4dmSURand() * _width - _mobile[i].r;
    _mobile[i].z = gl4dmSURand() * _depth - _mobile[i].r;
    _mobile[i].y = _depth;
    _mobile[i].vx = 3.0f * (gl4dmSURand() - 0.5f);
    _mobile[i].vy = 3.0f * (gl4dmURand() - 0.5f);
    _mobile[i].vz = 3.0f * (gl4dmSURand() - 0.5f);
    _mobile[i].color[0] = gl4dmURand(); // Rouge
    _mobile[i].color[1] = gl4dmURand(); // Vert
    _mobile[i].color[2] = gl4dmURand(); // Bleu
    _mobile[i].color[3] = 1.0f; // Alpha
    _mobile[i].freeze = GL_FALSE;
    _mobile[i].y_direction_inversee = GL_FALSE;
    updateTargetDirection(i);
  }
  predatorInit(_nb_mobiles, _width, HAUTEUR_SEUIL, _depth);
}

// Initialisation des ressorts
void springInit(int n) {
  int i;
  _nb_springs = n;
  if(_springs) {
    free(_springs);
    _springs = NULL;
  }
  _springs = malloc(_nb_springs * sizeof * _springs);
  assert(_springs);
  for(i = 0; i < _nb_springs; i++) {
    _springs[i].a = rand() % _nb_mobiles;
    _springs[i].b = rand() % _nb_mobiles;
    _springs[i].rest_length = 10.0f;
  }
}

// Définit si un mobile est gelé ou non
void mobileSetFreeze(GLuint id, GLboolean freeze) {
  _mobile[id].freeze = freeze;
}

// Récupère les coordonnées d'un mobile
void mobileGetCoords(GLuint id, GLfloat * coords) {
  coords[0] = _mobile[id].x;
  coords[1] = _mobile[id].y;
  coords[2] = _mobile[id].z;
}

// Définit les coordonnées d'un mobile
void mobileSetCoords(GLuint id, GLfloat * coords) {
  _mobile[id].x = coords[0];
  _mobile[id].y = coords[1];
  _mobile[id].z = coords[2];
}

// Met à jour les positions et états des mobiles
void mobileMove(void) {
  int i;
  double dt = get_dt(), d;

  predatorMove(_plan_s, HAUTEUR_SEUIL, _plan_s);

  applySpringForces();

  for(i = 0; i < _nb_mobiles; i++) {
    if(_mobile[i].freeze) continue;

    updateTargetDirection(i);
    applyBoidsRules(i);
    avoidPredator(i);
    update_move(i, dt);
    collision_collback(i, d);
  }
}

// Applique les forces des ressorts
static void applySpringForces(void) {
  for(int i = 0; i < _nb_springs; i++) {
    int a = _springs[i].a;
    int b = _springs[i].b;
    GLfloat dx = _mobile[b].x - _mobile[a].x;
    GLfloat dy = _mobile[b].y - _mobile[a].y;
    GLfloat dz = _mobile[b].z - _mobile[a].z;
    GLfloat distance = sqrt(dx * dx + dy * dy + dz * dz);
    GLfloat force = K_RESSORT * (distance - _springs[i].rest_length);

    if(force > MAX_FORCE) force = MAX_FORCE;
    if(force < -MAX_FORCE) force = -MAX_FORCE;

    GLfloat fx = (dx / distance) * force;
    GLfloat fy = (dy / distance) * force;
    GLfloat fz = (dz / distance) * force;

    _mobile[a].vx += fx;
    _mobile[a].vy += fy;
    _mobile[a].vz += fz;

    _mobile[b].vx -= fx;
    _mobile[b].vy -= fy;
    _mobile[b].vz -= fz;
  }
}

// Normalise un vecteur
static void normalize(GLfloat *v) {
  GLfloat length = sqrt(v[0] * v[0] + v[1] * v[1] + v[2] * v[2]);
  if (length != 0) {
    v[0] /= length;
    v[1] /= length;
    v[2] /= length;
  }
}

// Calcule le produit scalaire de deux vecteurs
static GLfloat dotProduct(GLfloat *a, GLfloat *b) {
  return a[0] * b[0] + a[1] * b[1] + a[2] * b[2];
}

// Calcule le produit vectoriel de deux vecteurs
static void crossProduct(GLfloat *a, GLfloat *b, GLfloat *result) {
  result[0] = a[1] * b[2] - a[2] * b[1];
  result[1] = a[2] * b[0] - a[0] * b[2];
  result[2] = a[0] * b[1] - a[1] * b[0];
}

// Oriente le mobile selon sa direction de déplacement
static void orientMobile(int i) {
  GLfloat direction[] = { _mobile[i].vx, _mobile[i].vy, _mobile[i].vz };
  normalize(direction);

  GLfloat reference[] = { 0.0f, 0.0f, 1.0f };
  GLfloat dot = dotProduct(reference, direction);
  GLfloat angle = acos(dot) * 180.0f / M_PI;
  GLfloat axis[3];
  crossProduct(reference, direction, axis);
  normalize(axis);

  gl4duLoadIdentityf();
  gl4duTranslatef(_mobile[i].x, _mobile[i].y, _mobile[i].z);
  gl4duRotatef(angle, axis[0], axis[1], axis[2]);
}

// Dessine les mobiles
void mobileDraw(GLuint obj) {
  int i;
  GLint pId;
  GLfloat black[4] = {.05f, .0f, .1f, .2f}; 
  glGetIntegerv(GL_CURRENT_PROGRAM, &pId);
  for (i = 0; i < _nb_mobiles; i++) {
    gl4duPushMatrix();
    orientMobile(i);
    gl4duScalef(0.2f, 0.2f, 0.2f); 
    gl4duSendMatrices();
    glUniform1i(glGetUniformLocation(pId, "id"), i + 3);
    if (_color_bird) {
      glUniform4fv(glGetUniformLocation(pId, "couleur"), 1, _mobile[i].color);
    } else {
      glUniform4fv(glGetUniformLocation(pId, "couleur"), 1, black);
    }
    assimpDrawScene(obj);
    gl4duPopMatrix();
  }
}

// Calcule la distance entre deux mobiles
static GLfloat distance(mobile_t a, mobile_t b) {
  return sqrt((a.x - b.x) * (a.x - b.x) + 
              (a.y - b.y) * (a.y - b.y) + 
              (a.z - b.z) * (a.z - b.z));
}

// Applique des frottements aux vitesses d'un mobile
static void frottements(int i, GLfloat kx, GLfloat ky, GLfloat kz) {
  GLfloat vx = fabs(_mobile[i].vx), vy = fabs(_mobile[i].vy), vz = fabs(_mobile[i].vz);

  if(vx < EPSILON)  _mobile[i].vx = 0;
  else              _mobile[i].vx = (vx - kx * vx) * SIGN(_mobile[i].vx);
  
  if(vy < EPSILON)  _mobile[i].vy = 0;
  else              _mobile[i].vy = (vy - ky * vy) * SIGN(_mobile[i].vy);
  
  if(vz < EPSILON)  _mobile[i].vz = 0;
  else              _mobile[i].vz = (vz - kz * vz) * SIGN(_mobile[i].vz);
}

// Libère la mémoire utilisée par les mobiles
static void quit(void) {
  _nb_mobiles = 0;
  if(_mobile) {
    free(_mobile);
    _mobile = NULL;
  }
  if(_springs) {
    free(_springs);
    _springs = NULL;
  }
}

// Calcule le delta time entre deux frames
static double get_dt(void) {
  static double t0 = 0, t, dt;
  t = gl4dGetElapsedTime();
  dt = (t - t0) / 1000.0;
  t0 = t;
  return dt;
}

// Applique les règles des boids pour un mobile donné
static void applyBoidsRules(int i) {
  int j, count = 0;
  GLfloat avgVx = 0, avgVy = 0, avgVz = 0;
  GLfloat centerX = 0, centerY = 0, centerZ = 0;
  GLfloat separationX = 0, separationY = 0, separationZ = 0;
  GLfloat d;
  GLfloat influenceDistance = _mobile[i].r * 4;

  for(j = 0; j < _nb_mobiles; j++) {
    if(i == j) continue;
    d = distance(_mobile[i], _mobile[j]);
    if(d < influenceDistance) {
      centerX += _mobile[j].x;
      centerY += _mobile[j].y;
      centerZ += _mobile[j].z;
      avgVx += _mobile[j].vx;
      avgVy += _mobile[j].vy;
      avgVz += _mobile[j].vz;
      if(d < _mobile[i].r * 3) {
        separationX += _mobile[i].x - _mobile[j].x;
        separationY += _mobile[i].y - _mobile[j].y;
        separationZ += _mobile[i].z - _mobile[j].z;
      }
      count++;
    }
  }

  if(count > 0) {
    centerX /= count;
    centerY /= count;
    centerZ /= count;
    _mobile[i].vx += (centerX - _mobile[i].x) * (COHESION_WEIGHT / 2);
    _mobile[i].vy += (centerY - _mobile[i].y) * (COHESION_WEIGHT / 2);
    _mobile[i].vz += (centerZ - _mobile[i].z) * (COHESION_WEIGHT / 2);

    avgVx /= count;
    avgVy /= count;
    avgVz /= count;
    _mobile[i].vx += (avgVx - _mobile[i].vx) * ALIGNMENT_WEIGHT;
    _mobile[i].vy += (avgVy - _mobile[i].vy) * ALIGNMENT_WEIGHT;
    _mobile[i].vz += (avgVz - _mobile[i].vz) * ALIGNMENT_WEIGHT;

    _mobile[i].vx += separationX * (SEPARATION_WEIGHT * 2);
    _mobile[i].vy += separationY * (SEPARATION_WEIGHT * 2);
    _mobile[i].vz += separationZ * (SEPARATION_WEIGHT * 2);
  }
}

// Évite le prédateur pour un mobile donné
static void avoidPredator(int i) {
  GLfloat dx = _predator.x - _mobile[i].x;
  GLfloat dy = _predator.y - _mobile[i].y;
  GLfloat dz = _predator.z - _mobile[i].z;
  GLfloat d = sqrt(dx * dx + dy * dy + dz * dz);
  if(d < _mobile[i].r * 10) {
    _mobile[i].vx -= dx / d * PREDATOR_AVOIDANCE_WEIGHT;
    _mobile[i].vy -= dy / d * PREDATOR_AVOIDANCE_WEIGHT;
    _mobile[i].vz -= dz / d * PREDATOR_AVOIDANCE_WEIGHT;
  }
}

// Met à jour le mouvement d'un mobile
static void update_move(int i, float dt) {
  _mobile[i].vx += (_mobile[i].targetX - _mobile[i].x) * TARGET_WEIGHT;
  _mobile[i].vy += (_mobile[i].targetY - _mobile[i].y) * TARGET_WEIGHT;
  _mobile[i].vz += (_mobile[i].targetZ - _mobile[i].z) * TARGET_WEIGHT;
  _mobile[i].vx *= DAMPING;
  _mobile[i].vy *= DAMPING;
  _mobile[i].vz *= DAMPING;
  if (_mobile[i].vx > VELOCITY_LIMIT) _mobile[i].vx = VELOCITY_LIMIT;
  if (_mobile[i].vx < -VELOCITY_LIMIT) _mobile[i].vx = -VELOCITY_LIMIT;
  if (_mobile[i].vy > VELOCITY_LIMIT) _mobile[i].vy = VELOCITY_LIMIT;
  if (_mobile[i].vy < -VELOCITY_LIMIT) _mobile[i].vy = -VELOCITY_LIMIT;
  if (_mobile[i].vz > VELOCITY_LIMIT) _mobile[i].vz = VELOCITY_LIMIT;
  if (_mobile[i].vz < -VELOCITY_LIMIT) _mobile[i].vz = -VELOCITY_LIMIT;
  _mobile[i].x += _mobile[i].vx * dt;
  _mobile[i].y += _mobile[i].vy * dt;
  _mobile[i].z += _mobile[i].vz * dt;
}

// Gère les collisions avec les bords de la boîte
static void collision_collback(int i, float d) {
  if((d = _mobile[i].x - _mobile[i].r + _width) <= EPSILON || 
     (d = _mobile[i].x + _mobile[i].r - _width) >= -EPSILON) {
    if(d * _mobile[i].vx > 0) _mobile[i].vx = -_mobile[i].vx;
    _mobile[i].x -= d - EPSILON;
    frottements(i, 0.1f, 0.0f, 0.1f);
  }

  if((d = _mobile[i].z - _mobile[i].r + _depth) <= EPSILON || 
     (d = _mobile[i].z + _mobile[i].r - _depth) >= -EPSILON) {
    if(d * _mobile[i].vz > 0) _mobile[i].vz = -_mobile[i].vz;
    _mobile[i].z -= d - EPSILON;
    frottements(i, 0.1f, 0.0f, 0.1f);
  }

  if((d = _mobile[i].y - _mobile[i].r) <= EPSILON) {
    if(_mobile[i].vy < 0) _mobile[i].vy = -_mobile[i].vy;
    _mobile[i].y -= d - EPSILON;
    _mobile[i].y_direction_inversee = GL_FALSE;
    _mobile[i].vy *= DAMPING;
    frottements(i, 0.1f, 0.0f, 0.1f);
  }

  if((d = _mobile[i].y + _mobile[i].r - HAUTEUR_SEUIL) >= -EPSILON) {
    if(_mobile[i].vy > 0) _mobile[i].vy = -_mobile[i].vy;
    _mobile[i].y -= d - EPSILON;
    _mobile[i].y_direction_inversee = GL_FALSE;
    _mobile[i].vy *= DAMPING;
    frottements(i, 0.1f, 0.0f, 0.1f);
  }
}

// Met à jour la direction cible aléatoire d'un mobile
static void updateTargetDirection(int i) {
  _mobile[i].targetX = gl4dmSURand() * _width - _mobile[i].r;
  _mobile[i].targetY = gl4dmURand() * (HAUTEUR_SEUIL - (2 * _mobile[i].r)) + _mobile[i].r;
  _mobile[i].targetZ = gl4dmSURand() * _depth - _mobile[i].r;
}
