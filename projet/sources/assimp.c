#include <assert.h>

#include <GL4D/gl4duw_SDL2.h>
#include <SDL_image.h>

#include <assimp/cimport.h>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

typedef struct scene_t scene_t;

struct scene_t {
  char * filename;
  const struct aiScene * scene;
  struct aiVector3D scene_min, scene_max, scene_center;
  GLuint * vaos, * buffers, * counts, * textures, nb_meshes, nb_textures;
};

#define aisgl_min(x,y) (x<y?x:y)
#define aisgl_max(x,y) (y>x?y:x)

static void get_bounding_box_for_node (scene_t * cur_scene, const struct aiNode* nd, struct aiVector3D* min, struct aiVector3D* max, struct aiMatrix4x4* trafo);
static void get_bounding_box (scene_t * cur_scene, struct aiVector3D* min, struct aiVector3D* max);
static void color4_to_float4(const struct aiColor4D *c, float f[4]);
static void set_float4(float f[4], float a, float b, float c, float d);
static void apply_material(const struct aiMaterial *mtl);
static void sceneMkVAOs (scene_t * cur_scene, const struct aiScene *sc, const struct aiNode* nd, GLuint * ivao);
static void sceneDrawVAOs(scene_t * cur_scene, const struct aiScene *sc, const struct aiNode* nd, GLuint * ivao);
static int  sceneNbMeshes(const struct aiScene *sc, const struct aiNode* nd, int subtotal);
static int  loadasset (scene_t * cur_scene);
static void _scenes_init(void);
static void _scenes_free(void);
static scene_t * _scenes_from_id(GLuint id_scene);
static GLuint _scenes_add(const char * filename);

static scene_t ** _scenes = NULL;
static GLuint _nb_scenes = 0;
static GLuint _size_scenes = 0;

GLuint assimpGenScene(const char * filename) {
  int i;
  GLuint ivao = 0, id_scene = 0;
  scene_t * cur_scene = NULL;
  struct aiLogStream stream;

  stream = aiGetPredefinedLogStream(aiDefaultLogStream_STDOUT, NULL);
  aiAttachLogStream(&stream);

  stream = aiGetPredefinedLogStream(aiDefaultLogStream_FILE,"assimp_log.txt");
  aiAttachLogStream(&stream);

  if(_scenes == NULL)
    _scenes_init();
  id_scene = _scenes_add(filename);
  assert(id_scene);
  cur_scene = _scenes_from_id(id_scene);
  assert(cur_scene);  

  if(loadasset(cur_scene) != 0) {
    fprintf(stderr, "Erreur lors du chargement du fichier %s\n", filename);
    exit(3);
  } 
  if(getenv("MODEL_IS_BROKEN"))  
    glFrontFace(GL_CW);

  cur_scene->textures = malloc((cur_scene->nb_textures = cur_scene->scene->mNumMaterials) * sizeof *(cur_scene->textures));
  assert(cur_scene->textures);
  
  glGenTextures(cur_scene->nb_textures, cur_scene->textures);

  for (i = 0; i < cur_scene->scene->mNumMaterials ; i++) {
    const struct aiMaterial* pMaterial = cur_scene->scene->mMaterials[i];
    if (aiGetMaterialTextureCount(pMaterial, aiTextureType_DIFFUSE) > 0) {
      struct aiString tfname;
      char * dir = pathOf(filename), buf[BUFSIZ];
      if (aiGetMaterialTexture(pMaterial, aiTextureType_DIFFUSE, 0, &tfname, NULL, NULL, NULL, NULL, NULL, NULL) == AI_SUCCESS) {
	SDL_Surface * t;
	snprintf(buf, sizeof buf, "%s/%s", dir, tfname.data);

	if(!(t = IMG_Load(buf))) { 
	  fprintf(stderr, "Probleme de chargement de textures %s\n", buf); 
	  fprintf(stderr, "\tNouvel essai avec %s\n", tfname.data);
	  if(!(t = IMG_Load(tfname.data))) { fprintf(stderr, "Probleme de chargement de textures %s\n", tfname.data); continue; }
	}
	glBindTexture(GL_TEXTURE_2D, cur_scene->textures[i]);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT/* GL_CLAMP_TO_EDGE */);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT/* GL_CLAMP_TO_EDGE */);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, t->w, t->h, 0, t->format->BytesPerPixel == 3 ? GL_RGB : GL_RGBA, GL_UNSIGNED_BYTE, t->pixels);
	SDL_FreeSurface(t);
      }
    }
  }

  cur_scene->nb_meshes= sceneNbMeshes(cur_scene->scene, cur_scene->scene->mRootNode, 0);
  cur_scene->vaos = malloc(cur_scene->nb_meshes * sizeof *(cur_scene->vaos));
  assert(cur_scene->vaos);
  glGenVertexArrays(cur_scene->nb_meshes, cur_scene->vaos);
  cur_scene->buffers = malloc(2 * cur_scene->nb_meshes * sizeof *(cur_scene->buffers));
  assert(cur_scene->buffers);
  glGenBuffers(2 * cur_scene->nb_meshes, cur_scene->buffers);
  cur_scene->counts = calloc(cur_scene->nb_meshes, sizeof *(cur_scene->counts));
  assert(cur_scene->counts);
  sceneMkVAOs(cur_scene, cur_scene->scene, cur_scene->scene->mRootNode, &ivao);
  return id_scene;
}

