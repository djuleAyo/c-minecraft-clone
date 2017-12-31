#include <stdio.h>
#include <stdlib.h>
#include <GL/glut.h>
#include <sys/time.h>
#include <math.h>
#include <assert.h>

#include "lodepng.h"
#include "perlin.h"
#include "keyStore.h"
#include "blockcraft.h"

const double PI = 3.14159265;

/* position of camera */
vec3 pos;

/* looking direction
   mouse input is added to these angles which are then computed to
   R^3 vector
*/
double aimFi;
double aimTheta;

vec3 aim;

/* current position of mouse in the window */
int mouseX;
int mouseY;

//
double mouseSensitivity = .5;

/* window height and width */
int windowW;
int windowH;

/* main game loop is called once for each frame
   if the function executes fast enough fps = rr where rr is refresh
   rate. So to benchmark above rr we compute the time needed for
   game loop execution. given values are in us - milisecs.
*/
int refreshRate = 0;
double minFR = 0;
double maxFR = 0;
double avrFR = 0;

/*these are used for frame rate computation and for
  animations as current time
*/
struct timeval frameStart;
struct timeval frameEnd;



/* measure given in chunks
   effects dimensions of terrain data structures
   effects projection far plane
*/
int visibility = 2;


chunk *blockData; // since volume is visibility dependant wich can be changed runtime this must be dynamic
worldCoords bdwo; // block data world origin
worldCoords bdmo; // block data memory origin

/*visible block data - structure for holding visible blocks - only 
  ones that get drawn
*/
VBV **vbd; 

/* minecraft texture pack
 */
static GLuint texPack;
unsigned texPackW;
unsigned texPackH;

int texCoords[] = {
  //order: u, d, n, e, s, w
  -1, -1, -1, -1, -1, -1,
  //BLOCK_TYPE_SOIL
  TEX_SOIL, TEX_SOIL, TEX_SOIL, TEX_SOIL, TEX_SOIL, TEX_SOIL,
  //BLOCK_TYPE_GRASS,
  TEX_GRASS_TOP, TEX_SOIL, TEX_GRASS_SIDE, TEX_GRASS_SIDE, TEX_GRASS_SIDE, TEX_GRASS_SIDE,
  //BLOCK_TYPE_SAND,
  TEX_SAND, TEX_SAND, TEX_SAND, TEX_SAND, TEX_SAND, TEX_SAND,
  //BLOCK_TYPE_STONE,
  TEX_STONE, TEX_STONE, TEX_STONE, TEX_STONE, TEX_STONE, TEX_STONE,
  //BLOCK_TYPE_OAK_WOOD,
  TEX_OAK_TOP, TEX_OAK_TOP, TEX_OAK_SIDE, TEX_OAK_SIDE, TEX_OAK_SIDE, TEX_OAK_SIDE,
  //BLOCK_TYPE_OAT_LEAF,
  TEX_OAK_LEAF, TEX_OAK_LEAF, TEX_OAK_LEAF, TEX_OAK_LEAF, TEX_OAK_LEAF, TEX_OAK_LEAF,
  //BLOCK_TYPE_WATER,
  TEX_WATER, TEX_WATER, TEX_WATER, TEX_WATER, TEX_WATER, TEX_WATER,
  //BLOCK_TYPE_LAVA
  TEX_LAVA, TEX_LAVA, TEX_LAVA, TEX_LAVA, TEX_LAVA, TEX_LAVA, 

};

/* display list id*/
int terrain;

#define tmpLen 10000000
GLint *tVerts;
GLfloat *tTex;

//terrain
int tVertsNum = 0;
int tTexNum = 0;

