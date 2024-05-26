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

//test
// Déclaration des fonctions
static void init(void);
static void mouse(int button, int state, int x, int y);
static void motion(int x, int y);
static void draw(void);
static void quit(void);
static void keydown(int keycode);
static void resize(int width, int height);
static void drawPredatorView(void);
static void initAudio(const char * filename);
static void mixCallback(void *udata, Uint8 *stream, int len);

#define SHADOW_MAP_SIDE 512
#define ECHANTILLONS 1024

// Dimensions de la fenêtre
static int _wW = 1280, _wH = 720;

// Identifiants des programmes GLSL
static GLuint _shPID = 0;
static GLuint _smPID = 0;

// Identifiant de notre étourneau
static GLuint _moineau = 0;
static GLuint _rapace = 0;

// Quelques objets géométriques
static GLuint _sphere = 0, _quad = 0;

static GLboolean _view_centroid = GL_FALSE;

// Scale du plan
GLfloat _plan_s = 8.0f;

// Framebuffer Object et textures associées
static GLuint _fbo = 0;
static GLuint _colorTex = 0;
static GLuint _depthTex = 0;
static GLuint _idTex = 0;
static GLuint _smTex = 0;
static GLuint _predatorFBO = 0;
static GLuint _predatorTex = 0;

// Nombre de mobiles créés dans la scène
static GLuint _nb_mobiles = 500;

// Identifiant et coordonnées du mobile sélectionné
static int _picked_mobile = -1;
static GLfloat _picked_mobile_coords[4] = {0};

// Copie CPU de la mémoire texture d'identifiants
static GLfloat * _pixels = NULL;

// Position de la lumière, relative aux objets
static GLfloat _lumpos[] = { 9, 3, 0, 1 };

// Variable de contrôle pour l'affichage du prédateur
static GLboolean _predator_visible  = GL_FALSE;
static GLboolean _predator_view = GL_FALSE;
GLboolean _color_bird = GL_FALSE;

//Gestion de la musique
static Mix_Music * _mmusic = NULL;
static char _filename[128] = "audio/son.mid";
static GLfloat _hauteurs[ECHANTILLONS];

// Fonction principale pour créer la fenêtre, initialiser GL et lancer la boucle principale d'affichage
int main(int argc, char ** argv) {
  if(!gl4duwCreateWindow(argc, argv, "GL4D - Les oiseaux qui volent en groupe", 0, 0, _wW, _wH, GL4DW_SHOWN))
    return 1;
  init();
  atexit(quit);
  gl4duwMouseFunc(mouse);
  gl4duwMotionFunc(motion);
  gl4duwKeyDownFunc(keydown);
  gl4duwResizeFunc(resize);  // Ajouter le gestionnaire de redimensionnement
  gl4duwIdleFunc(mobileMove);
  gl4duwDisplayFunc(draw);
  gl4duwMainLoop();
  return 0;
}

// Initialise les paramètres OpenGL
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

  // Création et paramétrage de la Texture de shadow map
  glGenTextures(1, &_smTex);
  glBindTexture(GL_TEXTURE_2D, _smTex);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, SHADOW_MAP_SIDE, SHADOW_MAP_SIDE, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);

  // Création et paramétrage de la Texture recevant la couleur
  glGenTextures(1, &_colorTex);
  glBindTexture(GL_TEXTURE_2D, _colorTex);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, _wW, _wH, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);

  // Création et paramétrage de la Texture recevant la profondeur
  glGenTextures(1, &_depthTex);
  glBindTexture(GL_TEXTURE_2D, _depthTex);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, _wW, _wH, 0, GL_DEPTH_COMPONENT, GL_UNSIGNED_BYTE, NULL);

  // Création et paramétrage de la Texture recevant les identifiants d'objets
  glGenTextures(1, &_idTex);
  glBindTexture(GL_TEXTURE_2D, _idTex);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_R32F, _wW, _wH, 0, GL_RED, GL_UNSIGNED_INT, NULL);

  // Création du Framebuffer Object
  glGenFramebuffers(1, &_fbo);

  // Création et paramétrage de la Texture pour la vue du prédateur
  glGenTextures(1, &_predatorTex);
  glBindTexture(GL_TEXTURE_2D, _predatorTex);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, _wW / 4, _wH / 4, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);

  // Création du Framebuffer Object pour la vue du prédateur
  glGenFramebuffers(1, &_predatorFBO);
  glBindFramebuffer(GL_FRAMEBUFFER, _predatorFBO);
  glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, _predatorTex, 0);
  glBindFramebuffer(GL_FRAMEBUFFER, 0);

  _pixels = malloc(_wW * _wH * sizeof *_pixels);
  assert(_pixels);
}

