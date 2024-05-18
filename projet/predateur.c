// predateur.c
#include "predateur.h"

// Instance du prédateur
predator_t _predator;

void predatorInit(GLfloat width, GLfloat height, GLfloat depth) {
  _predator.r = 0.2f;
  _predator.x = gl4dmSURand() * width - _predator.r;
  _predator.y = height / 2;
  _predator.z = gl4dmSURand() * depth - _predator.r;
  _predator.vx = 0.0f;
  _predator.vy = 0.0f;
  _predator.vz = 0.0f;
  _predator.color[0] = 1.0f; // Rouge pour le prédateur
  _predator.color[1] = 0.0f;
  _predator.color[2] = 0.0f;
  _predator.color[3] = 1.0f;
}

void predatorMove(GLfloat width, GLfloat height, GLfloat depth) {
  // Déplacez le prédateur de manière simple pour cet exemple
  _predator.vx += (gl4dmSURand() - 0.5f) * 0.1f;
  _predator.vy += (gl4dmSURand() - 0.5f) * 0.1f;
  _predator.vz += (gl4dmSURand() - 0.5f) * 0.1f;

  _predator.x += _predator.vx;
  _predator.y += _predator.vy;
  _predator.z += _predator.vz;

  // Éviter les murs
  if(_predator.x < -width + _predator.r) {
    _predator.vx = -_predator.vx;
  } else if(_predator.x > width - _predator.r) {
    _predator.vx = -_predator.vx;
  }

  if(_predator.y < _predator.r) {
    _predator.vy = -_predator.vy;
  } else if(_predator.y > height - _predator.r) {
    _predator.vy = -_predator.vy;
  }

  if(_predator.z < -depth + _predator.r) {
    _predator.vz = -_predator.vz;
  } else if(_predator.z > depth - _predator.r) {
    _predator.vz = -_predator.vz;
  }
}

void predatorDraw(GLuint obj) {
  GLint pId;
  glGetIntegerv(GL_CURRENT_PROGRAM, &pId);
  gl4duPushMatrix();
  gl4duTranslatef(_predator.x, _predator.y, _predator.z);
  gl4duScalef(_predator.r, _predator.r, _predator.r);
  gl4duSendMatrices();
  gl4duPopMatrix();
  glUniform1i(glGetUniformLocation(pId, "id"), 1); // ID pour le prédateur
  glUniform4fv(glGetUniformLocation(pId, "couleur"), 1, _predator.color);
  gl4dgDraw(obj);
}
