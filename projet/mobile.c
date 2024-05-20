#include <stdlib.h>
#include <assert.h>
#include <math.h>
#include <GL4D/gl4du.h>
#include <GL4D/gl4dg.h>
#include <stdio.h>
#include "predateur.h"
#include "mobile.h"

static mobile_t * _mobile = NULL;
static int _nb_mobiles = 0;
static GLfloat _width = 1, _depth = 1;

static void quit(void);
static void frottements(int i, GLfloat kx, GLfloat ky, GLfloat kz);
static double get_dt(void);
static void applyBoidsRules(int i);
static void updateTargetDirection(int i);
static void avoidPredator(int i);

void mobileInit(int n, GLfloat width, GLfloat depth) {
  int i;
  _width = width; _depth = depth;
  _nb_mobiles = n;
  if(_mobile) {
    free(_mobile);
    _mobile = NULL;
  } else
    atexit(quit);
  _mobile = malloc(_nb_mobiles * sizeof *_mobile);
  assert(_mobile);
  for(i = 0; i < _nb_mobiles; i++) {
    _mobile[i].r = 0.1f;
    _mobile[i].x = gl4dmSURand() * _width - _mobile[i].r;
    _mobile[i].z = gl4dmSURand() * _depth - _mobile[i].r;
    _mobile[i].y = _depth;
    _mobile[i].vx = 3.0f * (gl4dmSURand() - 0.5f);
    _mobile[i].vy = 3.0f * (gl4dmSURand() - 0.5f);
    _mobile[i].vz = 3.0f * (gl4dmSURand() - 0.5f);
    _mobile[i].color[0] = gl4dmURand();
    _mobile[i].color[1] = gl4dmURand();
    _mobile[i].color[2] = gl4dmURand();
    _mobile[i].color[3] = 1.0f;
    _mobile[i].freeze = GL_FALSE;
    _mobile[i].y_direction_inversee = GL_FALSE;
    updateTargetDirection(i); // Initialiser la direction cible
  }
  predatorInit(_width, HAUTEUR_SEUIL, _depth); // Initialiser le prédateur
}

void mobileSetFreeze(GLuint id, GLboolean freeze) {
  _mobile[id].freeze = freeze;
}

void mobileGetCoords(GLuint id, GLfloat * coords) {
  coords[0] = _mobile[id].x;
  coords[1] = _mobile[id].y;
  coords[2] = _mobile[id].z;
}

void mobileSetCoords(GLuint id, GLfloat * coords) {
  _mobile[id].x = coords[0];
  _mobile[id].y = coords[1];
  _mobile[id].z = coords[2];
}

