#include <GL4D/gl4du.h>
#include <GL4D/gl4dg.h>
#include <stdlib.h>
#include <assert.h>
#include "../headers/mobile.h"
#include "../headers/predateur.h"

// Structure du prédateur pour stocker ses propriétés
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

// Initialiser le prédateur avec des valeurs initiales et définir la direction de la cible
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

// Libérer les ressources du prédateur
void predatorFree(void) {
  // Réinitialiser les propriétés du prédateur
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

// Mettre à jour la position du prédateur en fonction de sa vitesse et de la direction de la cible
void predatorMove(GLfloat width, GLfloat height, GLfloat depth) {
  double dt = get_dt();
  
  predateurUpdateTargetDirection();
  
  if (fabs(predateurDistance()) >= _predator.r * 10){
    // Mettre à jour la vitesse vers la direction de la cible
    _predator.vx += (_predator.targetX - _predator.x) * TARGET_WEIGHT_PREDATOR;
    _predator.vy += (_predator.targetY - _predator.y) * TARGET_WEIGHT_PREDATOR;
    _predator.vz += (_predator.targetZ - _predator.z) * TARGET_WEIGHT_PREDATOR;

    // Appliquer un amortissement pour éviter l'augmentation exponentielle de la vitesse
    _predator.vx *= DAMPING;
    _predator.vy *= DAMPING;
    _predator.vz *= DAMPING;

    // Limiter la vitesse à la valeur maximale
    if (_predator.vx > VELOCITY_LIMIT_PREDATOR) _predator.vx = VELOCITY_LIMIT_PREDATOR;
    if (_predator.vx < -VELOCITY_LIMIT_PREDATOR) _predator.vx = -VELOCITY_LIMIT_PREDATOR;
    if (_predator.vy > VELOCITY_LIMIT_PREDATOR) _predator.vy = VELOCITY_LIMIT_PREDATOR;
    if (_predator.vy < -VELOCITY_LIMIT_PREDATOR) _predator.vy = -VELOCITY_LIMIT_PREDATOR;
    if (_predator.vz > VELOCITY_LIMIT_PREDATOR) _predator.vz = VELOCITY_LIMIT_PREDATOR;
    if (_predator.vz < -VELOCITY_LIMIT_PREDATOR) _predator.vz = -VELOCITY_LIMIT_PREDATOR;
  }

  // Mettre à jour la position du prédateur en fonction de la vitesse limitée
  _predator.x += _predator.vx * dt;
  _predator.y += _predator.vy * dt;
  _predator.z += _predator.vz * dt;

  predateurBoxCollider();
}

// Dessiner le prédateur en utilisant les fonctions OpenGL
void predatorDraw(GLuint obj) {
  GLint pId;
  glGetIntegerv(GL_CURRENT_PROGRAM, &pId);
  gl4duPushMatrix();
  orientPredator();
  gl4duScalef(0.5f, 0.5f, 0.5f);
  gl4duSendMatrices();
  // gl4duPopMatrix();
  glUniform4fv(glGetUniformLocation(pId, "couleur"), 1, _predator.color);
  // gl4dgDraw(obj);
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

// Vérifier les collisions avec les limites et refléter la vitesse si nécessaire
static void predateurBoxCollider(void) {
  double d;
  // Gérer les collisions avec les bords de la boîte
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

  // Si le bord de la sphère touche le bas de la boîte
  if( (d = _predator.y - _predator.r) <= EPSILON ) {
    if(_predator.vy < 0) _predator.vy = -_predator.vy;
    _predator.y -= d - EPSILON;
    _predator.y_direction_inversee = GL_FALSE;
    // Appliquer un amortissement supplémentaire sur l'axe y
    _predator.vy *= DAMPING;
    frottements(0.1f, 0.0f, 0.1f);
  }

  // Si le bord de la sphère touche le haut de la boîte
  if( (d = _predator.y + _predator.r - HAUTEUR_SEUIL) >= -EPSILON ) {
    if(_predator.vy > 0) _predator.vy = -_predator.vy;
    _predator.y -= d - EPSILON;
    _predator.y_direction_inversee = GL_FALSE;
    // Appliquer un amortissement supplémentaire sur l'axe y
    _predator.vy *= DAMPING;
    frottements(0.1f, 0.0f, 0.1f);
  }
}

// Calculer la distance entre le prédateur et sa cible
static GLfloat predateurDistance(void) {
  return sqrt((_predator.x - _predator.targetX) * (_predator.x - _predator.targetX) + 
              (_predator.y - _predator.targetY) * (_predator.y - _predator.targetY) + 
              (_predator.z - _predator.targetZ) * (_predator.z - _predator.targetZ));
}

// Mettre à jour la direction de la cible vers la position moyenne des mobiles
static void predateurUpdateTargetDirection(void) {
  GLfloat center[3] = {0.0f, 0.0f, 0.0f};
  for (int i = 0; i < _predator.nb_mobiles; i++) {
    center[0] += _mobile[i].x;
    center[1] += _mobile[i].y;
    center[2] += _mobile[i].z;
  }
  _predator.targetX = center[0] / _predator.nb_mobiles;
  _predator.targetY = center[1] / _predator.nb_mobiles;
  _predator.targetZ = center[2] / _predator.nb_mobiles;
}

// Calculer la différence de temps entre les cadres
static double get_dt(void) {
  static double t0 = 0, t, dt;
  t = gl4dGetElapsedTime();
  dt = (t - t0) / 1000.0;
  t0 = t;
  return dt;
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
static void orientPredator() {
  GLfloat direction[] = { _predator.vx, _predator.vy, _predator.vz };
  normalize(direction);

  // Le vecteur de référence initial (ex : axe Z)
  GLfloat reference[] = { 0.0f, 0.0f, 1.0f };

  // Calcul de l'angle entre la direction et le vecteur de référence
  GLfloat dot = dotProduct(reference, direction);
  GLfloat angle = acos(dot) * 180.0f / M_PI;

  // Calcul de l'axe de rotation (produit vectoriel entre la référence et la direction)
  GLfloat axis[3];
  crossProduct(reference, direction, axis);
  normalize(axis);

  // Appliquer la rotation au mobile
  gl4duLoadIdentityf();
  gl4duTranslatef(_predator.x, _predator.y, _predator.z);
  gl4duRotatef(angle, axis[0], axis[1], axis[2]);
}