void VBVtoArrays()
{
  for(int i = 0; i < 2 * visibility; i++)
    for(int j = 0; j < 2 * visibility; j++){
      VBV *v = vbd[i + j * 2 * visibility];

      for(int k = 0; k < v->length; k++){
	int *tex = &texCoords[v->data[k].type * 6];
	printf("block type %d\n", v->data[k].type);
	
	int mask = v->data[k].visibleFaces;
	int x = v->data[k].coords.x;
	int y = v->data[k].coords.y;
	int z = v->data[k].coords.z;

	if(mask & CUBE_FACE_UP) {

	  int texX = tex[0] % 16;
	  int texY = tex[0] / 16;

	  printf("%d %d\n", texX, texY);
			       	  
	  tVerts[tVertsNum++] = x;
	  tVerts[tVertsNum++] = y + 1;
	  tVerts[tVertsNum++] = z;
	  
	  tTex[tTexNum++] = texX / (float) 16;
	  tTex[tTexNum++] = texY / (float) 16;

 	  tVerts[tVertsNum++] = x + 1;
	  tVerts[tVertsNum++] = y + 1;
	  tVerts[tVertsNum++] = z;
	  
	  tTex[tTexNum++] = texX / (float) 16;
	  tTex[tTexNum++] = (texY + 1) / (float) 16;

	  tVerts[tVertsNum++] = x + 1;
	  tVerts[tVertsNum++] = y + 1;
	  tVerts[tVertsNum++] = z + 1;
	  
	  tTex[tTexNum++] = (texX + 1) / (float) 16;
	  tTex[tTexNum++] = (texY + 1) / (float) 16;

	  tVerts[tVertsNum++] = x;
	  tVerts[tVertsNum++] = y + 1;
	  tVerts[tVertsNum++] = z + 1;
	  
	  tTex[tTexNum++] = (texX + 1) / (float) 16;
	  tTex[tTexNum++] = texY / (float) 16;
	 
	}
  
	if(mask & CUBE_FACE_DOWN) {
	  int texX = tex[1] % 16;
	  int texY = tex[1] / 16;
	  printf("%d %d\n", texX, texY);	  
    	  tVerts[tVertsNum++] = x;
	  tVerts[tVertsNum++] = y;
	  tVerts[tVertsNum++] = z;
	  
	  tTex[tTexNum++] = texX / (float) 16;
	  tTex[tTexNum++] = texY / (float) 16;

 	  tVerts[tVertsNum++] = x + 1;
	  tVerts[tVertsNum++] = y;
	  tVerts[tVertsNum++] = z;
	  
	  tTex[tTexNum++] = texX / (float) 16;
	  tTex[tTexNum++] = (texY + 1) / (float) 16;

	  tVerts[tVertsNum++] = x + 1;
	  tVerts[tVertsNum++] = y;
	  tVerts[tVertsNum++] = z + 1;
	  
	  tTex[tTexNum++] = (texX + 1) / (float) 16;
	  tTex[tTexNum++] = (texY + 1) / (float) 16;

	  tVerts[tVertsNum++] = x;
	  tVerts[tVertsNum++] = y;
	  tVerts[tVertsNum++] = z + 1;
	  
	  tTex[tTexNum++] = (texX + 1) / (float) 16;
	  tTex[tTexNum++] = texY / (float) 16;
	  
	}
	if(mask & CUBE_FACE_WEST) {
	  
	  int texX = tex[5] % 16;
	  int texY = tex[5] / 16;
	  printf("%d %d\n", texX, texY);
	  tVerts[tVertsNum++] = x;
	  tVerts[tVertsNum++] = y;
	  tVerts[tVertsNum++] = z;
	  
	  tTex[tTexNum++] = texX / (float) 16;
	  tTex[tTexNum++] = (texY + 1) / (float) 16;

 	  tVerts[tVertsNum++] = x;
	  tVerts[tVertsNum++] = y + 1;
	  tVerts[tVertsNum++] = z;
	  
	  tTex[tTexNum++] = texX / (float) 16;
	  tTex[tTexNum++] = texY / (float) 16;

	  tVerts[tVertsNum++] = x;
	  tVerts[tVertsNum++] = y + 1;
	  tVerts[tVertsNum++] = z + 1;
	  
	  tTex[tTexNum++] = (texX + 1) / (float) 16;
	  tTex[tTexNum++] = texY / (float) 16;

	  tVerts[tVertsNum++] = x;
	  tVerts[tVertsNum++] = y;
	  tVerts[tVertsNum++] = z + 1;
	  
	  tTex[tTexNum++] = (texX + 1) / (float) 16;
	  tTex[tTexNum++] = (texY + 1) / (float) 16;
	 
    	}
	if(mask & CUBE_FACE_EAST) {

	  int texX = tex[3] % 16;
	  int texY = tex[3] / 16;
	  printf("%d %d\n", texX, texY);
	  tVerts[tVertsNum++] = x + 1;
	  tVerts[tVertsNum++] = y;
	  tVerts[tVertsNum++] = z;
	  
	  tTex[tTexNum++] = texX / (float) 16;
	  tTex[tTexNum++] = (texY + 1) / (float) 16;

 	  tVerts[tVertsNum++] = x + 1;
	  tVerts[tVertsNum++] = y + 1;
	  tVerts[tVertsNum++] = z;
	  
	  tTex[tTexNum++] = texX / (float) 16;
	  tTex[tTexNum++] = texY / (float) 16;

	  tVerts[tVertsNum++] = x + 1;
	  tVerts[tVertsNum++] = y + 1;
	  tVerts[tVertsNum++] = z + 1;
	  
	  tTex[tTexNum++] = (texX + 1) / (float) 16;
	  tTex[tTexNum++] = texY / (float) 16;

	  tVerts[tVertsNum++] = x + 1;
	  tVerts[tVertsNum++] = y;
	  tVerts[tVertsNum++] = z + 1;
	  
	  tTex[tTexNum++] = (texX + 1) / (float) 16;
	  tTex[tTexNum++] = (texY + 1) / (float) 16;
	     
	}
	if(mask & CUBE_FACE_NORTH) {

	  int texX = tex[2] % 16;
	  int texY = tex[2] / 16;
	  printf("%d %d\n", texX, texY);
	  tVerts[tVertsNum++] = x;
	  tVerts[tVertsNum++] = y;
	  tVerts[tVertsNum++] = z + 1;
	  
	  tTex[tTexNum++] = texX / (float) 16;
	  tTex[tTexNum++] = (texY + 1) / (float) 16;

 	  tVerts[tVertsNum++] = x + 1;
	  tVerts[tVertsNum++] = y;
	  tVerts[tVertsNum++] = z + 1;
	  
	  tTex[tTexNum++] = (texX + 1) / (float) 16;
	  tTex[tTexNum++] = (texY + 1) / (float) 16;

	  tVerts[tVertsNum++] = x + 1;
	  tVerts[tVertsNum++] = y + 1;
	  tVerts[tVertsNum++] = z + 1;
	  
	  tTex[tTexNum++] = (texX + 1) / (float) 16;
	  tTex[tTexNum++] = texY / (float) 16;

	  tVerts[tVertsNum++] = x;
	  tVerts[tVertsNum++] = y + 1;
	  tVerts[tVertsNum++] = z + 1;
	  
	  tTex[tTexNum++] = texX / (float) 16;
	  tTex[tTexNum++] = texY / (float) 16;
	 
	}
	if(mask & CUBE_FACE_SOUTH) {

	  int texX = tex[4] % 16;
	  int texY = tex[4] / 16;
	  printf("%d %d\n", texX, texY);
	  tVerts[tVertsNum++] = x;
	  tVerts[tVertsNum++] = y;
	  tVerts[tVertsNum++] = z;
	  
	  tTex[tTexNum++] = (texX + 1) / (float) 16;
	  tTex[tTexNum++] = (texY + 1) / (float) 16;

 	  tVerts[tVertsNum++] = x + 1;
	  tVerts[tVertsNum++] = y;
	  tVerts[tVertsNum++] = z;
	  
	  tTex[tTexNum++] = texX / (float) 16;
	  tTex[tTexNum++] = (texY + 1) / (float) 16;

	  tVerts[tVertsNum++] = x + 1;
	  tVerts[tVertsNum++] = y + 1;
	  tVerts[tVertsNum++] = z;
	  
	  tTex[tTexNum++] = texX / (float) 16;
	  tTex[tTexNum++] = texY / (float) 16;

	  tVerts[tVertsNum++] = x;
	  tVerts[tVertsNum++] = y + 1;
	  tVerts[tVertsNum++] = z;
	  
	  tTex[tTexNum++] = (texX + 1) / (float) 16;
	  tTex[tTexNum++] = texY / (float) 16;
	 
	}
      }
    }   
}