void mobileMove(void) {
  int i;
  GLfloat dt = get_dt(), d;
  
  predatorMove(_plan_s, HAUTEUR_SEUIL, _plan_s); // Déplacer le prédateur

  for(i = 0; i < _nb_mobiles; i++) {
    if(_mobile[i].freeze) continue;

    // Mettre à jour la direction cible périodiquement
    if(rand() % 100 < 5) { // Par exemple, 5% de chance de mettre à jour la direction à chaque itération
      updateTargetDirection(i);
    }

    // Appliquer les règles de Boids pour chaque mobile
    applyBoidsRules(i);

    // Éviter le prédateur
    avoidPredator(i);

    // Appliquer la direction cible
    _mobile[i].vx += (_mobile[i].targetX - _mobile[i].x) * TARGET_WEIGHT;
    _mobile[i].vy += (_mobile[i].targetY - _mobile[i].y) * TARGET_WEIGHT;
    _mobile[i].vz += (_mobile[i].targetZ - _mobile[i].z) * TARGET_WEIGHT;

    // Appliquer une force de levée pour éviter de rester bloqué en bas
    if (_mobile[i].y < _mobile[i].r * 2) {
      _mobile[i].vy += 0.5f; // Augmenter la force de levée
    }

    // Ajuster la vitesse pour être constante
    GLfloat speed = sqrt(_mobile[i].vx * _mobile[i].vx + _mobile[i].vy * _mobile[i].vy + _mobile[i].vz * _mobile[i].vz);
    if (speed > EPSILON) {
      _mobile[i].vx = (_mobile[i].vx / speed) * TARGET_VELOCITY;
      _mobile[i].vy = (_mobile[i].vy / speed) * TARGET_VELOCITY;
      _mobile[i].vz = (_mobile[i].vz / speed) * TARGET_VELOCITY;
    }

    // Appliquer un facteur d'amortissement léger pour éviter l'augmentation exponentielle de la vitesse
    _mobile[i].vx *= DAMPING;
    _mobile[i].vy *= DAMPING;
    _mobile[i].vz *= DAMPING;

    // Appliquer les vitesses à la position du mobile
    _mobile[i].x += _mobile[i].vx * dt;
    _mobile[i].y += _mobile[i].vy * dt;
    _mobile[i].z += _mobile[i].vz * dt;

    // Gérer les collisions avec les bords de la boîte
    if( (d = _mobile[i].x - _mobile[i].r + _width) <= EPSILON || 
        (d = _mobile[i].x + _mobile[i].r - _width) >= -EPSILON ) {
      if(d * _mobile[i].vx > 0) _mobile[i].vx = -_mobile[i].vx;
      _mobile[i].x -= d - EPSILON;
      frottements(i, 0.1f, 0.0f, 0.1f);
    }

    if( (d = _mobile[i].z - _mobile[i].r + _depth) <= EPSILON || 
        (d = _mobile[i].z + _mobile[i].r - _depth) >= -EPSILON ) {
      if(d * _mobile[i].vz > 0) _mobile[i].vz = -_mobile[i].vz;
      _mobile[i].z -= d - EPSILON;
      frottements(i, 0.1f, 0.0f, 0.1f);
    }

    // Si le bord de la sphère touche le bas de la boîte
    if( (d = _mobile[i].y - _mobile[i].r) <= EPSILON ) {
      if(_mobile[i].vy < 0) _mobile[i].vy = -_mobile[i].vy;
      _mobile[i].y -= d - EPSILON;
      _mobile[i].y_direction_inversee = GL_FALSE;
      // Appliquer un amortissement supplémentaire sur l'axe y
      _mobile[i].vy *= DAMPING;
      frottements(i, 0.1f, 0.0f, 0.1f);
    }

    // Si le bord de la sphère touche le haut de la boîte
    if( (d = _mobile[i].y + _mobile[i].r - HAUTEUR_SEUIL) >= -EPSILON ) {
      if(_mobile[i].vy > 0) _mobile[i].vy = -_mobile[i].vy;
      _mobile[i].y -= d - EPSILON;
      _mobile[i].y_direction_inversee = GL_FALSE;
      // Appliquer un amortissement supplémentaire sur l'axe y
      _mobile[i].vy *= DAMPING;
      frottements(i, 0.1f, 0.0f, 0.1f);
    }
  }
}

void mobileDraw(GLuint obj) {
  int i;
  GLint pId;
  glGetIntegerv(GL_CURRENT_PROGRAM, &pId);
  for(i = 0; i < _nb_mobiles; i++) {
    gl4duPushMatrix();
    gl4duTranslatef(_mobile[i].x, _mobile[i].y, _mobile[i].z);
    gl4duScalef(_mobile[i].r, _mobile[i].r, _mobile[i].r);
    gl4duSendMatrices();
    gl4duPopMatrix();
    glUniform1i(glGetUniformLocation(pId, "id"), i + 3);
    glUniform4fv(glGetUniformLocation(pId, "couleur"), 1, _mobile[i].color);
    gl4dgDraw(obj);
  }
}

void attraction(GLuint id, GLfloat * coords) {
  GLfloat dx = coords[0] - _mobile[id].x;
  GLfloat dy = coords[1] - _mobile[id].y;
  GLfloat dz = coords[2] - _mobile[id].z;
  GLfloat d = sqrt(dx * dx + dy * dy + dz * dz);
  if(d > EPSILON) {
    GLfloat F = -K_RESSORT * (d - _mobile[id].r);
    dx /= d; dy /= d; dz /= d;
    _mobile[id].vx += F * dx;
    _mobile[id].vy += F * dy;
    _mobile[id].vz += F * dz;
  }
}

