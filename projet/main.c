#include "headers/mobile.h"
#include "headers/predateur.h"
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <SDL_mixer.h>
#include <GL4D/gl4df.h>
#include <GL4D/gl4du.h>
#include <GL4D/gl4duw_SDL2.h>

#include <SDL2/SDL_ttf.h>


static void init(void);
static void draw(void);
static void quit(void);
static void keydown(int keycode);
static void resize(int width, int height);
static void drawPredatorView(void);
static void initAudio(const char * filename);
static void mixCallback(void *udata, Uint8 *stream, int len);

#define SHADOW_MAP_SIDE 512
#define ECHANTILLONS 1024

static int _wW = 1280, _wH = 720;

static GLuint _shPID = 0;
static GLuint _smPID = 0;

static GLuint _moineau = 0;
static GLuint _rapace = 0;

static GLuint _sphere = 0, _quad = 0;

static GLboolean _view_centroid = GL_FALSE;

static GLfloat _width = 8.0f;
static GLfloat _depth = 8.0f;

GLfloat _plan_s = 8.0f;

static GLuint _fbo = 0;
static GLuint _colorTex = 0;
static GLuint _depthTex = 0;
static GLuint _idTex = 0;
static GLuint _smTex = 0;
static GLuint _predatorFBO = 0;
static GLuint _predatorTex = 0;

static GLuint _nb_mobiles = 2000;

static GLfloat * _pixels = NULL;

static GLfloat _lumpos[] = { 13, 7, 0, 1 };

static GLboolean _predator_visible  = GL_FALSE;
static GLboolean _predator_view = GL_FALSE;
GLboolean _color_bird = GL_FALSE;

static Mix_Music * _mmusic = NULL;
static char _filename[128] = "audio/son.mid";
static GLfloat _hauteurs[ECHANTILLONS];

int main(int argc, char ** argv) {
  if(!gl4duwCreateWindow(argc, argv, "GL4D - Nuée d'étourneaux ", 0, 0, _wW, _wH, GL4DW_SHOWN))
    return 1;
  init();
  atexit(quit);
  gl4duwKeyDownFunc(keydown);
  gl4duwResizeFunc(resize); 
  gl4duwIdleFunc(mobileMove);
  gl4duwDisplayFunc(draw);
  gl4duwMainLoop();
  return 0;
}

static void init(void) {

  initAudio(_filename);

  glEnable(GL_DEPTH_TEST);
  _shPID  = gl4duCreateProgram("<vs>shaders/basic.vs", "<fs>shaders/basic.fs", NULL);
  _smPID  = gl4duCreateProgram("<vs>shaders/shadowMap.vs", "<fs>shaders/shadowMap.fs", NULL);

  _moineau = assimpGenScene("models/étourneau_trop_bg.glb");
  _rapace = assimpGenScene("models/étourneau_trop_bg.glb");
  
  gl4duGenMatrix(GL_FLOAT, "modelMatrix");
  gl4duGenMatrix(GL_FLOAT, "lightViewMatrix");
  gl4duGenMatrix(GL_FLOAT, "lightProjectionMatrix");
  gl4duGenMatrix(GL_FLOAT, "cameraViewMatrix");
  gl4duGenMatrix(GL_FLOAT, "cameraProjectionMatrix");
  gl4duGenMatrix(GL_FLOAT, "cameraPVMMatrix");

  glViewport(0, 0, _wW, _wH);
  gl4duBindMatrix("lightProjectionMatrix");
  gl4duLoadIdentityf();
  gl4duFrustumf(-1, 1, -1, 1, 1.5, 50.0);
  gl4duBindMatrix("cameraProjectionMatrix");
  gl4duLoadIdentityf();
  gl4duFrustumf(-0.5, 0.5, -0.5 * _wH / _wW, 0.5 * _wH / _wW, 1.0, 50.0);
  gl4duBindMatrix("modelMatrix");

  _sphere = gl4dgGenSpheref(30, 30);
  _quad = gl4dgGenQuadf();
  mobileInit(_nb_mobiles, _plan_s, _plan_s);
  predatorFree();

  glGenTextures(1, &_smTex);
  glBindTexture(GL_TEXTURE_2D, _smTex);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, SHADOW_MAP_SIDE, SHADOW_MAP_SIDE, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);

  glGenTextures(1, &_colorTex);
  glBindTexture(GL_TEXTURE_2D, _colorTex);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, _wW, _wH, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);

  glGenTextures(1, &_depthTex);
  glBindTexture(GL_TEXTURE_2D, _depthTex);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, _wW, _wH, 0, GL_DEPTH_COMPONENT, GL_UNSIGNED_BYTE, NULL);

  glGenTextures(1, &_idTex);
  glBindTexture(GL_TEXTURE_2D, _idTex);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_R32F, _wW, _wH, 0, GL_RED, GL_UNSIGNED_INT, NULL);

  glGenFramebuffers(1, &_fbo);

  glGenTextures(1, &_predatorTex);
  glBindTexture(GL_TEXTURE_2D, _predatorTex);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, _wW / 4, _wH / 4, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);

  glGenFramebuffers(1, &_predatorFBO);
  glBindFramebuffer(GL_FRAMEBUFFER, _predatorFBO);
  glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, _predatorTex, 0);
  glBindFramebuffer(GL_FRAMEBUFFER, 0);

  _pixels = malloc(_wW * _wH * sizeof *_pixels);
  assert(_pixels);
}