void assimpDrawScene(GLuint id_scene) {
  GLfloat tmp;
  GLuint ivao = 0;
  scene_t * cur_scene = _scenes_from_id(id_scene);
  assert(cur_scene);  
  tmp = cur_scene->scene_max.x - cur_scene->scene_min.x;
  tmp = aisgl_max(cur_scene->scene_max.y - cur_scene->scene_min.y, tmp);
  tmp = aisgl_max(cur_scene->scene_max.z - cur_scene->scene_min.z, tmp);
  tmp = 1.0f / tmp;
  gl4duScalef(tmp, tmp, tmp);
  gl4duTranslatef( -cur_scene->scene_center.x, -cur_scene->scene_center.y, -cur_scene->scene_center.z);
  sceneDrawVAOs(cur_scene, cur_scene->scene, cur_scene->scene->mRootNode, &ivao);
}

void assimpDeleteScene(GLuint id_scene) {
  scene_t * cur_scene = _scenes_from_id(id_scene);
  assert(cur_scene);  
  aiReleaseImport(cur_scene->scene);
  aiDetachAllLogStreams();
  if(cur_scene->counts) {
    free(cur_scene->counts);
    cur_scene->counts = NULL;
  }
  if(cur_scene->textures) {
    glDeleteTextures(cur_scene->nb_textures, cur_scene->textures);
    free(cur_scene->textures);
    cur_scene->textures = NULL;
  }
  if(cur_scene->vaos) {
    glDeleteVertexArrays(cur_scene->nb_meshes, cur_scene->vaos);
    free(cur_scene->vaos);
    cur_scene->vaos = NULL;
  }
  if(cur_scene->buffers) {
    glDeleteBuffers(2 * cur_scene->nb_meshes, cur_scene->buffers);
    free(cur_scene->buffers);
    cur_scene->buffers = NULL;
  }
  if(cur_scene->filename) {
    free(cur_scene->filename);
    cur_scene->filename = NULL;
  }
}

static void get_bounding_box_for_node(scene_t * cur_scene, const struct aiNode* nd, struct aiVector3D* min, struct aiVector3D* max, struct aiMatrix4x4* trafo) {
  struct aiMatrix4x4 prev;
  unsigned int n = 0, t;
  prev = *trafo;
  aiMultiplyMatrix4(trafo,&nd->mTransformation);
  for (; n < nd->mNumMeshes; ++n) {
    const struct aiMesh* mesh = cur_scene->scene->mMeshes[nd->mMeshes[n]];
    for (t = 0; t < mesh->mNumVertices; ++t) {
      struct aiVector3D tmp = mesh->mVertices[t];
      aiTransformVecByMatrix4(&tmp,trafo);
      min->x = aisgl_min(min->x,tmp.x);
      min->y = aisgl_min(min->y,tmp.y);
      min->z = aisgl_min(min->z,tmp.z);
      max->x = aisgl_max(max->x,tmp.x);
      max->y = aisgl_max(max->y,tmp.y);
      max->z = aisgl_max(max->z,tmp.z);
    }
  }
  for (n = 0; n < nd->mNumChildren; ++n) {
    get_bounding_box_for_node(cur_scene, nd->mChildren[n],min,max,trafo);
  }
  *trafo = prev;
}