void repulsion(GLuint id, GLfloat * coords) {
  GLfloat dx = coords[0] - _mobile[id].x;
  GLfloat dy = coords[1] - _mobile[id].y;
  GLfloat dz = coords[2] - _mobile[id].z;
  GLfloat d = sqrt(dx * dx + dy * dy + dz * dz);
  if(d > EPSILON) {
    GLfloat F = K_RESSORT * REPULSION_MULTIPLIER * (d - _mobile[id].r); 
    dx /= d; dy /= d; dz /= d;
    _mobile[id].vx -= F * dx;
    _mobile[id].vy -= F * dy;
    _mobile[id].vz -= F * dz;
  }
}

void applySpringForce(GLuint id, GLuint neighborId) {
  GLfloat dx = _mobile[neighborId].x - _mobile[id].x;
  GLfloat dy = _mobile[neighborId].y - _mobile[id].y;
  GLfloat dz = _mobile[neighborId].z - _mobile[id].z;
  GLfloat d = sqrt(dx * dx + dy * dy + dz * dz);
  if(d > EPSILON) {
    GLfloat F = K_RESSORT * (d - _mobile[id].r); // Force de rappel proportionnelle
    if (F > MAX_FORCE) F = MAX_FORCE; // Limiter la force de rappel
    dx /= d; dy /= d; dz /= d;
    _mobile[id].vx += F * dx;
    _mobile[id].vy += F * dy;
    _mobile[id].vz += F * dz;
  }
}

static GLfloat distance(mobile_t a, mobile_t b) {
  return sqrt((a.x - b.x) * (a.x - b.x) + 
              (a.y - b.y) * (a.y - b.y) + 
              (a.z - b.z) * (a.z - b.z));
}

static void frottements(int i, GLfloat kx, GLfloat ky, GLfloat kz) {
  GLfloat vx = fabs(_mobile[i].vx), vy = fabs(_mobile[i].vy), vz = fabs(_mobile[i].vz);

  if(vx < EPSILON)  _mobile[i].vx = 0;
  else              _mobile[i].vx = (vx - kx * vx) * SIGN(_mobile[i].vx);
  
  if(vy < EPSILON)  _mobile[i].vy = 0;
  else              _mobile[i].vy = (vy - ky * vy) * SIGN(_mobile[i].vy);
  
  if(vz < EPSILON)  _mobile[i].vz = 0;
  else              _mobile[i].vz = (vz - kz * vz) * SIGN(_mobile[i].vz);
}

static void quit(void) {
  _nb_mobiles = 0;
  if(_mobile) {
    free(_mobile);
    _mobile = NULL;
  }
}

static double get_dt(void) {
  static double t0 = 0, t, dt;
  t = gl4dGetElapsedTime();
  dt = (t - t0) / 1000.0;
  t0 = t;
  return dt;
}

