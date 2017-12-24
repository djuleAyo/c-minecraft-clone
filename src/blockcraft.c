#include <stdio.h>
#include <stdlib.h>
#include <GL/glut.h>
#include <sys/time.h>
#include <math.h>
#include <assert.h>

#include "lodepng.h"
#include "perlin.h"
#include "keyStore.h"

typedef char cubeFaces;
#define CUBE_FACE_NONE  0
#define CUBE_FACE_NORTH 1
#define CUBE_FACE_SOUTH 2
#define CUBE_FACE_UP    4
#define CUBE_FACE_DOWN  8
#define CUBE_FACE_EAST 16
#define CUBE_FACE_WEST 32
#define CUBE_FACE_ALL  63


#define timeDif(x, y) (x.tv_sec - y.tv_sec) * 1000000 + x.tv_usec - y.tv_usec

typedef enum {
  ACTION_MOV_FORWARD,
  ACTION_MOV_BACK,
  ACTION_MOV_LEFT,
  ACTION_MOV_RIGHT,
  ACTION_MOV_UP,
  ACTION_MOV_DOWN
} action;

typedef enum {
  BLOCK_TYPE_AIR = 0,
  BLOCK_TYPE_SOIL,
  BLOCK_TYPE_SAND,
  BLOCK_TYPE_STONE,
} blockType;

typedef struct {
  int x;
  int y;
  int z;
} worldCoords;

typedef worldCoords chunkCoords;

typedef struct {
  double x;
  double y;
  double z;
} vec3;

const double PI = 3.14159265;

worldCoords testCubes[100];

vec3 pos;
vec3 aim;

double aimFi;
double aimTheta;

int mouseX;
int mouseY;

int windowW;
int windowH;

int refreshRate = 0;
double minFR = 0;
double maxFR = 0;
double avrFR = 0;

struct timeval frameStart;
struct timeval frameEnd;

double delta = .5;

//this is in chunks
int visibility = 10;

#define MAX_HEIGHT 256
#define CHUNK_DIM 16
//blockType testChunk[ 2 * visibility * 2 * visibility * 2 * visibility * CHUNK_DIM * CHUNK_DIM * CHUNK_DIM];

typedef blockType chunk[CHUNK_DIM * CHUNK_DIM * MAX_HEIGHT];

typedef struct{
  worldCoords coords;
  blockType type;
  short visibleFaces;
} visibleBlock;

typedef struct{
  visibleBlock *data;
  int volume;
  int length;
} VBV;

#define INITIAL_VBV_VOLUME 256


#define checkImageWidth 64
#define checkImageHeight 64
static GLubyte checkImage[checkImageHeight][checkImageWidth][4];

static GLuint texName;

void makeCheckImage(void)
{
   int i, j, c;
    
   for (i = 0; i < checkImageHeight; i++) {
      for (j = 0; j < checkImageWidth; j++) {
         c = ((((i&0x8)==0)^((j&0x8))==0))*255;
         checkImage[i][j][0] = (GLubyte) c;
         checkImage[i][j][1] = (GLubyte) c;
         checkImage[i][j][2] = (GLubyte) c;
         checkImage[i][j][3] = (GLubyte) 255;
      }
   }
}

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

void VBVadd(VBV *v, int x, int y, int z, blockType type, int faces)
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
    printf("%d\t%d\t%d\t\t%d\n",b->coords.x, b->coords.y, b->coords.z, b->visibleFaces);
  }
  
}

chunk *blockData; // since volume is visibility dependant wich can be changed runtime this must be dynamic
worldCoords bdwo; // block data world origin
worldCoords bdmo; // block data memory origin


int isCached(int x, int y, int z)
{
  if(x < bdwo.x || x >= bdwo.x + 2 * visibility * CHUNK_DIM) return 0;
  if(y < bdwo.y || y >= bdwo.y + 2 * visibility * CHUNK_DIM) return 0;
  if(z < bdwo.z || z >= bdwo.z + 2 * visibility * CHUNK_DIM) return 0;

  return 1;
}