// Fonction de redimensionnement
static void resize(int width, int height) {
  _wW = width;
  _wH = height;
  glViewport(0, 0, _wW, _wH);

  // Mettre à jour les matrices de projection
  gl4duBindMatrix("cameraProjectionMatrix");
  gl4duLoadIdentityf();
  gl4duFrustumf(-0.5, 0.5, -0.5 * _wH / _wW, 0.5 * _wH / _wW, 1.0, 50.0);

  // Redimensionner les textures
  glBindTexture(GL_TEXTURE_2D, _colorTex);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, _wW, _wH, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);

  glBindTexture(GL_TEXTURE_2D, _depthTex);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, _wW, _wH, 0, GL_DEPTH_COMPONENT, GL_UNSIGNED_BYTE, NULL);

  glBindTexture(GL_TEXTURE_2D, _idTex);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_R32F, _wW, _wH, 0, GL_RED, GL_UNSIGNED_INT, NULL);

  // Redimensionner la texture de la vue du prédateur
  glBindTexture(GL_TEXTURE_2D, _predatorTex);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, _wW / 4, _wH / 4, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);

  free(_pixels);
  _pixels = malloc(_wW * _wH * sizeof *_pixels);
  assert(_pixels);
}

// Gestion des événements clavier
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
}

// Call-back au clic (tous les boutons avec état down (1) ou up (0))
static void mouse(int button, int state, int x, int y) {
  if(button == GL4D_BUTTON_LEFT) {
    y = _wH - y;
    glBindTexture(GL_TEXTURE_2D, _idTex);
    glGetTexImage(GL_TEXTURE_2D, 0, GL_RED, GL_FLOAT, _pixels);
    if(x >= 0 && x < _wW && y >=0 && y < _wH)
      _picked_mobile = (int)((_nb_mobiles + 2.0f) * _pixels[y * _wW + x]) - 3;
    if(_picked_mobile >= 0 && _picked_mobile < _nb_mobiles) {
      mobileSetFreeze(_picked_mobile, state);
      if(state) {
        // Récupération de la coordonnée espace-écran du mobile
        GLfloat m[16], tmpp[16], tmpm[16], * gl4dm;
        GLfloat mcoords[] = {0, 0, 0, 1};
        // Récupération des coordonnées spatiales du mobile
        mobileGetCoords(_picked_mobile, mcoords);
        // Copie de la matrice de projection dans tmpp
        gl4duBindMatrix("cameraProjectionMatrix");
        gl4dm = gl4duGetMatrixData();
        memcpy(tmpp, gl4dm, sizeof tmpp);
        // Copie de la matrice de vue dans tmpm
        gl4duBindMatrix("cameraViewMatrix");
        gl4dm = gl4duGetMatrixData();
        memcpy(tmpm, gl4dm, sizeof tmpm);
        // m est tmpp x tmpm
        MMAT4XMAT4(m, tmpp, tmpm);
        // Modélisation et projection de la coordonnée du mobile dans _picked_mobile_coords
        MMAT4XVEC4(_picked_mobile_coords, m, mcoords);
        MVEC4WEIGHT(_picked_mobile_coords);
      }
    }
    if(!state)
      _picked_mobile = -1;
  }
}

// Call-back lors du drag souris
static void motion(int x, int y) {
  if(_picked_mobile >= 0 && _picked_mobile < _nb_mobiles) {
    GLfloat m[16], tmpp[16], tmpm[16], * gl4dm;
    // p est la coordonnée de la souris entre -1 et +1
    GLfloat p[] = { 2.0f * x / (GLfloat)_wW - 1.0f,
                    -(2.0f * y / (GLfloat)_wH - 1.0f), 
                    0.0f, 1.0 }, ip[4];
    // Copie de la matrice de projection dans tmpp
    gl4duBindMatrix("cameraProjectionMatrix");
    gl4dm = gl4duGetMatrixData();
    memcpy(tmpp, gl4dm, sizeof tmpp);
    // Copie de la matrice de vue dans tmpm
    gl4duBindMatrix("cameraViewMatrix");
    gl4dm = gl4duGetMatrixData();
    memcpy(tmpm, gl4dm, sizeof tmpm);
    // m est tmpp x tmpm
    MMAT4XMAT4(m, tmpp, tmpm);
    // Inversion de m
    MMAT4INVERSE(m);
    // Ajout de la profondeur à l'écran du mobile comme profondeur du clic
    p[2] = _picked_mobile_coords[2];
    // ip est la transformée inverse de la coordonnée du clic (donc coordonnée spatiale du clic)
    MMAT4XVEC4(ip, m, p);
    MVEC4WEIGHT(ip);
    // Affectation de ip comme nouvelle coordonnée spatiale du mobile
    mobileSetCoords(_picked_mobile, ip);
  }
}

// Calcule le centroid de tous les oiseaux
static void calculateCentroid(GLfloat *cx, GLfloat *cy, GLfloat *cz) {
  GLfloat sumX = 0, sumY = 0, sumZ = 0;
  for (int i = 0; i < _nb_mobiles; i++) {
    sumX += _mobile[i].x;
    sumY += _mobile[i].y;
    sumZ += _mobile[i].z;
  }
  *cx = sumX / _nb_mobiles;
  *cy = sumY / _nb_mobiles;
  *cz = sumZ / _nb_mobiles;
}