int
main (int argc, char **argv)
{
  tVerts = malloc(sizeof(int) * tmpLen);
  tTex = malloc(sizeof(float) * tmpLen);
  assert(tVerts && tTex);
  
  pos.x = 0;
  pos.y = 50;
  pos.z = 0;

  mouseX = 0;
  mouseY = 0;

  bdwo.x = 0;
  bdwo.y = 0;
  bdwo.z = 0;



  //for since data for both sides of player is kept
  //since aim can be changed fast
  blockData = malloc(sizeof(chunk) * 4 * visibility * visibility);
  assert(blockData);

  BDinit();

  //  blockData[ 0 ] [ 1 + 16 + 256 * 16] = BLOCK_TYPE_SAND;
  
  VBDinit();
  VBVprint(vbd[0]);
  VBVtoArrays();
  printf("finished VBVto arrays\n");
  
  glutInit(&argc, argv);

  //set some states
  glutInitDisplayMode(GLUT_RGB | GLUT_DOUBLE | GLUT_DEPTH);
  glutSetKeyRepeat(GLUT_KEY_REPEAT_OFF);

  glutCreateWindow("BlockCraft");
  glutFullScreen();
  glutSetCursor(GLUT_CURSOR_NONE);
  glClearColor(0.25, 0.5, 0.8, 1);
  //glEnable(GL_CULL_FACE);
  //glCullFace(GL_BACK);

  loadTexPack("./media/texturePack.png");

  glEnable(GL_TEXTURE_2D);

  glBindTexture(GL_TEXTURE_2D, texPack);
  glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_DECAL);

  glEnableClientState(GL_VERTEX_ARRAY);
  glEnableClientState(GL_TEXTURE_COORD_ARRAY);

  glVertexPointer(3, GL_INT, 0, tVerts);
  glTexCoordPointer(2, GL_FLOAT, 0, tTex);

  glColor3f(0, 1, 0);

  terrain = glGenLists(1);
  glNewList(terrain, GL_COMPILE);
  drawVBD();
  glEndList();

  printf("passed the list\n");
  
  glutPassiveMotionFunc(passiveMotion);
  glutReshapeFunc(reshape);
  glutDisplayFunc(display);
  glutKeyboardFunc(keyboard);
  glutKeyboardUpFunc(keyboardUp);
  glutTimerFunc(1000, showRefreshRate, 0);

  glEnable(GL_DEPTH_TEST);

  glutMainLoop();
}