static void get_bounding_box (scene_t * cur_scene, struct aiVector3D* min, struct aiVector3D* max) {
  struct aiMatrix4x4 trafo;
  aiIdentityMatrix4(&trafo);
  min->x = min->y = min->z =  1e10f;
  max->x = max->y = max->z = -1e10f;
  get_bounding_box_for_node(cur_scene, cur_scene->scene->mRootNode,min,max,&trafo);
}

static void color4_to_float4(const struct aiColor4D *c, float f[4]) {
  f[0] = c->r; f[1] = c->g; f[2] = c->b; f[3] = c->a;
}

static void set_float4(float f[4], float a, float b, float c, float d) {
  f[0] = a; f[1] = b; f[2] = c; f[3] = d;
}

static void apply_material(const struct aiMaterial *mtl) {
  float c[4];
  unsigned int max;
  float shininess, strength;
  struct aiColor4D diffuse, specular, ambient, emission;
  GLint id;
  glGetIntegerv(GL_CURRENT_PROGRAM, &id);
  
  set_float4(c, 0.8f, 0.8f, 0.8f, 1.0f);
  if (AI_SUCCESS == aiGetMaterialColor(mtl, AI_MATKEY_COLOR_DIFFUSE, &diffuse)){
    color4_to_float4(&diffuse, c);
  }
  glUniform4fv(glGetUniformLocation(id, "diffuse_color"), 1, c);

  set_float4(c, 0.0f, 0.0f, 0.0f, 1.0f);
  if (AI_SUCCESS == aiGetMaterialColor(mtl, AI_MATKEY_COLOR_SPECULAR, &specular)){
    color4_to_float4(&specular, c);
  }
  glUniform4fv(glGetUniformLocation(id, "specular_color"), 1, c);

  set_float4(c, 0.2f, 0.2f, 0.2f, 1.0f);
  if (AI_SUCCESS == aiGetMaterialColor(mtl, AI_MATKEY_COLOR_AMBIENT, &ambient)){
    color4_to_float4(&ambient, c);
  }
  glUniform4fv(glGetUniformLocation(id, "ambient_color"), 1, c);

  set_float4(c, 0.0f, 0.0f, 0.0f, 1.0f);
  if (AI_SUCCESS == aiGetMaterialColor(mtl, AI_MATKEY_COLOR_EMISSIVE, &emission)){
    color4_to_float4(&emission, c);
  }
  glUniform4fv(glGetUniformLocation(id, "emission_color"), 1, c);

  max = 1;
  if(aiGetMaterialFloatArray(mtl, AI_MATKEY_SHININESS, &shininess, &max) == AI_SUCCESS) {
    max = 1;
    if(aiGetMaterialFloatArray(mtl, AI_MATKEY_SHININESS_STRENGTH, &strength, &max) == AI_SUCCESS)
      glUniform1f(glGetUniformLocation(id, "shininess"), shininess * strength);
    else
	glUniform1f(glGetUniformLocation(id, "shininess"), shininess);
  } else {
    shininess = 0.0;
    glUniform1f(glGetUniformLocation(id, "shininess"), shininess);
  }
}

