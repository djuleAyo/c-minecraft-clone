#include <stdio.h>
#include <stdlib.h>
#include <GL/glut.h>
#include <sys/time.h>
#include <math.h>
#include <assert.h>

#include "keyStore.h"
//#include "SOIL.h"

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

double delta = .05;

//this is in chunks
int visibility = 10;

#define MAX_HEIGHT 256
#define CHUNK_DIM 16
//blockType testChunk[ 2 * visibility * 2 * visibility * 2 * visibility * CHUNK_DIM * CHUNK_DIM * CHUNK_DIM];

typedef blockType chunk[CHUNK_DIM * CHUNK_DIM * MAX_HEIGHT];
chunk testChunk;

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

void delVBV(VBV *v)
{
  free(v->data);
  free(v);
}

void shrinkVBV(VBV *v)
{
  if(v->volume == INITIAL_VBV_VOLUME) return;

  v->volume /= 2;
  v->length = v->length > v->volume ? v->volume : v->length;

  v->data = realloc(v->data, v->volume);

  assert(v->data);
}

void expandVBV(VBV *v)
{
  v->volume *= 2;
  v->data = realloc(v->data, v->volume);

  assert(v->data);
}

void VBVadd(VBV *v, int x, int y, int z, blockType type, int faces)
{
  if(v->length == v->volume)
    expandVBV(v);

  visibleBlock *b =
    &(v->data[v->length]);
  v->length++;

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
  for(int i = 0; i < v->length; i++){
    visibleBlock b = v->data[i];
    printf("%d,%d,%d %d\n", b.coords.x, b.coords.y, b.coords.z, b.visibleFaces);
  }
}

VBV *v;

chunk *blockData; // since volume is visibility dependant wich can be changed runtime this must be dynamic


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
    glColor3f(1.0, 1.0, 1.0);
    
    glBegin(GL_TRIANGLES);
    glVertex3i(x, y + 1, z);
    glVertex3i(x + 1, y + 1, z);
    glVertex3i(x + 1, y + 1, z + 1);

    glVertex3i(x, y + 1, z);
    glVertex3i(x + 1, y + 1, z + 1);
    glVertex3i(x, y + 1, z + 1);
    glEnd();
  }
  
  if(mask & CUBE_FACE_DOWN) {
    glColor3f(1.0, 1.0, 0.0);
    
    glBegin(GL_TRIANGLES);
    glVertex3i(x, y, z);
    glVertex3i(x + 1, y, z);
    glVertex3i(x + 1, y, z + 1);

    glVertex3i(x, y, z);
    glVertex3i(x + 1, y, z + 1);
    glVertex3i(x, y, z + 1);
    glEnd();
  }
  if(mask & CUBE_FACE_WEST) {
    glColor3f(1.0, 0.0, 0.0);
    
    glBegin(GL_TRIANGLES);
    glVertex3i(x, y, z);
    glVertex3i(x, y + 1, z);
    glVertex3i(x, y + 1, z + 1);

    glVertex3i(x, y, z);
    glVertex3i(x, y + 1, z + 1);
    glVertex3i(x, y, z + 1);
    glEnd();
  }
  if(mask & CUBE_FACE_EAST) {
    glColor3f(1.0, 0.0, 1.0);
    
    glBegin(GL_TRIANGLES);
    glVertex3i(x + 1, y, z);
    glVertex3i(x + 1, y + 1, z);
    glVertex3i(x + 1, y + 1, z + 1);

    glVertex3i(x + 1, y, z);
    glVertex3i(x + 1, y + 1, z + 1);
    glVertex3i(x + 1, y, z + 1);
    glEnd();
  }
  if(mask & CUBE_FACE_NORTH) {
    glColor3f(0.0, 1.0, 1.0);
    
    glBegin(GL_TRIANGLES);
    glVertex3i(x, y, z + 1);
    glVertex3i(x + 1, y, z + 1);
    glVertex3i(x + 1, y + 1, z + 1);

    glVertex3i(x, y, z + 1);
    glVertex3i(x + 1, y + 1, z + 1);
    glVertex3i(x, y + 1, z + 1);
    glEnd();
    }
  if(mask & CUBE_FACE_SOUTH) {
    glColor3f(0.0, 0.0, 1.0);
    
    glBegin(GL_TRIANGLES);
    glVertex3i(x, y, z);
    glVertex3i(x + 1, y, z);
    glVertex3i(x + 1, y + 1, z);

    glVertex3i(x, y, z);
    glVertex3i(x + 1, y + 1, z);
    glVertex3i(x, y + 1, z);
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

void
display()
{
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

  assert(gettimeofday(&frameStart, NULL) == 0);

  //process user input
  getAimVector();
  callKeyActions();
  
  for(int i = 0; i < v->length; i++){
    visibleBlock b = v->data[i];
    drawCubeFaces(b.coords.x, b.coords.y, b.coords.z, b.visibleFaces);
  }
   

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

  printf("%d %d\n", windowW, windowH);

  glutWarpPointer(w / 2, h / 2);
  aimFi = -PI / 2;
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

int
main (int argc, char **argv)
{
  
  /*
  GLuint tex_2d = SOIL_load_OGL_texture
    (
     "img.png",
     SOIL_LOAD_AUTO,
     SOIL_CREATE_NEW_ID,
     SOIL_FLAG_MIPMAPS | SOIL_FLAG_INVERT_Y | SOIL_FLAG_NTSC_SAFE_RGB | SOIL_FLAG_COMPRESS_TO_DXT
     );
	
  if( 0 == tex_2d )
    {
      printf( "SOIL loading error: '%s'\n", SOIL_last_result() );
    }
  */
  for(int x = 0; x < 16; x++){
    for(int y = 0; y < 16; y++){
      for(int z = 0; z < 16; z++){
	if(y % 2 == 0)
	  testChunk[x + y * 16 + z * 16 * 16] = BLOCK_TYPE_SOIL;
	else
	  testChunk[x + y * 16 + z * 16 * 16] = BLOCK_TYPE_AIR;
      }
    }
  }

  pos.x = 0;
  pos.y = 0;
  pos.z = 3;

  mouseX = 0;
  mouseY = 0;

  v = VBVmake();
  for(int i = 0; i < 16; i++)
    for(int j = 0; j < 16; j++)
      VBVadd(v, i, 2, j, BLOCK_TYPE_SOIL, CUBE_FACE_UP);
  printf("len %d\n", v->length);
  VBVprint(v);
  
  glutInit(&argc, argv);

  //set some states
  glutInitDisplayMode(GLUT_RGB | GLUT_DOUBLE | GLUT_DEPTH);
  glutSetKeyRepeat(GLUT_KEY_REPEAT_OFF);

  glutCreateWindow("BlockCraft");
  glutFullScreen();
  glutSetCursor(GLUT_CURSOR_NONE);
  glClearColor(0.25, 0.5, 0.8, 1);

  glutPassiveMotionFunc(passiveMotion);
  glutReshapeFunc(reshape);
  glutDisplayFunc(display);
  glutKeyboardFunc(keyboard);
  glutKeyboardUpFunc(keyboardUp);
  glutTimerFunc(1000, showRefreshRate, 0);

  glEnable(GL_DEPTH_TEST);

  glutMainLoop();
}