blockType readCacheBlock(int x, int y, int z)
{
  worldCoords delta;
  delta.x = x - bdwo.x;
  delta.y = y - bdwo.y;
  delta.z = z - bdwo.z;

  /*
 printf("read cache block\n");

 printf("for x %d , y %d, z %d checking out %d %d chunk and %d %d %d block\n",
	x, y, z,
	delta.x / CHUNK_DIM, delta.z / CHUNK_DIM * 2 * visibility,
	delta.x % CHUNK_DIM,
	delta.y * CHUNK_DIM,
	(delta.z % CHUNK_DIM) * CHUNK_DIM * MAX_HEIGHT);
  */
 return (blockData[(delta.x / CHUNK_DIM) + (delta.z / CHUNK_DIM) * 2 * visibility])
    [(delta.x % CHUNK_DIM)
     + (delta.y) * CHUNK_DIM
     + (delta.z % CHUNK_DIM) * CHUNK_DIM * MAX_HEIGHT];
  
}

void chunkToVBV(int oX, int oY, int oZ, chunk *c, VBV *v)
{
  for(int i = 0; i < 16; i++)
    for(int j = 0; j < 256; j++)
      for(int k = 0; k < 16; k++){
	if(!(*c)[i + j * 16 + k * 16 * 256]) continue;
	short faces = 0;
	if(isCached(oX + i  - 1, j, k + oZ) && !readCacheBlock(oX + i - 1, j, k + oZ))
	  faces |= CUBE_FACE_WEST;
	if(isCached(oX + i  + 1, j, k + oZ) && !readCacheBlock(oX + i  + 1, j, k + oZ))
	  faces |= CUBE_FACE_EAST;
	if(isCached(i + oX, oY + j - 1, k + oZ) && !readCacheBlock(i + oX, oY + j - 1, k + oZ))
	  faces |= CUBE_FACE_DOWN;
	if(isCached(i + oX, j  + 1, k + oZ) && !readCacheBlock(i + oX, j + 1, k + oZ))
	  faces |= CUBE_FACE_UP;
	if(isCached(i + oX, j, oZ + k  - 1) && !readCacheBlock(i + oX, j, oZ + k  - 1))
	  faces |= CUBE_FACE_SOUTH;
	if(isCached(i + oX, j, oZ + k  + 1) && !readCacheBlock(i + oX, j, oZ + k  + 1))
	  faces |= CUBE_FACE_NORTH;

	if(faces){
	  VBVadd(v, oX + i, j + oY, k + oZ, BLOCK_TYPE_SOIL, faces);
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

VBV **vbd; // visible block data

void getAimVector()
{
  aim.x = cos(aimTheta) * cos(aimFi);
  aim.y = sin(aimTheta);
  aim.z = cos(aimTheta) * sin(aimFi);
}

void
drawCubeFaces(int x, int y, int z,  cubeFaces mask)
{
  // intended to draw any subset of cubes faces
  // o as origin. the origin of a cube is vert that takes
  // minimal set of vert coord values for given cube is positive quartespace
  // meaning the south-west-down corner
  if(!mask) return;

  if(mask & CUBE_FACE_UP) {
    glColor3f(.0, 1.0, .0);

    glBegin(GL_QUADS);
    glTexCoord2f(8 * 1.0 / 16, 4 * 1.0 / 16); glVertex3i(x, y + 1, z);
    glTexCoord2f(8 * 1.0 / 16, 5 * 1.0 / 16); glVertex3i(x + 1, y + 1, z);
    glTexCoord2f(9 * 1.0 / 16, 5 * 1.0 / 16); glVertex3i(x + 1, y + 1, z + 1);
    glTexCoord2f(9 * 1.0 / 16, 4 * 1.0 / 16); glVertex3i(x, y + 1, z + 1);
    glEnd();
  }
  
  if(mask & CUBE_FACE_DOWN) {
    glColor3f(1, 1, 1);
    
    glBegin(GL_QUADS);
    glTexCoord2f(0.0, 0.0); glVertex3i(x, y, z);
    glTexCoord2f(0.0, 1.0 / 16); glVertex3i(x + 1, y, z);
    glTexCoord2f(1.0 / 16, 1.0 / 16); glVertex3i(x + 1, y, z + 1);
    glTexCoord2f(1.0 / 16, 0.0); glVertex3i(x, y, z + 1);
    glEnd();
  }
  if(mask & CUBE_FACE_WEST) {
    glColor3f(1, 1, 1);
    
    glBegin(GL_QUADS);
    glTexCoord2f(3 * 1.0 / 16, 1.0 / 16); glVertex3i(x, y, z);
    glTexCoord2f(3 * 1.0 / 16, 0); glVertex3i(x, y + 1, z);
    glTexCoord2f(4 * 1.0 / 16, 0); glVertex3i(x, y + 1, z + 1);
    glTexCoord2f(4 * 1.0 / 16, 1.0 / 16); glVertex3i(x, y, z + 1);
    glEnd();
  }
  if(mask & CUBE_FACE_EAST) {
    glColor3f(.5, .5, .5);
    
    glBegin(GL_QUADS);
    glTexCoord2f(3 * 1.0 / 16, 1.0 / 16); glVertex3i(x + 1, y, z);
    glTexCoord2f(3 * 1.0 / 16, 0); glVertex3i(x + 1, y + 1, z);
    glTexCoord2f(4 * 1.0 / 16, 0); glVertex3i(x + 1, y + 1, z + 1);
    glTexCoord2f(4 * 1.0 / 16, 1.0 / 16); glVertex3i(x + 1, y, z + 1);
    glEnd();
  }
  if(mask & CUBE_FACE_NORTH) {
    glColor3f(3 * 1.0 / 16, 1.0 / 16, 3 * 1.0 / 16);
    
    glBegin(GL_QUADS);
    glTexCoord2f(3 * 1.0 / 16, 1.0 / 16); glVertex3i(x, y, z + 1);
    glTexCoord2f(4 * 1.0 / 16, 1.0 / 16); glVertex3i(x + 1, y, z + 1);
    glTexCoord2f(4 * 1.0 / 16, 0); glVertex3i(x + 1, y + 1, z + 1);
    glTexCoord2f(3 * 1.0 / 16, 0); glVertex3i(x, y + 1, z + 1);
    glEnd();
    }
  if(mask & CUBE_FACE_SOUTH) {
    glColor3f(1.0 / 16, 3 * 1.0 / 16, 1.0 / 16);
    
    glBegin(GL_QUADS);
    glTexCoord2f(4 * 1.0 / 16, 1.0 / 16); glVertex3i(x, y, z);
    glTexCoord2f(3 * 1.0 / 16, 1.0 / 16); glVertex3i(x + 1, y, z);
    glTexCoord2f(3 * 1.0 / 16, 0); glVertex3i(x + 1, y + 1, z);
    glTexCoord2f(4 * 1.0 / 16, 0); glVertex3i(x, y + 1, z);
    glEnd();
  }
  
}

void movForward()
{
  pos.x += delta * aim.x;
  pos.y += delta * aim.y;
  pos.z += delta * aim.z;
}
void movBack()
{
  pos.x -= delta * aim.x;
  pos.y -= delta * aim.y;
  pos.z -= delta * aim.z;
}
void movLeft()
{
  pos.x += delta * sin(aimFi);
  pos.z -= delta * cos(aimFi);
}
void movRight()
{
  pos.x -= delta * sin(aimFi);
  pos.z += delta * cos(aimFi);
}
void movUp()
{
  pos.y += delta;
}
void movDown()
{
  pos.y -= delta;
}

void callKeyActions()
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

void drawVBD()
{
  for(int i = 0; i < 2 * visibility; i++)
    for(int j = 0; j < 2 * visibility; j++){
      VBV *v = vbd[i + j * 2 * visibility];
      for(int x = 0; x < v->length; x++) {
	visibleBlock *b = &(v->data[x]);
	drawCubeFaces(b->coords.x, b->coords.y, b->coords.z, b->visibleFaces);
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
	    if(readCacheBlock(i * CHUNK_DIM  + x, y, j * CHUNK_DIM + z))
	      drawCubeFaces(i * CHUNK_DIM  + x, y, j * CHUNK_DIM + z, CUBE_FACE_ALL);      
	  }
}

unsigned texPackW;
unsigned texPackH;

static GLuint texPack;

void decodeOneStep(const char* filename)
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


void
display()
{
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

  assert(gettimeofday(&frameStart, NULL) == 0);

  glEnable(GL_TEXTURE_2D);

  glBindTexture(GL_TEXTURE_2D, texPack);
  glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_DECAL);
  

  
  //process user input
  getAimVector();
  callKeyActions();

  //drawBD();
  drawVBD();
  
  glLoadIdentity ();          
  gluLookAt (pos.x, pos.y, pos.z,
	     pos.x + aim.x,
	     pos.y + aim.y,
	     pos.z + aim.z,
	     0.0, 1.0, 0.0
	     );
  
  refreshRate++;

  assert(gettimeofday(&frameEnd, NULL) == 0);
  int frameRate = timeDif(frameEnd, frameStart);

  if(maxFR < frameRate) maxFR = frameRate;
  if(!minFR) minFR = frameRate;
  else if(minFR > frameRate) minFR = frameRate;
  avrFR = (avrFR * (refreshRate - 1) + frameRate) / refreshRate;

  glDisable(GL_TEXTURE_2D);
  
  glutSwapBuffers();
  glutPostRedisplay();
}

void
keyboard(unsigned char c, int x, int y)
{
  double delta = .1;

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
  gluPerspective(60, w / (float) h, 1, 200);
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

	  int n = (int)(pnoise2d(worldX / 70.0, worldZ/70.0,
				 .8, 6, 1) * 20 + 30 );

	  int n1 = (int)(pnoise2d(worldX / 30.0, worldZ/30.0,
				 .8, 6, 1) * 10 + 30 );
	  //printf("%d\t%d\t%d\n", worldX, worldZ, n);
	  
	  for(int y = 0; y < MAX_HEIGHT; y++){
	    if(y < n + n1){
	      double n3d = pnoise3d(worldX / 50.0, y / 50.0, worldZ / 50.0, 1, 1, 1);
	      //if(fabs(n3d) < 0.1){
	      //(blockData[i + j * 2 * visibility])[x + y * 16 + z * 16 * 256] = BLOCK_TYPE_AIR;
	      //}
	      //else
		(blockData[i + j * 2 * visibility])[x + y * 16 + z * 16 * 256] = BLOCK_TYPE_SOIL;
	    }
	    else 
	      (blockData[i + j * 2 * visibility])[x + y * 16 + z * 16 * 256] = BLOCK_TYPE_AIR;
	  }
	}
  printf("block data init finished\n");
}

void VBVinit()
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

int
main (int argc, char **argv)
{
  pos.x = 0;
  pos.y = 0;
  pos.z = 3;

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

  //BDprint();
  
  VBVinit();
  

  
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
  makeCheckImage();
  glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

  glGenTextures(1, &texName);
  glBindTexture(GL_TEXTURE_2D, texName);

  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, 
		  GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, 
		  GL_NEAREST);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, checkImageWidth, 
	       checkImageHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE, 
	       checkImage);
  printf("made check image\n");
  decodeOneStep("./media/texturePack.png");

  
  glutPassiveMotionFunc(passiveMotion);
  glutReshapeFunc(reshape);
  glutDisplayFunc(display);
  glutKeyboardFunc(keyboard);
  glutKeyboardUpFunc(keyboardUp);
  glutTimerFunc(1000, showRefreshRate, 0);

  glEnable(GL_DEPTH_TEST);

  glutMainLoop();
}