static void resize(int width, int height) {
  _wW = width;
  _wH = height;
  glViewport(0, 0, _wW, _wH);

  gl4duBindMatrix("cameraProjectionMatrix");
  gl4duLoadIdentityf();
  gl4duFrustumf(-0.5, 0.5, -0.5 * _wH / _wW, 0.5 * _wH / _wW, 1.0, 50.0);

  glBindTexture(GL_TEXTURE_2D, _colorTex);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, _wW, _wH, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);

  glBindTexture(GL_TEXTURE_2D, _depthTex);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, _wW, _wH, 0, GL_DEPTH_COMPONENT, GL_UNSIGNED_BYTE, NULL);

  glBindTexture(GL_TEXTURE_2D, _idTex);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_R32F, _wW, _wH, 0, GL_RED, GL_UNSIGNED_INT, NULL);

  glBindTexture(GL_TEXTURE_2D, _predatorTex);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, _wW / 4, _wH / 4, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);

  free(_pixels);
  _pixels = malloc(_wW * _wH * sizeof *_pixels);
  assert(_pixels);
}

static void keydown(int keycode) {
  if (keycode == SDLK_p) {
    if (_predator_visible) {
      predatorFree();
      _predator_visible = GL_FALSE;
    } else {
      predatorInit(_nb_mobiles, _plan_s, HAUTEUR_SEUIL, _plan_s);
      _predator_visible = GL_TRUE;
    }
  }
  if (keycode == SDLK_c) {
    _color_bird = !_color_bird;
  }
  if (keycode == SDLK_v) {
    _view_centroid = !_view_centroid;
  }
  if (keycode == SDLK_m) {
    _predator_view = !_predator_view;
  }
  if (keycode == SDLK_b) {
    useBoids = !useBoids;
  }
}

static void calculateCentroid(GLfloat *cx, GLfloat *cy, GLfloat *cz) {
  GLfloat sumX = 0, sumY = 0, sumZ = 0;
  int count = 0; // Compteur pour éviter la division par zéro

  for (int i = 0; i < _nb_mobiles; i++) {
    if (!isfinite(_mobile[i].x) || !isfinite(_mobile[i].y) || !isfinite(_mobile[i].z)) continue;
    sumX += _mobile[i].x;
    sumY += _mobile[i].y;
    sumZ += _mobile[i].z;
    count++;
  }

  if (count > 0) {
    *cx = sumX / count;
    *cy = sumY / count;
    *cz = sumZ / count;
  } else {
    *cx = 0;
    *cy = 0;
    *cz = 0;
  }

}