// Musique
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
    //exit(3); commenté car ne réagit correctement sur toutes les architectures
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

// La scène est soit dessinée du point de vue de la lumière (sm = GL_TRUE donc shadow map) soit dessinée du point de vue de la caméra
static inline void scene(GLboolean sm) {
  glEnable(GL_CULL_FACE);

  // Calculer le centroid
  GLfloat cx, cy, cz;
  calculateCentroid(&cx, &cy, &cz);

  if (sm) {
    glCullFace(GL_FRONT);
    glUseProgram(_smPID);
    gl4duBindMatrix("lightViewMatrix");
    gl4duLoadIdentityf();
    if (_view_centroid) {
      gl4duLookAtf(_lumpos[0], _lumpos[1], _lumpos[2], cx, cy, cz, 0, 1, 0); // Utiliser le centroid
    } else {
      gl4duLookAtf(_lumpos[0], _lumpos[1], _lumpos[2], 0, 2, 0, 0, 1, 0); // Vue par défaut
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
      gl4duLookAtf(_predator.x, _predator.y, _predator.z, cx, cy, cz, 0, 1, 0); // Vue du prédateur
    } else if (_view_centroid) {
      gl4duLookAtf(0, 4, 18, cx, cy, cz, 0, 1, 0); // Utiliser le centroid
    } else {
      gl4duLookAtf(0, 4, 18, 0, 2, 0, 0, 1, 0); // Vue par défaut
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
    gl4duScalef(_plan_s, _plan_s, _plan_s);
    gl4duSendMatrices();
  } gl4duPopMatrix();
  gl4dgDraw(_quad);
  gl4duSendMatrices();
  mobileDraw(_moineau);
  if (_predator_visible) {
    predatorDraw(_rapace);
  }
}

// Dessine la vue du prédateur
static void drawPredatorView(void) {
  glBindFramebuffer(GL_FRAMEBUFFER, _predatorFBO);
  glViewport(0, 0, _wW / 4, _wH / 4);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

  gl4duBindMatrix("cameraViewMatrix");
  gl4duLoadIdentityf();
  GLfloat cx, cy, cz;
  calculateCentroid(&cx, &cy, &cz);
  gl4duLookAtf(_predator.x, _predator.y, _predator.z, cx, cy, cz, 0, 1, 0);

  scene(GL_FALSE);

  glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

// Dessine dans le contexte OpenGL actif
static void draw(void) {
  GLenum renderings[] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1 };
  glBindFramebuffer(GL_FRAMEBUFFER, _fbo);
  // Désactiver le rendu de couleur et ne laisser que le depth, dans _smTex
  glDrawBuffer(GL_NONE);
  glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, 0, 0);
  glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, _smTex, 0);
  // Viewport de la shadow map et dessin de la scène du point de vue de la lumière
  glViewport(0, 0, SHADOW_MAP_SIDE, SHADOW_MAP_SIDE);
  glClear(GL_DEPTH_BUFFER_BIT);
  scene(GL_TRUE);
  glDrawBuffers(1, &renderings[0]);
  glBindTexture(GL_TEXTURE_2D, _smTex);
  glGetTexImage(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, GL_FLOAT, _pixels);
  glBindTexture(GL_TEXTURE_2D, _colorTex);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, _wW, _wH, 0, GL_RED, GL_FLOAT, _pixels);
  gl4dfConvTex2Frame(_colorTex);

  // Paramétrer le fbo pour 2 rendus couleurs + un (autre) depth
  glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, _colorTex, 0);
  glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D, _idTex, 0);
  glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, _depthTex, 0);
  glViewport(0, 0, _wW, _wH);
  // Un seul rendu GL_COLOR_ATTACHMENT1 + effacement 0
  glDrawBuffers(1, &renderings[1]);
  glClearColor(0, 0, 0, 0);
  glClear(GL_COLOR_BUFFER_BIT);
  // Un seul rendu GL_COLOR_ATTACHMENT0 + effacement couleur et depth
  glDrawBuffers(1, renderings);
  glClearColor(0.0f, 0.0f, 1.0f, 1.0f); 
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  // Deux rendus GL_COLOR_ATTACHMENT0 et GL_COLOR_ATTACHMENT1
  glDrawBuffers(2, renderings);

  scene(GL_FALSE);

  // Copie du fbo à l'écran
  glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
  glBlitFramebuffer(0, 0, _wW, _wH, 0, 0, _wW, _wH, GL_COLOR_BUFFER_BIT, GL_LINEAR);
  glBlitFramebuffer(0, 0, _wW, _wH, 0, 0, _wW, _wH, GL_DEPTH_BUFFER_BIT, GL_NEAREST);

  // Dessiner la vue du prédateur
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


// Libère les éléments utilisés au moment de sortir du programme (atexit)
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