/*
  VBV stands for visible block vector
  these are main part of VBD - visible block data
  
  used for storing block types 
 */
VBV *VBVmake()
{
  VBV *new = malloc(sizeof(VBV));
  assert(new);

  new->data = malloc(sizeof(visibleBlock) * INITIAL_VBV_VOLUME);
  assert(new->data);
  new->volume = INITIAL_VBV_VOLUME;
  new->length = 0;
  
  return new;
}

void VBVdel(VBV *v)
{
  free(v->data);
  free(v);
}

void VBVshrink(VBV *v)
{
  if(v->volume == INITIAL_VBV_VOLUME) return;

  v->volume /= 2;
  v->length = v->length > v->volume ? v->volume : v->length;

  v->data = realloc(v->data, v->volume * sizeof(visibleBlock));

  assert(v->data);
}

void VBVexpand(VBV *v)
{
  v->volume *= 2;

  v->data =  realloc(v->data, v->volume * sizeof(visibleBlock));

  assert(v->data);
}

void VBVadd(VBV *v, wCoordX x, wCoordY y, wCoordZ z, blockType type, int faces)
{
  if(v->length == v->volume)
    VBVexpand(v);

  visibleBlock *b =
    &(v->data[v->length++]);

  b->coords.x = x;
  b->coords.y = y;
  b->coords.z = z;
  b->type = type;
  b->visibleFaces = faces;
}

void VBVprint(VBV *v)
{
  if(v->length == 0){
    printf("empty\n");
    return;
  }
  printf("!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n");
  for(int i = 0; i < v->length; i++){
    visibleBlock *b = &(v->data[i]);
    printf("%d\t%d\t%d\t\t%d\t\t%d\n", b->coords.x, b->coords.y, b->coords.z, b->visibleFaces, b->type);
  }
  
}



/* checking if a block is in range of VBD
 */
int isCached(wCoordX x, wCoordY y, wCoordZ z)
{
  if(x < bdwo.x || x >= bdwo.x + 2 * visibility * CHUNK_DIM) return 0;
  if(y < bdwo.y || y >= bdwo.y + 2 * visibility * CHUNK_DIM) return 0;
  if(z < bdwo.z || z >= bdwo.z + 2 * visibility * CHUNK_DIM) return 0;

  return 1;
}

