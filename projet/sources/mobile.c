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

mobile_t * _mobile = NULL;
spring_t * _springs = NULL;
OctreeNode_t * _root = NULL;

static int _nb_mobiles = 0;
static int _nb_springs = 0;
static GLfloat _width = 1, _depth = 1;

static void quit(void);
static void frottements(int i, GLfloat kx, GLfloat ky, GLfloat kz);
static void updateTargetDirection(int i);
static void avoidPredator(int i);
static void update_move(int i, float dt);
static void collision_collback(int i, float d);
static void applySpringForces(void);
static double get_dt(void);

static void normalize(GLfloat *v);
static GLfloat dotProduct(GLfloat *a, GLfloat *b);
static void crossProduct(GLfloat *a, GLfloat *b, GLfloat *result);
static void orientMobile(int i);

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
    _mobile[i].id = i;
    _mobile[i].r = 0.2f;
    _mobile[i].x = gl4dmSURand() * _width - _mobile[i].r;
    _mobile[i].z = gl4dmSURand() * _depth - _mobile[i].r;
    _mobile[i].y = _depth;
    _mobile[i].vx = 3.0f * (gl4dmSURand() - 0.5f);
    _mobile[i].vy = 3.0f * (gl4dmURand() - 0.5f);
    _mobile[i].vz = 3.0f * (gl4dmSURand() - 0.5f);
    _mobile[i].color[0] = gl4dmURand();
    _mobile[i].color[1] = gl4dmURand();
    _mobile[i].color[2] = gl4dmURand();
    _mobile[i].color[3] = 1.0f;
    _mobile[i].freeze = GL_FALSE;
    _mobile[i].y_direction_inversee = GL_FALSE;
    updateTargetDirection(i);
  }
  springInit(_nb_mobiles);
  predatorInit(_nb_mobiles, _width, HAUTEUR_SEUIL, _depth);
}

int compareDistances(const void *a, const void *b) {
  struct Distance *distA = (struct Distance *)a;
  struct Distance *distB = (struct Distance *)b;
  if (distA->dist < distB->dist) return -1;
  if (distA->dist > distB->dist) return 1;
  return 0;
}

void springInit(int n) {
  int i, j, k;
  _nb_springs = n * 6;
  if (_springs) {
    free(_springs);
    _springs = NULL;
  }
  _springs = malloc(_nb_springs * sizeof * _springs);
  assert(_springs);

  _root = createOctreeNode(_width/2, _depth/2, _depth/2, fmaxf(_width, _depth));

  for (i = 0; i < _nb_mobiles; i++) {
    insertMobile(_root, i, 0);
  }

  k = 0;
  for (i = 0; i < _nb_mobiles; i++) {
    int closest_neighbors[6];
    findClosestNeighbors(i, closest_neighbors);

    for (j = 0; j < 6; j++) {
      if (closest_neighbors[j] != -1) {
        _springs[k].a = i;
        _springs[k].b = closest_neighbors[j];
        _springs[k].rest_length = 3.0f;
        k++;
      }
    }
  }

  freeOctreeNode(_root);
}



OctreeNode_t* createOctreeNode(GLfloat x, GLfloat y, GLfloat z, GLfloat size) {
  OctreeNode_t *node = (OctreeNode_t*)malloc(sizeof(OctreeNode_t));
  node->x = x;
  node->y = y;
  node->z = z;
  node->size = size;
  node->is_leaf = 1;
  node->mobiles = (int*)malloc(MAX_OBJECTS * sizeof(int));
  node->num_mobiles = 0;
  for (int i = 0; i < 8; i++) {
    node->children[i] = NULL;
  }
  return node;
}

void insertMobile(OctreeNode_t *node, int mobile_index, int depth) {
  if (node->is_leaf) {
    if (node->num_mobiles < MAX_OBJECTS || depth == MAX_DEPTH) {
      node->mobiles[node->num_mobiles++] = mobile_index;
    } else {
      node->is_leaf = 0;
      for (int i = 0; i < 8; i++) {
        GLfloat newSize = node->size / 2;
        node->children[i] = createOctreeNode(
          node->x + (i & 1 ? newSize / 2 : -newSize / 2),
          node->y + (i & 2 ? newSize / 2 : -newSize / 2),
          node->z + (i & 4 ? newSize / 2 : -newSize / 2),
          newSize
        );
      }

      for (int i = 0; i < node->num_mobiles; i++) {
        int idx = node->mobiles[i];
        insertMobile(node, idx, depth + 1);
      }
      node->num_mobiles = 0;
      insertMobile(node, mobile_index, depth + 1);
    }
  } else {
    for (int i = 0; i < 8; i++) {
      GLfloat halfSize = node->size / 2;
      if (fabsf(_mobile[mobile_index].x - node->children[i]->x) <= halfSize &&
          fabsf(_mobile[mobile_index].y - node->children[i]->y) <= halfSize &&
          fabsf(_mobile[mobile_index].z - node->children[i]->z) <= halfSize) {
        insertMobile(node->children[i], mobile_index, depth + 1);
        break;
      }
    }
  }
}

void freeOctreeNode(OctreeNode_t *node) {
  if (!node->is_leaf) {
    for (int i = 0; i < 8; i++) {
      freeOctreeNode(node->children[i]);
    }
  }
  free(node->mobiles);
  free(node);
}

