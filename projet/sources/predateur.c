#include <GL4D/gl4du.h>
#include <GL4D/gl4dg.h>
#include <stdlib.h>
#include <assert.h>
#include "../headers/mobile.h"
#include "../headers/predateur.h"

predator_t _predator;

static void predateurUpdateTargetDirection(void);
static GLfloat predateurDistance(void);
static void frottements(GLfloat kx, GLfloat ky, GLfloat kz);
static void predateurBoxCollider(void);
static double get_dt(void);

static void normalize(GLfloat *v);
static GLfloat dotProduct(GLfloat *a, GLfloat *b);
static void crossProduct(GLfloat *a, GLfloat *b, GLfloat *result);
static void orientPredator();

static GLfloat _width = 1, _depth = 1;

void predatorInit(int nb_mobiles, GLfloat width, GLfloat height, GLfloat depth) {
  _width = width; _depth = depth;
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
  _predator.y_direction_inversee = GL_FALSE;
  predateurUpdateTargetDirection();
}

void predatorFree(void) {
  _predator.r = 0.0f;
  _predator.x = 0.0f;
  _predator.y = 0.0f;
  _predator.z = 0.0f;
  _predator.vx = 0.0f;
  _predator.vy = 0.0f;
  _predator.vz = 0.0f;
  _predator.color[0] = 0.0f;
  _predator.color[1] = 0.0f;
  _predator.color[2] = 0.0f;
  _predator.color[3] = 0.0f;
  _predator.nb_mobiles = 0;
  _predator.y_direction_inversee = GL_FALSE;
}

void predatorMove(GLfloat width, GLfloat height, GLfloat depth) {
  double dt = get_dt();
  
  predateurUpdateTargetDirection();
  
  if (fabs(predateurDistance()) >= _predator.r * 10){
    _predator.vx += (_predator.targetX - _predator.x) * TARGET_WEIGHT_PREDATOR;
    _predator.vy += (_predator.targetY - _predator.y) * TARGET_WEIGHT_PREDATOR;
    _predator.vz += (_predator.targetZ - _predator.z) * TARGET_WEIGHT_PREDATOR;

    _predator.vx *= DAMPING;
    _predator.vy *= DAMPING;
    _predator.vz *= DAMPING;

    if (_predator.vx > VELOCITY_LIMIT_PREDATOR) _predator.vx = VELOCITY_LIMIT_PREDATOR;
    if (_predator.vx < -VELOCITY_LIMIT_PREDATOR) _predator.vx = -VELOCITY_LIMIT_PREDATOR;
    if (_predator.vy > VELOCITY_LIMIT_PREDATOR) _predator.vy = VELOCITY_LIMIT_PREDATOR;
    if (_predator.vy < -VELOCITY_LIMIT_PREDATOR) _predator.vy = -VELOCITY_LIMIT_PREDATOR;
    if (_predator.vz > VELOCITY_LIMIT_PREDATOR) _predator.vz = VELOCITY_LIMIT_PREDATOR;
    if (_predator.vz < -VELOCITY_LIMIT_PREDATOR) _predator.vz = -VELOCITY_LIMIT_PREDATOR;
  }

  _predator.x += _predator.vx * dt;
  _predator.y += _predator.vy * dt;
  _predator.z += _predator.vz * dt;

  predateurBoxCollider();
}

void predatorDraw(GLuint obj) {
  GLint pId;
  glGetIntegerv(GL_CURRENT_PROGRAM, &pId);
  gl4duPushMatrix();
  orientPredator();
  gl4duScalef(0.5f, 0.5f, 0.5f);
  gl4duSendMatrices();
  glUniform4fv(glGetUniformLocation(pId, "couleur"), 1, _predator.color);
  assimpDrawScene(obj);
}


static void frottements(GLfloat kx, GLfloat ky, GLfloat kz) {
  GLfloat vx = fabs(_predator.vx), vy = fabs(_predator.vy), vz = fabs(_predator.vz);

  if(vx < EPSILON)  _predator.vx = 0;
  else              _predator.vx = (vx - kx * vx) * SIGN(_predator.vx);
  
  if(vy < EPSILON)  _predator.vy = 0;
  else              _predator.vy = (vy - ky * vy) * SIGN(_predator.vy);
  
  if(vz < EPSILON)  _predator.vz = 0;
  else              _predator.vz = (vz - kz * vz) * SIGN(_predator.vz);
}