blockType readCacheBlock(wCoordX x, wCoordY y, wCoordZ z)
{
  worldCoords delta;
  delta.x = x - bdwo.x;
  delta.y = y - bdwo.y;
  delta.z = z - bdwo.z;

 return (blockData[(delta.x / CHUNK_DIM) + (delta.z / CHUNK_DIM) * 2 * visibility])
    [(delta.x % CHUNK_DIM)
     + (delta.y) * CHUNK_DIM
     + (delta.z % CHUNK_DIM) * CHUNK_DIM * MAX_HEIGHT];
  
}

/* make VBV representation of a chunk */
void chunkToVBV(wCoordX oX, wCoordY oY, wCoordZ oZ, chunk *c, VBV *v)
{
  for(int i = 0; i < CHUNK_DIM; i++)
    for(int j = 0; j < MAX_HEIGHT; j++)
      for(int k = 0; k < CHUNK_DIM; k++){
	if(!(*c)[i + j * CHUNK_DIM + k * CHUNK_DIM * MAX_HEIGHT]) continue;
	short faces = 0;
	if(!isCached(oX + i  - 1, j, k + oZ) || !readCacheBlock(oX + i - 1, j, k + oZ))
	  faces |= CUBE_FACE_WEST;
	if(!isCached(oX + i  + 1, j, k + oZ) || !readCacheBlock(oX + i  + 1, j, k + oZ))
	  faces |= CUBE_FACE_EAST;
	if(!isCached(i + oX, oY + j - 1, k + oZ) || !readCacheBlock(i + oX, oY + j - 1, k + oZ))
	  faces |= CUBE_FACE_DOWN;
	if(!isCached(i + oX, j  + 1, k + oZ) || !readCacheBlock(i + oX, j + 1, k + oZ))
	  faces |= CUBE_FACE_UP;
	if(!isCached(i + oX, j, oZ + k  - 1) || !readCacheBlock(i + oX, j, oZ + k  - 1))
	  faces |= CUBE_FACE_SOUTH;
	if(!isCached(i + oX, j, oZ + k  + 1) || !readCacheBlock(i + oX, j, oZ + k  + 1))
	  faces |= CUBE_FACE_NORTH;

	if(faces){
	  VBVadd(v, oX + i, j + oY, k + oZ,
		 (*c)[i + j * CHUNK_DIM + k * CHUNK_DIM * MAX_HEIGHT]
		 , faces);
		
	}
      }
}

void BDprint()
{
  for(int x = bdwo.x; x < bdwo.x + 2 * visibility * CHUNK_DIM; x++){
    printf("\n"); 
    for(int z = bdwo.z; z < bdwo.z + 2 * visibility * CHUNK_DIM; z++){
      int h = 0;
      for(int y = 0; y < MAX_HEIGHT; y++){
	if(readCacheBlock(x, y, z)) h = y;
	else break;
      }
      printf(" %d ", h);
    }
  }
}