static void sceneMkVAOs(scene_t * cur_scene, const struct aiScene *sc, const struct aiNode* nd, GLuint * ivao) {
  int i, j, comp;
  unsigned int n = 0;
  static int temp = 0;
  
  temp++;

  for (; n < nd->mNumMeshes; ++n) {
    GLfloat * vertices = NULL;
    GLuint  * indices  = NULL;
    const struct aiMesh* mesh = sc->mMeshes[nd->mMeshes[n]];
    comp  = mesh->mVertices ? 3 : 0;
    comp += mesh->mNormals ? 3 : 0;
    comp += mesh->mTextureCoords[0] ? 2 : 0;
    if(mesh->mBones) {
      fprintf(stderr, "Tableau de %d bones : %p\n", mesh->mNumBones, mesh->mBones);
      for(int ib = 0; ib < mesh->mNumBones; ++ib) {
	fprintf(stderr, "\t Bone %d name : %s\n", ib, (char *)(mesh->mBones[ib]->mName.data));
      }
    }
    if(!comp) continue;

    glBindVertexArray(cur_scene->vaos[*ivao]);
    glBindBuffer(GL_ARRAY_BUFFER, cur_scene->buffers[2 * (*ivao)]);

    vertices = malloc(comp * mesh->mNumVertices * sizeof *vertices);
    assert(vertices);
    i = 0;
    glDisableVertexAttribArray(0);
    glDisableVertexAttribArray(1);
    glDisableVertexAttribArray(2);
    if(mesh->mVertices) {
      glEnableVertexAttribArray(0);
      glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, (const void *)(i * sizeof *vertices));
      for(j = 0; j < mesh->mNumVertices; ++j) {
	vertices[i++] = mesh->mVertices[j].x;
	vertices[i++] = mesh->mVertices[j].y;
	vertices[i++] = mesh->mVertices[j].z;
      }      
    }
    if(mesh->mNormals) {
      glEnableVertexAttribArray(1);
      glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, (const void *)(i * sizeof *vertices));
      for(j = 0; j < mesh->mNumVertices; ++j) {
	vertices[i++] = mesh->mNormals[j].x;
	vertices[i++] = mesh->mNormals[j].y;
	vertices[i++] = mesh->mNormals[j].z;
      }      
    }
    if(mesh->mTextureCoords[0]) {
      glEnableVertexAttribArray(2);
      glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 0, (const void *)(i * sizeof *vertices));
      for(j = 0; j < mesh->mNumVertices; ++j) {
	vertices[i++] = mesh->mTextureCoords[0][j].x;
	vertices[i++] = mesh->mTextureCoords[0][j].y;
      }      
    }
    glBufferData(GL_ARRAY_BUFFER, (i * sizeof *vertices), vertices, GL_STATIC_DRAW);      
    free(vertices);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, cur_scene->buffers[2 * (*ivao) + 1]);
    if(mesh->mFaces) {
      indices = malloc(3 * mesh->mNumFaces * sizeof *indices);
      assert(indices);
      for(i = 0, j = 0; j < mesh->mNumFaces; ++j) {
	assert(mesh->mFaces[j].mNumIndices < 4);
	if(mesh->mFaces[j].mNumIndices != 3) continue;
	indices[i++] = mesh->mFaces[j].mIndices[0];
	indices[i++] = mesh->mFaces[j].mIndices[1];
	indices[i++] = mesh->mFaces[j].mIndices[2];
      }
      glBufferData(GL_ELEMENT_ARRAY_BUFFER, i * sizeof *indices, indices, GL_STATIC_DRAW);
      cur_scene->counts[*ivao] = i;
      free(indices);
    }
    glBindVertexArray(0);
    (*ivao)++;
  }
  for (n = 0; n < nd->mNumChildren; ++n) {
    sceneMkVAOs(cur_scene, sc, nd->mChildren[n], ivao);
  }
}


