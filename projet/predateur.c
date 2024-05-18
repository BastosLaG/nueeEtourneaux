#include <GL4D/gl4du.h>
#include <GL4D/gl4dg.h>
#include <stdlib.h>
#include <assert.h>
#include "predateur.h"

predator_t _predator;

void predatorInit(GLfloat width, GLfloat height, GLfloat depth) {
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
}

void predatorMove(GLfloat width, GLfloat height, GLfloat depth) {
  _predator.x += _predator.vx;
  _predator.y += _predator.vy;
  _predator.z += _predator.vz;

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

  // Ajout d'une légère variation à la vitesse pour rendre le mouvement plus naturel
  _predator.vx += 0.01f * (gl4dmSURand() - 0.5f);
  _predator.vy += 0.01f * (gl4dmSURand() - 0.5f);
  _predator.vz += 0.01f * (gl4dmSURand() - 0.5f);

  // Limitation de la vitesse maximale
  GLfloat maxSpeed = 0.2f;
  GLfloat speed = sqrt(_predator.vx * _predator.vx + _predator.vy * _predator.vy + _predator.vz * _predator.vz);
  if (speed > maxSpeed) {
    _predator.vx = (_predator.vx / speed) * maxSpeed;
    _predator.vy = (_predator.vy / speed) * maxSpeed;
    _predator.vz = (_predator.vz / speed) * maxSpeed;
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
  glUniform4fv(glGetUniformLocation(pId, "couleur"), 1, _predator.color);
  gl4dgDraw(obj);
}
