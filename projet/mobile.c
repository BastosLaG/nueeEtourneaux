/*!\file mobile.c
 *
 * \brief Bibliothèque de gestion de mobiles
 * \author Farès BELHADJ, amsi@ai.univ-paris8.fr 
 * \date March 10 2017
 */
#include <stdlib.h>
#include <assert.h>
#include <math.h>
#include <GL4D/gl4du.h>
#include <GL4D/gl4dg.h>

#define HAUTEUR_SEUIL 7.0f
#define K_RESSORT 1.5f // Constante de raideur du ressort
#define NUM_NEIGHBORS 6 // Nombre de voisins les plus proches
#define DAMPING 0.99f // Facteur d'amortissement pour stabiliser le mouvement

/*!\typedef structure pour mobile */
typedef struct mobile_t mobile_t;
struct mobile_t {
  GLuint id;

  // set pos
  GLfloat x, y, z, r;
  
  // set velocities
  GLfloat vx, vy, vz;
  
  // set warning predator
  GLboolean enAlerte;
  
  // set color
  GLfloat color[4];
  
  // set bonus setting
  GLboolean freeze;
  GLboolean y_direction_inversee;

};

static mobile_t * _mobile = NULL;
static int _nb_mobiles = 0;
static GLfloat _width = 1, _depth = 1;
// static GLfloat _gravity[3] = {0, -9.8 * 3.0, 0};

static void quit(void);
static void frottements(int i, GLfloat kx, GLfloat ky, GLfloat kz);
static double get_dt(void);

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
    _mobile[i].vx = 1.0f; 
    _mobile[i].vy = 1.0f; 
    _mobile[i].vz = 1.0f;
    _mobile[i].color[0] = gl4dmURand();
    _mobile[i].color[1] = gl4dmURand();
    _mobile[i].color[2] = gl4dmURand();
    _mobile[i].color[3] = 1.0f;
    _mobile[i].freeze = GL_FALSE;
    _mobile[i].y_direction_inversee = GL_FALSE;
  }
}

#define EPSILON 0.00001f

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

void attraction(GLuint id, GLfloat * coords) {
  GLfloat dx = coords[0] - _mobile[id].x;
  GLfloat dy = coords[1] - _mobile[id].y;
  GLfloat dz = coords[2] - _mobile[id].z;
  GLfloat d = sqrt(dx * dx + dy * dy + dz * dz);
  if(d > EPSILON) {
    // Force de rappel du ressort
    GLfloat F = -K_RESSORT * (d - _mobile[id].r); // Distance initiale d'équilibre peut être _mobile[id].r
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
    // Force de rappel du ressort
    GLfloat F = K_RESSORT * (d - _mobile[id].r); // Distance initiale d'équilibre peut être _mobile[id].r
    dx /= d; dy /= d; dz /= d;
    _mobile[id].vx -= F * dx;
    _mobile[id].vy -= F * dy;
    _mobile[id].vz -= F * dz;
  }
}

static GLfloat distance(mobile_t a, mobile_t b) {
  return sqrt((a.x - b.x) * (a.x - b.x) + 
              (a.y - b.y) * (a.y - b.y) + 
              (a.z - b.z) * (a.z - b.z));
}

void applySpringForce(GLuint id, GLuint neighborId) {
  GLfloat dx = _mobile[neighborId].x - _mobile[id].x;
  GLfloat dy = _mobile[neighborId].y - _mobile[id].y;
  GLfloat dz = _mobile[neighborId].z - _mobile[id].z;
  GLfloat d = sqrt(dx * dx + dy * dy + dz * dz);
  if(d > EPSILON) {
    // Force de rappel du ressort
    GLfloat F = K_RESSORT * (d - _mobile[id].r); // Distance initiale d'équilibre peut être _mobile[id].r
    dx /= d; dy /= d; dz /= d;
    _mobile[id].vx += F * dx;
    _mobile[id].vy += F * dy;
    _mobile[id].vz += F * dz;
  }
}

void mobileMove(void) {
  int i, j;
  GLfloat dt = get_dt(), d;
  
  for(i = 0; i < _nb_mobiles; i++) {
    if(_mobile[i].freeze) continue;

    // Trouver les six voisins les plus proches
    struct neighbor {
      int id;
      GLfloat dist;
    } neighbors[NUM_NEIGHBORS];

    for(j = 0; j < NUM_NEIGHBORS; j++) {
      neighbors[j].id = -1;
      neighbors[j].dist = FLT_MAX;
    }

    for(j = 0; j < _nb_mobiles; j++) {
      if(i == j) continue;
      d = distance(_mobile[i], _mobile[j]);
      if(d < neighbors[NUM_NEIGHBORS - 1].dist) {
        neighbors[NUM_NEIGHBORS - 1].id = j;
        neighbors[NUM_NEIGHBORS - 1].dist = d;
        // Trier les voisins par distance
        for(int k = NUM_NEIGHBORS - 1; k > 0 && neighbors[k].dist < neighbors[k - 1].dist; k--) {
          struct neighbor tmp = neighbors[k];
          neighbors[k] = neighbors[k - 1];
          neighbors[k - 1] = tmp;
        }
      }
    }

    // Appliquer les forces des ressorts des voisins les plus proches
    for(j = 0; j < NUM_NEIGHBORS; j++) {
      if(neighbors[j].id != -1) {
        applySpringForce(i, neighbors[j].id);
      }
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