void
drawCubeFaces(wCoordX x, wCoordY y, wCoordZ z,  cubeFaces mask, blockType block)
{
  // intended to draw any subset of cubes faces
  // o as origin. the origin of a cube is vert that takes
  // minimal set of vert coord values for given cube in positive quarterspace
  // meaning the south-west-down corner
  if(!mask) return;
  int *tex = &texCoords[(int)block * 6];
  
  if(mask & CUBE_FACE_UP) {

    int texX = tex[0] % 16;
    int texY = tex[0] / 16;

    glBegin(GL_QUADS);
    glTexCoord2f(texX  / (float) 16, texY / (float) 16); glVertex3i(x, y + 1, z);
    glTexCoord2f(texX  / (float) 16, (texY + 1) / (float) 16); glVertex3i(x + 1, y + 1, z);
    glTexCoord2f((texX + 1) / (float) 16, (texY + 1) / (float) 16); glVertex3i(x + 1, y + 1, z + 1);
    glTexCoord2f((texX + 1) / (float) 16, texY / (float) 16); glVertex3i(x, y + 1, z + 1);
    glEnd();
  }
  
  if(mask & CUBE_FACE_DOWN) {
    //glColor3f(1, 1, 1);

    int texX = tex[1] % 16;
    int texY = tex[1] / 16;

    glBegin(GL_QUADS);
    glTexCoord2f(texX / (float) 16, texY / (float) 16 ); glVertex3i(x, y, z);
    glTexCoord2f(texX / (float) 16, (texY + 1) / (float) 16); glVertex3i(x + 1, y, z);
    glTexCoord2f((texX + 1) / (float) 16, (texY + 1) / (float) 16); glVertex3i(x + 1, y, z + 1);
    glTexCoord2f((texX + 1) / (float) 16, texY / (float) 16); glVertex3i(x, y, z + 1);
    glEnd();
  }
  if(mask & CUBE_FACE_WEST) {
    //glColor3f(1, 1, 1);

    int texX = tex[5] % 16;
    int texY = tex[5] / 16;

    glBegin(GL_QUADS);
    glTexCoord2f((texX) / (float) 16, (texY + 1) / (float) 16); glVertex3i(x, y, z);
    glTexCoord2f(texX / (float) 16, texY / (float) 16); glVertex3i(x, y + 1, z);
    glTexCoord2f((texX + 1) / (float) 16, (texY) / (float) 16); glVertex3i(x, y + 1, z + 1);
    glTexCoord2f((texX + 1) / (float) 16, (texY + 1) / (float) 16); glVertex3i(x, y, z + 1);
    glEnd();
  }
  if(mask & CUBE_FACE_EAST) {
    //glColor3f(.5, .5, .5);

    int texX = tex[3] % 16;
    int texY = tex[3] / 16;

    glBegin(GL_QUADS);
    glTexCoord2f((texX) / (float) 16, (texY + 1) / (float) 16); glVertex3i(x + 1, y, z);
    glTexCoord2f((texX) / (float) 16, (texY) / (float) 16); glVertex3i(x + 1, y + 1, z);
    glTexCoord2f((texX + 1) / (float) 16, (texY) / (float) 16); glVertex3i(x + 1, y + 1, z + 1);
    glTexCoord2f((texX + 1) / (float) 16, (texY + 1) / (float) 16); glVertex3i(x + 1, y, z + 1);
    glEnd();
  }
  if(mask & CUBE_FACE_NORTH) {
    //glColor3f(3 * 1.0 / (float) 16, 1.0 / (float) 16, 3 * 1.0 / (float) 16);

    int texX = tex[2] % 16;
    int texY = tex[2] / 16;

    glBegin(GL_QUADS);
    glTexCoord2f((texX) / (float) 16, (texY + 1) / (float) 16); glVertex3i(x, y, z + 1);
    glTexCoord2f((texX + 1) / (float) 16, (texY + 1) / (float) 16); glVertex3i(x + 1, y, z + 1);
    glTexCoord2f((texX + 1) / (float) 16, (texY) / (float) 16); glVertex3i(x + 1, y + 1, z + 1);
    glTexCoord2f((texX) / (float) 16, (texY) / (float) 16); glVertex3i(x, y + 1, z + 1);
    glEnd();
    }
  if(mask & CUBE_FACE_SOUTH) {
    //glColor3f(1.0 / (float) 16, 3 * 1.0 / (float) 16, 1.0 / (float) 16);

    int texX = tex[4] % 16;
    int texY = tex[4] / 16;

    glBegin(GL_QUADS);
    glTexCoord2f((texX + 1) / (float) 16, (texY + 1) / (float) 16); glVertex3i(x, y, z);
    glTexCoord2f((texX) / (float) 16, (texY + 1) / (float) 16); glVertex3i(x + 1, y, z);
    glTexCoord2f((texX) / (float) 16, (texY) / (float) 16); glVertex3i(x + 1, y + 1, z);
    glTexCoord2f((texX + 1) / (float) 16, (texY) / (float) 16); glVertex3i(x, y + 1, z);
    glEnd();
  }
  
}
void drawVBD()
{
  for(int i = 0; i < 2 * visibility; i++)
    for(int j = 0; j < 2 * visibility; j++){
      VBV *v = vbd[i + j * 2 * visibility];
      for(int x = 0; x < v->length; x++) {
	visibleBlock *b = &(v->data[x]);
	drawCubeFaces(b->coords.x, b->coords.y, b->coords.z, b->visibleFaces, b->type);
	//drawCubeFaces(v->data[x].coords.x + i * CHUNK_DIM, v->data[x].coords.y, v->data[x].coords.z + j * CHUNK_DIM, v->data[x].visibleFaces);
      }
      
    }
}
void drawBD()
{
  for(int i = 0; i < 2 * visibility; i++)
    for(int j = 0; j < 2 * visibility; j++)
      for(int x = 0; x < CHUNK_DIM; x++)
	for(int y = 0; y < MAX_HEIGHT; y++)
	  for(int z = 0; z < CHUNK_DIM; z++){
	    blockType b = readCacheBlock(i * CHUNK_DIM  + x, y, j * CHUNK_DIM + z);
	    if(b)
	      drawCubeFaces(i * CHUNK_DIM  + x, y, j * CHUNK_DIM + z, CUBE_FACE_ALL, b);      
	  }
}