void findClosestNeighbors(int mobile_index, int * closest_neighbors) {
  GLfloat minDist[6];
  for (int i = 0; i < 6; i++) {
    minDist[i] = FLT_MAX;
    closest_neighbors[i] = -1;
  }
  void searchOctree(OctreeNode_t *node) {
    if (node == NULL) return;

    GLfloat halfSize = node->size / 2;
    if (fabsf(_mobile[mobile_index].x - node->x) > halfSize ||
        fabsf(_mobile[mobile_index].y - node->y) > halfSize ||
        fabsf(_mobile[mobile_index].z - node->z) > halfSize) {
      return;
    }

    if (node->is_leaf) {
      for (int i = 0; i < node->num_mobiles; i++) {
        int idx = node->mobiles[i];
        if (idx == mobile_index) continue;
        GLfloat dx = _mobile[mobile_index].x - _mobile[idx].x;
        GLfloat dy = _mobile[mobile_index].y - _mobile[idx].y;
        GLfloat dz = _mobile[mobile_index].z - _mobile[idx].z;
        GLfloat dist = sqrtf(dx * dx + dy * dy + dz * dz);

        for (int j = 0; j < 6; j++) {
          if (dist < minDist[j]) {
            for (int k = 5; k > j; k--) {
              minDist[k] = minDist[k - 1];
              closest_neighbors[k] = closest_neighbors[k - 1];
            }
            minDist[j] = dist;
            closest_neighbors[j] = idx;
            break;
          }
        }
      }
    } else {
      for (int i = 0; i < 8; i++) {
        searchOctree(node->children[i]);
      }
    }
  }
  searchOctree(_root);
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

GLboolean useBoids = GL_FALSE;

void mobileMove(void) {
  int i;
  double dt = get_dt(), d;

  predatorMove(_plan_s, HAUTEUR_SEUIL, _plan_s);

  if (useBoids) {
    for(i = 0; i < _nb_mobiles; i++) {
      if(_mobile[i].freeze) continue;
      applyBoidsRules(i);
      updateTargetDirection(i);
      avoidPredator(i);
      update_move(i, dt);
      collision_collback(i, d);
    }
  } else {
    applySpringForces();
    for(i = 0; i < _nb_mobiles; i++) {
      if(_mobile[i].freeze) continue;
      updateTargetDirection(i);
      avoidPredator(i);
      update_move(i, dt);
      collision_collback(i, d);
    }
  }
}

static void applySpringForces(void) {
  GLfloat centerX = _width / 2.0f;
  GLfloat centerY = _depth / 2.0f;
  GLfloat centerZ = _depth / 2.0f;

  for (int i = 0; i < _nb_springs; i++) {
    GLuint a = _springs[i].a;
    GLuint b = _springs[i].b;

    if (a < 0 || a >= _nb_mobiles || b < 0 || b >= _nb_mobiles) {
        continue;
    }

    GLfloat dx = _mobile[b].x - _mobile[a].x;
    GLfloat dy = _mobile[b].y - _mobile[a].y;
    GLfloat dz = _mobile[b].z - _mobile[a].z;
    GLfloat distance = sqrtf(dx * dx + dy * dy + dz * dz);

    GLfloat distA = sqrtf(
        powf(_mobile[a].x - centerX, 2) + 
        powf(_mobile[a].y - centerY, 2) + 
        powf(_mobile[a].z - centerZ, 2)
    );
    GLfloat distB = sqrtf(
        powf(_mobile[b].x - centerX, 2) + 
        powf(_mobile[b].y - centerY, 2) + 
        powf(_mobile[b].z - centerZ, 2)
    );
    GLfloat factorA = 1.0f + (distA / 5.0f);
    GLfloat factorB = 1.0f + (distB / 5.0f);

    GLfloat forceA = K_RESSORT * factorA * (distance - _springs[i].rest_length);
    GLfloat forceB = K_RESSORT * factorB * (distance - _springs[i].rest_length);

    if (forceA > MAX_FORCE) forceA = MAX_FORCE;
    if (forceA < -MAX_FORCE) forceA = -MAX_FORCE;
    if (forceB > MAX_FORCE) forceB = MAX_FORCE;
    if (forceB < -MAX_FORCE) forceB = -MAX_FORCE;

    GLfloat fxA = (dx / distance) * forceA;
    GLfloat fyA = (dy / distance) * forceA;
    GLfloat fzA = (dz / distance) * forceA;
    GLfloat fxB = (dx / distance) * forceB;
    GLfloat fyB = (dy / distance) * forceB;
    GLfloat fzB = (dz / distance) * forceB;

    _mobile[a].vx += fxA;
    _mobile[a].vy += fyA;
    _mobile[a].vz += fzA;
    _mobile[b].vx -= fxB;
    _mobile[b].vy -= fyB;
    _mobile[b].vz -= fzB;
  }
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
  if(_springs) {
    free(_springs);
    _springs = NULL;
  }
}

static double get_dt(void) {
  static double t0 = 0, t, dt;
  t = gl4dGetElapsedTime();
  dt = (t - t0) / 1000.0;
  t0 = t;
  return dt;
}

void applyBoidsRules(int i) {
  int j, count = 0;
  GLfloat avgVx = 0, avgVy = 0, avgVz = 0;
  GLfloat centerX = 0, centerY = 0, centerZ = 0;
  GLfloat separationX = 0, separationY = 0, separationZ = 0;
  GLfloat d;
  GLfloat influenceDistance = _mobile[i].r * 10;

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
    if (count == NUM_NEIGHBORS) continue;
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

static void updateTargetDirection(int i) {
  _mobile[i].targetX = gl4dmSURand() * _width - _mobile[i].r;
  _mobile[i].targetY = gl4dmURand() * (HAUTEUR_SEUIL - (2 * _mobile[i].r)) + _mobile[i].r;
  _mobile[i].targetZ = gl4dmSURand() * _depth - _mobile[i].r;
}