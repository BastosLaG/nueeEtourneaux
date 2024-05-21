#include <GL4D/gl4du.h>
#include <GL4D/gl4dg.h>
#include <stdlib.h>
#include <assert.h>
#include "predateur.h"
#include "mobile.h"

predator_t _predator;
static void predateurUpdateTargetDirection(void);
static void predateurBoxCollider(GLfloat width, GLfloat height, GLfloat depth);
static GLfloat predateurDistance(void);
static double get_dt(void);

static GLfloat _width = 1, _depth = 1;

void predatorInit(int nb_mobiles, GLfloat width, GLfloat height, GLfloat depth) {
  _predator.r = 0.2f;
  _predator.x = gl4dmSURand() * width - _predator.r;
  _predator.y = height / 2;
  _predator.z = gl4dmSURand() * depth - _predator.r;
  _predator.vx = 0.1f;
  _predator.vy = 0.1f;
  _predator.vz = 0.1f;
  _predator.color[0] = 1.0f;
  _predator.color[1] = 0.0f;
  _predator.color[2] = 0.0f;
  _predator.color[3] = 1.0f;
  _predator.nb_mobiles = nb_mobiles;
  predateurUpdateTargetDirection();
}
void predatorMove(GLfloat width, GLfloat height, GLfloat depth) {
  double dt = get_dt();
  
  predateurUpdateTargetDirection();
  
  _predator.x += _predator.vx;
  _predator.y += _predator.vy;
  _predator.z += _predator.vz;



  // Appliquer la direction cible
  _predator.vx += (_predator.targetX - _predator.x) * TARGET_WEIGHT;
  _predator.vy += (_predator.targetY - _predator.y) * TARGET_WEIGHT;
  _predator.vz += (_predator.targetZ - _predator.z) * TARGET_WEIGHT;

  // Appliquer un facteur d'amortissement léger pour éviter l'augmentation exponentielle de la vitesse
  _predator.vx *= DAMPING;
  _predator.vy *= DAMPING;
  _predator.vz *= DAMPING;

  // Appliquer des limites de vitesse
  if (_predator.vx > VELOCITY_LIMIT) _predator.vx = VELOCITY_LIMIT;
  if (_predator.vx < -VELOCITY_LIMIT) _predator.vx = -VELOCITY_LIMIT;
  if (_predator.vy > VELOCITY_LIMIT) _predator.vy = VELOCITY_LIMIT;
  if (_predator.vy < -VELOCITY_LIMIT) _predator.vy = -VELOCITY_LIMIT;
  if (_predator.vz > VELOCITY_LIMIT) _predator.vz = VELOCITY_LIMIT;
  if (_predator.vz < -VELOCITY_LIMIT) _predator.vz = -VELOCITY_LIMIT;

  // Appliquer les vitesses à la position du mobile
  _predator.x += _predator.vx * dt;
  _predator.y += _predator.vy * dt;
  _predator.z += _predator.vz * dt;

  predateurBoxCollider(width, height, depth);

}
void predatorDraw(GLuint obj) {
  GLint pId;
  glGetIntegerv(GL_CURRENT_PROGRAM, &pId);
  gl4duPushMatrix();
  gl4duTranslatef(_predator.x, _predator.y, _predator.z);
  gl4duScalef(_predator.r, _predator.r, _predator.r);
  gl4duSendMatrices();
  gl4duPopMatrix();
  glUniform4fv(glGetUniformLocation(pId, "couleur"), 1, _predator.color);
  gl4dgDraw(obj);
}

static void predateurBoxCollider(GLfloat width, GLfloat height, GLfloat depth) {
  // Vérification des limites en X
  if (_predator.x - _predator.r < -width / 2) {
    _predator.x = -width / 2 + _predator.r;
    _predator.vx = -_predator.vx;
  }
  if (_predator.x + _predator.r > width / 2) {
    _predator.x = width / 2 - _predator.r;
    _predator.vx = -_predator.vx;
  }

  // Vérification des limites en Y
  if (_predator.y - _predator.r < 0) {
    _predator.y = _predator.r;
    _predator.vy = -_predator.vy;
  }
  if (_predator.y + _predator.r > height) {
    _predator.y = height - _predator.r;
    _predator.vy = -_predator.vy;
  }

  // Vérification des limites en Z
  if (_predator.z - _predator.r < -depth / 2) {
    _predator.z = -depth / 2 + _predator.r;
    _predator.vz = -_predator.vz;
  }
  if (_predator.z + _predator.r > depth / 2) {
    _predator.z = depth / 2 - _predator.r;
    _predator.vz = -_predator.vz;
  }
}

static GLfloat predateurDistance(void) {
  return sqrt((_predator.x - _predator.targetX) * (_predator.x - _predator.targetX) + 
              (_predator.y - _predator.targetY) * (_predator.y - _predator.targetY) + 
              (_predator.z - _predator.targetZ) * (_predator.z - _predator.targetZ));
}

static void predateurUpdateTargetDirection(void) {
  GLfloat center[3];
  for (int i = 0; i < _predator.nb_mobiles; i++)
  {
    center[0] += _mobile[i].x;
    center[1] += _mobile[i].y;
    center[2] += _mobile[i].z;
  }
  _predator.targetX = center[0] / _predator.nb_mobiles;
  _predator.targetY = center[1] / _predator.nb_mobiles;
  _predator.targetZ = center[2] / _predator.nb_mobiles;
  
  // _predator.targetX = gl4dmSURand() * _width - _predator.r;
  // _predator.targetY = gl4dmURand() * (HAUTEUR_SEUIL - (2 * _predator.r)) + _predator.r;
  // _predator.targetZ = gl4dmSURand() * _depth - _predator.r;
}

static double get_dt(void) {
  static double t0 = 0, t, dt;
  t = gl4dGetElapsedTime();
  dt = (t - t0) / 1000.0;
  t0 = t;
  return dt;
}