static void mixCallback(void *udata, Uint8 *stream, int len) {
  int i;
  Sint16 *s = (Sint16 *)stream;
  if(len >= 2 * ECHANTILLONS)
    for(i = 0; i < ECHANTILLONS; i++)
      _hauteurs[i] = _wH / 2 + (_wH / 2) * s[i] / ((1 << 15) - 1.0);
  return;
}
static void initAudio(const char * filename) {
  int mixFlags = MIX_INIT_OGG | MIX_INIT_MP3 | MIX_INIT_MOD, res;
  res = Mix_Init(mixFlags);
  if( (res & mixFlags) != mixFlags ) {
    fprintf(stderr, "Mix_Init: Erreur lors de l'initialisation de la bibliotheque SDL_Mixer\n");
    fprintf(stderr, "Mix_Init: %s\n", Mix_GetError());
  }
  if(Mix_OpenAudio(44100, AUDIO_S16LSB, 2, 1024) < 0)
    exit(4);
  if(!(_mmusic = Mix_LoadMUS(filename))) {
    fprintf(stderr, "Erreur lors du Mix_LoadMUS: %s\n", Mix_GetError());
    exit(5);
  }
  Mix_SetPostMix(mixCallback, NULL);
  if(!Mix_PlayingMusic())
    Mix_PlayMusic(_mmusic, -1);
}

static inline void scene(GLboolean sm) {
  glEnable(GL_CULL_FACE);

  GLfloat cx, cy, cz;
  calculateCentroid(&cx, &cy, &cz);

  if (sm) {
    glCullFace(GL_FRONT);
    glUseProgram(_smPID);
    gl4duBindMatrix("lightViewMatrix");
    gl4duLoadIdentityf();
    if (_view_centroid) {
      gl4duLookAtf(_lumpos[0], _lumpos[1], _lumpos[2], cx, cy, cz, 0, 1, 0);
    } else {
      gl4duLookAtf(_lumpos[0], _lumpos[1], _lumpos[2], 0, 2, 0, 0, 1, 0);
    }
    gl4duBindMatrix("modelMatrix");
    gl4duLoadIdentityf();
  } else {
    GLfloat vert[] = {0, 1, 0, 1}, lp[4], *mat;
    glCullFace(GL_BACK);
    glUseProgram(_shPID);
    glEnable(GL_TEXTURE_2D);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, _smTex);
    glUniform1i(glGetUniformLocation(_shPID, "smTex"), 0);
    gl4duBindMatrix("cameraViewMatrix");
    gl4duLoadIdentityf();
    if (_predator_view && _predator_visible) {
      gl4duLookAtf(_predator.x, _predator.y, _predator.z, cx, cy, cz, 0, 1, 0); 
    } else if (_view_centroid) {
      gl4duLookAtf(0, 4, 18, cx, cy, cz, 0, 1, 0); 
    } else {
      gl4duLookAtf(0, 4, 18, 0, 2, 0, 0, 1, 0); 
    }
    mat = gl4duGetMatrixData();
    MMAT4XVEC4(lp, mat, _lumpos);
    MVEC4WEIGHT(lp);
    glUniform4fv(glGetUniformLocation(_shPID, "lumpos"), 1, lp);
    gl4duBindMatrix("modelMatrix");
    gl4duLoadIdentityf();
    gl4duPushMatrix(); {
      gl4duTranslatef(_lumpos[0], _lumpos[1], _lumpos[2]);
      gl4duScalef(0.3f, 0.3f, 0.3f);
      gl4duSendMatrices();
    } gl4duPopMatrix();
    glUniform1i(glGetUniformLocation(_shPID, "id"), 2);
    glUniform1i(glGetUniformLocation(_shPID, "nb_mobiles"), _nb_mobiles);
    gl4dgDraw(_sphere);
    glUniform4fv(glGetUniformLocation(_shPID, "couleur"), 1, vert);
    glUniform1i(glGetUniformLocation(_shPID, "id"), 1);
  }

  gl4duPushMatrix(); {
    gl4duRotatef(-90, 1, 0, 0);
    gl4duScalef(_plan_s*2, _plan_s, _plan_s);
    gl4duSendMatrices();
  } gl4duPopMatrix();
  gl4dgDraw(_quad);
  gl4duSendMatrices();
  mobileDraw(_moineau);
  if (_predator_visible) {
    predatorDraw(_rapace);
  }
}