void getAimVector()
{
  aim.x = cos(aimTheta) * cos(aimFi);
  aim.y = sin(aimTheta);
  aim.z = cos(aimTheta) * sin(aimFi);
}

void movForward()
{
  pos.x += mouseSensitivity * aim.x;
  pos.y += mouseSensitivity * aim.y;
  pos.z += mouseSensitivity * aim.z;
}
void movBack()
{
  pos.x -= mouseSensitivity * aim.x;
  pos.y -= mouseSensitivity * aim.y;
  pos.z -= mouseSensitivity * aim.z;
}
void movLeft()
{
  pos.x += mouseSensitivity * sin(aimFi);
  pos.z -= mouseSensitivity * cos(aimFi);
}
void movRight()
{
  pos.x -= mouseSensitivity * sin(aimFi);
  pos.z += mouseSensitivity * cos(aimFi);
}
void movUp()
{
  pos.y += mouseSensitivity;
}
void movDown()
{
  pos.y -= mouseSensitivity;
}

void callActions()
{
  keyEnt *i = keyTable;

  while (i) {

    switch(i->c){
    case '.':
      movForward();
      break;
    case 'e':
      movBack();
      break;
    case 'o':
      movLeft();
      break;
    case 'u':
      movRight();
      break;
    case ' ':
      movUp();
      break;
    case 'j':
      movDown();
      break;

    }
    
    i = i->next;
  }
}


void loadTexPack(const char* filename)
{
  unsigned error;
  unsigned char* image;
  unsigned width, height;

  error = lodepng_decode32_file(&image, &texPackW, &texPackH, filename);
  if(error) printf("error %u: %s\n", error, lodepng_error_text(error));

  printf("loaded texture pack\n");


  glGenTextures(1, &texPack);
  glBindTexture(GL_TEXTURE_2D, texPack);

  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, 
		  GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, 
		  GL_NEAREST);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, texPackW, 
	       texPackH, 0, GL_RGBA, GL_UNSIGNED_BYTE, 
	       image);

  free(image);
}

void getFR()
{
  refreshRate++;

  assert(gettimeofday(&frameEnd, NULL) == 0);
  int frameRate = timeDif(frameEnd, frameStart);

  if(maxFR < frameRate) maxFR = frameRate;
  if(!minFR) minFR = frameRate;
  else if(minFR > frameRate) minFR = frameRate;
  avrFR = (avrFR * (refreshRate - 1) + frameRate) / refreshRate;

}

void
display()
{
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

  assert(gettimeofday(&frameStart, NULL) == 0);
  
  //process user input
  getAimVector();
  callActions();

  //drawBD();
  //drawVBD();
  //glCallList(terrain);

  glDrawArrays(GL_QUADS, 0, tVertsNum / 3);
  
  glLoadIdentity ();          
  gluLookAt (pos.x, pos.y, pos.z,
	     pos.x + aim.x,
	     pos.y + aim.y,
	     pos.z + aim.z,
	     0.0, 1.0, 0.0
	     );
  
  getFR();
   
  glutSwapBuffers();
  glutPostRedisplay();
}

void
keyboard(unsigned char c, int x, int y)
{
  press(c);
}

void
keyboardUp(unsigned char c, int x, int y)
{
  release(c);
}

