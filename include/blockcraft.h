#pragma once

/*documenting type */
typedef char cubeFaces;
/*and valid values for the type are these: */
#define CUBE_FACE_NONE  0
#define CUBE_FACE_NORTH 1
#define CUBE_FACE_SOUTH 2
#define CUBE_FACE_UP    4
#define CUBE_FACE_DOWN  8
#define CUBE_FACE_EAST 16
#define CUBE_FACE_WEST 32
#define CUBE_FACE_ALL  63

typedef enum {
  BLOCK_TYPE_AIR = 0,
  BLOCK_TYPE_SOIL,
  BLOCK_TYPE_GRASS,
  BLOCK_TYPE_SAND,
  BLOCK_TYPE_STONE,
  BLOCK_TYPE_OAK_WOOD,
  BLOCK_TYPE_OAT_LEAF,
  BLOCK_TYPE_WATER,
  BLOCK_TYPE_LAVA
} blockType;

typedef enum {
  TEX_GRASS_TOP= 0,
  TEX_STONE= 1,
  TEX_SOIL= 2,
  TEX_GRASS_SIDE= 3,
  TEX_SAND= 18,
  TEX_OAK_SIDE= 20,
  TEX_OAK_TOP= 21,
  TEX_OAK_LEAF= 53,
  TEX_WATER= 205,
  TEX_LAVA= 256,
} texture;

/* Used for struct timeval from sys/time.h header */
#define timeDif(x, y) (x.tv_sec - y.tv_sec) * 1000000 + x.tv_usec - y.tv_usec

typedef enum {
  ACTION_MOV_FORWARD,
  ACTION_MOV_BACK,
  ACTION_MOV_LEFT,
  ACTION_MOV_RIGHT,
  ACTION_MOV_UP,
  ACTION_MOV_DOWN
} action;

/*documenting type*/
typedef struct {
  int x;
  int y;
  int z;
} worldCoords;
typedef int wCoordX;
typedef int wCoordY;
typedef int wCoordZ;

typedef worldCoords chunkCoords;
typedef int wChunkCoordsX;
typedef int wChunkCoordsY;
typedef int wChunkCoordsZ;

typedef struct {
  double x;
  double y;
  double z;
} vec3;

#define MAX_HEIGHT 256
#define CHUNK_DIM 16
#define SEA_LEVEL 40

typedef blockType chunk[CHUNK_DIM * CHUNK_DIM * MAX_HEIGHT];

/* most of the blocks are not visible, so we extract visible ones
   and describe them with given struct. 
*/
typedef struct{
  worldCoords coords;
  blockType type;
  short visibleFaces;
} visibleBlock;

typedef struct{
  visibleBlock *data;
  int volume;
  int length;
  blockType type;
} VBV;

typedef enum{
  BIOME_SEA,
  BIOME_FOREST,
  BIOME_DESERT
} biome;

#define INITIAL_VBV_VOLUME 256

void drawCubeFaces(wCoordX x, wCoordY y, wCoordZ z,  cubeFaces mask, blockType block);

VBV *VBVmake();
void VBVdel(VBV *v);
void VBVshrink(VBV *v);
void VBVexpand(VBV *v);
void VBVadd(VBV *v, wCoordX x, wCoordY y, wCoordZ z, blockType type, int faces);
void VBVprint(VBV *v);


/* checking if a block is in range of VBD
 */
int isCached(wCoordX x, wCoordY y, wCoordZ z);
blockType readCacheBlock(wCoordX x, wCoordY y, wCoordZ z);
/* make VBV representation of a chunk */
void chunkToVBV(wCoordX oX, wCoordY oY, wCoordZ oZ, chunk *c, VBV *v);

void BDinit();
void VBDinit();
void BDprint();
void drawVBD();
void drawBD();


void getAimVector();

void movForward();
void movBack();
void movLeft();
void movRight();
void movUp();
void movDown();
void callActions();

void loadTexPack(const char* filename);

void getFR();
void showRefreshRate(int value);

void
display();

void
keyboard(unsigned char c, int x, int y);
void
keyboardUp(unsigned char c, int x, int y);
void passiveMotion (int x, int y);
void reshape (int w, int h);


//BIOMES
biome getBiome(wCoordX x, wCoordZ z);