static void drawPredatorView(void) {
  glBindFramebuffer(GL_FRAMEBUFFER, _predatorFBO);
  glViewport(0, 0, _wW / 4, _wH / 4);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

  gl4duBindMatrix("cameraViewMatrix");
  gl4duLoadIdentityf();
  gl4duLookAtf(_predator.x, _predator.y, _predator.z, _predator.targetX, _predator.targetY, _predator.targetZ, 0, 1, 0);

  scene(GL_FALSE);

  glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

static void draw(void) {
  GLenum renderings[] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1 };
  glBindFramebuffer(GL_FRAMEBUFFER, _fbo);
  glDrawBuffer(GL_NONE);
  glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, 0, 0);
  glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, _smTex, 0);
  glViewport(0, 0, SHADOW_MAP_SIDE, SHADOW_MAP_SIDE);
  glClear(GL_DEPTH_BUFFER_BIT);
  scene(GL_TRUE);
  glDrawBuffers(1, &renderings[0]);
  glBindTexture(GL_TEXTURE_2D, _smTex);
  glGetTexImage(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, GL_FLOAT, _pixels);
  glBindTexture(GL_TEXTURE_2D, _colorTex);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, _wW, _wH, 0, GL_RED, GL_FLOAT, _pixels);
  gl4dfConvTex2Frame(_colorTex);

  glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, _colorTex, 0);
  glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D, _idTex, 0);
  glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, _depthTex, 0);
  glViewport(0, 0, _wW, _wH);
  glDrawBuffers(1, &renderings[1]);
  glClearColor(0, 0, 0, 0);
  glClear(GL_COLOR_BUFFER_BIT);
  glDrawBuffers(1, renderings);
  glClearColor(0.0f, 1.0f, 1.0f, 1.0f); 
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  glDrawBuffers(2, renderings);

  scene(GL_FALSE);

  glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
  glBlitFramebuffer(0, 0, _wW, _wH, 0, 0, _wW, _wH, GL_COLOR_BUFFER_BIT, GL_LINEAR);
  glBlitFramebuffer(0, 0, _wW, _wH, 0, 0, _wW, _wH, GL_DEPTH_BUFFER_BIT, GL_NEAREST);

  if (_predator_visible) {
    drawPredatorView();

    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
    glViewport(_wW - _wW / 4 - 10, 10, _wW / 4, _wH / 4);
    glBindTexture(GL_TEXTURE_2D, _predatorTex);
    glEnable(GL_TEXTURE_2D);
    gl4duGenMatrix(GL_FLOAT, "miniMapMatrix");
    gl4duBindMatrix("miniMapMatrix");
    gl4duLoadIdentityf();
    gl4duScalef(1.0f, 1.0f, 1.0f);
    gl4duSendMatrices();
    gl4dgDraw(_quad);
    glDisable(GL_TEXTURE_2D);
  }
}

static void quit(void) {
  if(_fbo) {
    glDeleteTextures(1, &_colorTex);
    glDeleteTextures(1, &_depthTex);
    glDeleteTextures(1, &_idTex);
    glDeleteTextures(1, &_smTex);
    glDeleteFramebuffers(1, &_fbo);
    _fbo = 0;
  }
  if(_pixels) {
    free(_pixels);
    _pixels = NULL;
  }
  gl4duClean(GL4DU_ALL);
  if (_predator_visible) {
    predatorFree();
  }
  if(_predatorFBO) {
    glDeleteTextures(1, &_predatorTex);
    glDeleteFramebuffers(1, &_predatorFBO);
    _predatorFBO = 0;
  }
}