static void sceneDrawVAOs(scene_t * cur_scene, const struct aiScene *sc, const struct aiNode* nd, GLuint * ivao) {
  unsigned int n = 0;
  struct aiMatrix4x4 m = nd->mTransformation;
  GLint id;

  glGetIntegerv(GL_CURRENT_PROGRAM, &id);
  gl4duPushMatrix();
  gl4duMultMatrixf((GLfloat*)&m);
  gl4duSendMatrices();

  for (; n < nd->mNumMeshes; ++n) {
    const struct aiMesh* mesh = sc->mMeshes[nd->mMeshes[n]];
    if(cur_scene->counts[*ivao]) {
      glBindVertexArray(cur_scene->vaos[*ivao]);
      apply_material(sc->mMaterials[mesh->mMaterialIndex]);
      if (aiGetMaterialTextureCount(sc->mMaterials[mesh->mMaterialIndex], aiTextureType_DIFFUSE) > 0) {
	glBindTexture(GL_TEXTURE_2D, cur_scene->textures[mesh->mMaterialIndex]);
	glUniform1i(glGetUniformLocation(id, "hasTexture"), 1);
	glUniform1i(glGetUniformLocation(id, "myTexture"), 0);
      } else {
	glUniform1i(glGetUniformLocation(id, "hasTexture"), 0);
      }
      glDrawElements(GL_TRIANGLES, cur_scene->counts[*ivao], GL_UNSIGNED_INT, 0);
      glBindVertexArray(0);
      glBindTexture(GL_TEXTURE_2D, 0);
    }
    (*ivao)++;
  }  
  for (n = 0; n < nd->mNumChildren; ++n) {
    sceneDrawVAOs(cur_scene, sc, nd->mChildren[n], ivao);
  }
  gl4duPopMatrix(); 
}

static int sceneNbMeshes(const struct aiScene *sc, const struct aiNode* nd, int subtotal) {
  int n = 0;
  subtotal += nd->mNumMeshes;
  for(n = 0; n < nd->mNumChildren; ++n)
    subtotal += sceneNbMeshes(sc, nd->mChildren[n], 0);
  return subtotal;
}

static int loadasset (scene_t * cur_scene) {
  cur_scene->scene = aiImportFile(cur_scene->filename, 
		       aiProcessPreset_TargetRealtime_MaxQuality |
		       aiProcess_CalcTangentSpace       |
		       aiProcess_Triangulate            |
		       aiProcess_JoinIdenticalVertices  |
		       aiProcess_SortByPType);
  if (cur_scene->scene) {
    get_bounding_box(cur_scene, &(cur_scene->scene_min), &(cur_scene->scene_max));
    cur_scene->scene_center.x = (cur_scene->scene_min.x + cur_scene->scene_max.x) / 2.0f;
    cur_scene->scene_center.y = (cur_scene->scene_min.y + cur_scene->scene_max.y) / 2.0f;
    cur_scene->scene_center.z = (cur_scene->scene_min.z + cur_scene->scene_max.z) / 2.0f;
    return 0;
  }
  return 1;
}

static void _scenes_init(void) {
  if(_scenes)
    _scenes_free();
  else
    atexit(_scenes_free);
  _scenes = calloc((_size_scenes = 8), sizeof * _scenes);
  assert(_scenes);
  _nb_scenes = 0;
}

static void _scenes_free(void) {
  if(_scenes) {
    int i;
    for(i = 0; i < _nb_scenes; ++i) {
      if(_scenes[i]) {
	assimpDeleteScene(i + 1);
	free(_scenes[i]);
	_scenes[i] = NULL;
      }
    }
    free(_scenes);
  }
  _scenes = NULL;
  _nb_scenes = 0;
  _size_scenes = 0;
}

static scene_t * _scenes_from_id(GLuint id_scene) {
  GLuint id = id_scene - 1;
  if(id < _nb_scenes) {
    return _scenes[id];
  }
  return NULL;
}

static GLuint _scenes_add(const char * filename) {
  if((_nb_scenes + 1) == _size_scenes) {
    GLuint os = _size_scenes, i;
    scene_t ** ptr = realloc(_scenes, (_size_scenes *= 2) * sizeof *ptr);
    if(ptr == NULL) {
      _scenes_free();
      assert(ptr);
      return 0;
    }
    _scenes = ptr;
    for(i = os; i < _size_scenes; ++i)
      _scenes[i] = NULL;
  }
  _scenes[_nb_scenes] = calloc(1, sizeof *(_scenes[_nb_scenes]));
  assert(_scenes[_nb_scenes]);
  _scenes[_nb_scenes]->filename = strdup(filename);
  return ++_nb_scenes;
}