void passiveMotion (int x, int y)
{
  if(x == windowW / 2 && y == windowH / 2){
    mouseX = x;
    mouseY = y;
    return;
  }
  
  double delta = .01;
  
  int deltaX = mouseX - x;
  mouseX = x;
  int deltaY = mouseY - y;
  mouseY = y;

  aimFi -= deltaX * delta;
  
  aimTheta += deltaY * delta;
  if(aimTheta > PI/ 2) aimTheta = PI / 2;
  if(aimTheta < - PI / 2) aimTheta = -PI / 2;

  glutWarpPointer(windowW / 2, windowH / 2);
  
  getAimVector();
}

void reshape (int w, int h)
{
  windowW = w;
  windowH = h;
  
  glViewport (0, 0, (GLsizei) w, (GLsizei) h); 
  glMatrixMode (GL_PROJECTION);
  glLoadIdentity ();
  gluPerspective(60, w / (float) h, 1, 5 * visibility * 16);
  glMatrixMode (GL_MODELVIEW);

  glutWarpPointer(w / 2, h / 2);
  aimFi = PI / 2;
  aimTheta = 0;

  getAimVector();
  
}

void showRefreshRate(int value)
{
  printf("rr %d\n", refreshRate);
  refreshRate = 0;

  printf("fr:\nmin: %f\nmax: %f\navr: %f\n\n", minFR, maxFR, avrFR);

  minFR = 0;
  maxFR = 0;
  avrFR = 0;
  
  glutTimerFunc(1000, showRefreshRate, 0);
}

void BDinit()
{
  printf("initing block data\n");
  for(int i = 0; i < 2 * visibility; i++)
    for(int j = 0; j < 2 * visibility; j++)
      for(int x = 0; x < CHUNK_DIM; x++)
	for(int z = 0; z < CHUNK_DIM; z++){
	  int worldX = bdwo.x + x + i * CHUNK_DIM ;
	  int worldZ = bdwo.z + z + j * CHUNK_DIM;


	  int surfice = (int)(pnoise2d(worldX / 70.0, worldZ/70.0,
				 .8, 6, 1) * 20 + 30 );

	  int n1 = (int)(pnoise2d(worldX / 30.0, worldZ/30.0,
				 .8, 6, 1) * 10 + 30 );

	  //printf("%d\t%d\t%d\n", worldX, worldZ, n);
	  
	  for(int y = 0; y < MAX_HEIGHT; y++){
	    
	    if(y < surfice /*+ n1*/){
	      //double n3d = pnoise3d(worldX / 50.0, y / 50.0, worldZ / 50.0, 1, 1, 1);
	      /*if(fabs(n3d) < 0.1){
		(blockData[i + j * 2 * visibility])[x + y * 16 + z * 16 * 256] = BLOCK_TYPE_AIR;
	      }
	      else
	      */
	      if(y < surfice - 3)
		(blockData[i + j * 2 * visibility])[x + y * CHUNK_DIM + z * CHUNK_DIM * MAX_HEIGHT] = BLOCK_TYPE_STONE;
	      else if(y < surfice - 1)
		(blockData[i + j * 2 * visibility])[x + y * CHUNK_DIM + z * CHUNK_DIM * MAX_HEIGHT] = BLOCK_TYPE_SOIL;
	      else
		(blockData[i + j * 2 * visibility])[x + y * CHUNK_DIM + z * CHUNK_DIM * MAX_HEIGHT] = BLOCK_TYPE_GRASS;
	    }
	    else 
	      (blockData[i + j * 2 * visibility])[x + y * CHUNK_DIM + z * CHUNK_DIM * MAX_HEIGHT] = BLOCK_TYPE_AIR;
	  }
	}
  printf("block data init finished\n");
}

void VBDinit()
{
  printf("initing vbd\n");
  vbd = malloc (sizeof(VBV*) * 4 * visibility * visibility);
  for(int i = 0; i < 4 * visibility * visibility; i++){
    vbd[i] = VBVmake();
  }
  assert(vbd);

  //init vbd data
  for(int i = 0; i < 2 * visibility; i++)
    for(int j = 0; j < 2 * visibility; j++){
      chunkToVBV(bdwo.x + i * CHUNK_DIM , 0, bdwo.z + j * CHUNK_DIM,
		 &blockData[i + j * 2 * visibility], vbd[i + j * 2 * visibility]);
      //printf("visible blocks in %d %d: %d\n", i, j, vbd[i + j * 2 * visibility]->length);
    }
  printf("init vbd finished\n");

}