// Fonction pour appliquer les règles de Boids
static void applyBoidsRules(int i) {
  int j, count = 0;
  GLfloat avgVx = 0, avgVy = 0, avgVz = 0;
  GLfloat centerX = 0, centerY = 0, centerZ = 0;
  GLfloat separationX = 0, separationY = 0, separationZ = 0;
  GLfloat d;
  GLfloat influenceDistance = _mobile[i].r * 20; // Augmenter la distance d'influence

  for(j = 0; j < _nb_mobiles; j++) {
    if(i == j) continue;
    d = distance(_mobile[i], _mobile[j]);
    if(d < influenceDistance) { // Utiliser la nouvelle distance d'influence
      // Règle de cohésion
      centerX += _mobile[j].x;
      centerY += _mobile[j].y;
      centerZ += _mobile[j].z;

      // Règle d'alignement
      avgVx += _mobile[j].vx;
      avgVy += _mobile[j].vy;
      avgVz += _mobile[j].vz;

      // Règle de séparation
      if(d < _mobile[i].r * 3) { // Augmenter la distance minimale acceptable pour la séparation
        separationX += _mobile[i].x - _mobile[j].x;
        separationY += _mobile[i].y - _mobile[j].y;
        separationZ += _mobile[i].z - _mobile[j].z;
      }

      count++;
    }
  }

  if(count > 0) {
    // Calculer la cohésion (diriger vers le centre des voisins)
    centerX /= count;
    centerY /= count;
    centerZ /= count;
    _mobile[i].vx += (centerX - _mobile[i].x) * (COHESION_WEIGHT / 2); // Réduire la force de cohésion
    _mobile[i].vy += (centerY - _mobile[i].y) * (COHESION_WEIGHT / 2);
    _mobile[i].vz += (centerZ - _mobile[i].z) * (COHESION_WEIGHT / 2);

    // Calculer l'alignement (diriger vers la vitesse moyenne des voisins)
    avgVx /= count;
    avgVy /= count;
    avgVz /= count;
    _mobile[i].vx += (avgVx - _mobile[i].vx) * ALIGNMENT_WEIGHT;
    _mobile[i].vy += (avgVy - _mobile[i].vy) * ALIGNMENT_WEIGHT;
    _mobile[i].vz += (avgVz - _mobile[i].vz) * ALIGNMENT_WEIGHT;

    // Appliquer la séparation (éviter les collisions)
    _mobile[i].vx += separationX * (SEPARATION_WEIGHT * 2); // Augmenter la force de séparation
    _mobile[i].vy += separationY * (SEPARATION_WEIGHT * 2);
    _mobile[i].vz += separationZ * (SEPARATION_WEIGHT * 2);
  }

  // Éviter les murs
  if(_mobile[i].x < -_width + _mobile[i].r) {
    _mobile[i].vx += AVOIDANCE_WEIGHT;
  } else if(_mobile[i].x > _width - _mobile[i].r) {
    _mobile[i].vx -= AVOIDANCE_WEIGHT;
  }

  if(_mobile[i].y < _mobile[i].r) {
    _mobile[i].vy += AVOIDANCE_WEIGHT;
  } else if(_mobile[i].y > HAUTEUR_SEUIL - _mobile[i].r) {
    _mobile[i].vy -= AVOIDANCE_WEIGHT;
  }

  if(_mobile[i].z < -_depth + _mobile[i].r) {
    _mobile[i].vz += AVOIDANCE_WEIGHT;
  } else if(_mobile[i].z > _depth - _mobile[i].r) {
    _mobile[i].vz -= AVOIDANCE_WEIGHT;
  }
}

static void avoidPredator(int i) {
  GLfloat dx = _predator.x - _mobile[i].x;
  GLfloat dy = _predator.y - _mobile[i].y;
  GLfloat dz = _predator.z - _mobile[i].z;
  GLfloat d = sqrt(dx * dx + dy * dy + dz * dz);
  if(d < _mobile[i].r * 10) { // Si le prédateur est à moins de 10 fois le rayon du mobile
    _mobile[i].vx -= dx / d * PREDATOR_AVOIDANCE_WEIGHT;
    _mobile[i].vy -= dy / d * PREDATOR_AVOIDANCE_WEIGHT;
    _mobile[i].vz -= dz / d * PREDATOR_AVOIDANCE_WEIGHT;
  }
}

// Fonction pour mettre à jour la direction cible aléatoire
static void updateTargetDirection(int i) {
  _mobile[i].targetX = gl4dmSURand() * _width - _mobile[i].r;
  _mobile[i].targetY = gl4dmSURand() * (HAUTEUR_SEUIL - 2 * _mobile[i].r) + _mobile[i].r; // Assurez-vous que la direction cible reste dans la zone de vol
  _mobile[i].targetZ = gl4dmSURand() * _depth - _mobile[i].r;
}