static void predateurBoxCollider(void) {
  double d;
  if( (d = _predator.x - _predator.r + _width) <= EPSILON || 
      (d = _predator.x + _predator.r - _width) >= -EPSILON ) {
    if(d * _predator.vx > 0) _predator.vx = -_predator.vx;
    _predator.x -= d - EPSILON;
    frottements(0.1f, 0.0f, 0.1f);
  }

  if( (d = _predator.z - _predator.r + _depth) <= EPSILON || 
      (d = _predator.z + _predator.r - _depth) >= -EPSILON ) {
    if(d * _predator.vz > 0) _predator.vz = -_predator.vz;
    _predator.z -= d - EPSILON;
    frottements(0.1f, 0.0f, 0.1f);
  }

  if( (d = _predator.y - _predator.r) <= EPSILON ) {
    if(_predator.vy < 0) _predator.vy = -_predator.vy;
    _predator.y -= d - EPSILON;
    _predator.y_direction_inversee = GL_FALSE;
    _predator.vy *= DAMPING;
    frottements(0.1f, 0.0f, 0.1f);
  }

  if( (d = _predator.y + _predator.r - HAUTEUR_SEUIL) >= -EPSILON ) {
    if(_predator.vy > 0) _predator.vy = -_predator.vy;
    _predator.y -= d - EPSILON;
    _predator.y_direction_inversee = GL_FALSE;
    _predator.vy *= DAMPING;
    frottements(0.1f, 0.0f, 0.1f);
  }
}

static GLfloat predateurDistance(void) {
  return sqrt((_predator.x - _predator.targetX) * (_predator.x - _predator.targetX) + 
              (_predator.y - _predator.targetY) * (_predator.y - _predator.targetY) + 
              (_predator.z - _predator.targetZ) * (_predator.z - _predator.targetZ));
}

static void predateurUpdateTargetDirection(void) {
  GLfloat center[3] = {0.0f, 0.0f, 0.0f};
  int count = 0;
  for (int i = 0; i < _predator.nb_mobiles; i++) {
    if (!isfinite(_mobile[i].x) || !isfinite(_mobile[i].y) || !isfinite(_mobile[i].z)) continue;
    center[0] += _mobile[i].x;
    center[1] += _mobile[i].y;
    center[2] += _mobile[i].z;
    count++;
  }

  if (count > 0) {
    _predator.targetX = center[0] / count;
    _predator.targetY = center[1] / count;
    _predator.targetZ = center[2] / count;
  } else {
    _predator.targetX = 0;
    _predator.targetY = 0;
    _predator.targetZ = 0;
  }

}

static double get_dt(void) {
  static double t0 = 0, t, dt;
  t = gl4dGetElapsedTime();
  dt = (t - t0) / 1000.0;
  t0 = t;
  return dt;
}

static void normalize(GLfloat *v) {
  GLfloat length = sqrt(v[0] * v[0] + v[1] * v[1] + v[2] * v[2]);
  if (length != 0) {
    v[0] /= length;
    v[1] /= length;
    v[2] /= length;
  }
}
static GLfloat dotProduct(GLfloat *a, GLfloat *b) {
  return a[0] * b[0] + a[1] * b[1] + a[2] * b[2];
}
static void crossProduct(GLfloat *a, GLfloat *b, GLfloat *result) {
  result[0] = a[1] * b[2] - a[2] * b[1];
  result[1] = a[2] * b[0] - a[0] * b[2];
  result[2] = a[0] * b[1] - a[1] * b[0];
}
static void orientPredator() {
  GLfloat direction[] = { _predator.vx, _predator.vy, _predator.vz };
  normalize(direction);

  GLfloat reference[] = { 0.0f, 0.0f, 1.0f };

  GLfloat dot = dotProduct(reference, direction);
  GLfloat angle = acos(dot) * 180.0f / M_PI;

  GLfloat axis[3];
  crossProduct(reference, direction, axis);
  normalize(axis);

  gl4duLoadIdentityf();
  gl4duTranslatef(_predator.x, _predator.y, _predator.z);
  gl4duRotatef(angle, axis[0], axis[1], axis[2]);
}
