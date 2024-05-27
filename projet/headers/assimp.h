#ifndef _ASSIMP_H

#define _ASSIMP_H

#ifdef __cplusplus
extern "C" {
#endif

  extern GLuint assimpGenScene(const char * filename);
  extern void   assimpDrawScene(GLuint id_scene);
  extern void   assimpQuit(GLuint id_scene);
  
#ifdef __cplusplus
}
#endif

#endif

