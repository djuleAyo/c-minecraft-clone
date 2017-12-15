#include <stdio.h>
#include <stdlib.h>
#include <GL/glut.h>
#include <sys/time.h>
#include <math.h>

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

worldCoords testCube;

vec3 pos;
vec3 aim;

double aimFi;
double aimTheta;

int mouseX;
int mouseY;

int windowW;
int windowH;

void getAimVector()
{
  aim.x = cos(aimTheta) * cos(aimFi);
  aim.y = sin(aimTheta);
  aim.z = cos(aimTheta) * sin(aimFi);
}

void
drawCubeFaces(worldCoords *o,  cubeFaces mask)
{
  // intended to draw any subset of cubes faces
  // o as origin. the origin of a cube is vert that takes
  // minimal set of vert coord values for given cube is positive quartespace
  // meaning the south-west-down corner
  if(!mask) return;
  if(mask ^ CUBE_FACE_UP) {
    glColor3f(1.0, 1.0, 1.0);
    
    glBegin(GL_TRIANGLES);
    glVertex3i(o->x, o->y + 1, o->z);
    glVertex3i(o->x + 1, o->y + 1, o->z);
    glVertex3i(o->x + 1, o->y + 1, o->z + 1);

    glVertex3i(o->x, o->y + 1, o->z);
    glVertex3i(o->x + 1, o->y + 1, o->z + 1);
    glVertex3i(o->x, o->y + 1, o->z + 1);
    glEnd();
  }
  
  if(mask ^ CUBE_FACE_DOWN) {
    glColor3f(1.0, 1.0, 0.0);
    
    glBegin(GL_TRIANGLES);
    glVertex3i(o->x, o->y, o->z);
    glVertex3i(o->x + 1, o->y, o->z);
    glVertex3i(o->x + 1, o->y, o->z + 1);

    glVertex3i(o->x, o->y, o->z);
    glVertex3i(o->x + 1, o->y, o->z + 1);
    glVertex3i(o->x, o->y, o->z + 1);
    glEnd();
  }
  if(mask ^ CUBE_FACE_WEST) {
    glColor3f(1.0, 0.0, 0.0);
    
    glBegin(GL_TRIANGLES);
    glVertex3i(o->x, o->y, o->z);
    glVertex3i(o->x, o->y + 1, o->z);
    glVertex3i(o->x, o->y + 1, o->z + 1);

    glVertex3i(o->x, o->y, o->z);
    glVertex3i(o->x, o->y + 1, o->z + 1);
    glVertex3i(o->x, o->y, o->z + 1);
    glEnd();
  }
  if(mask ^ CUBE_FACE_EAST) {
    glColor3f(1.0, 0.0, 1.0);
    
    glBegin(GL_TRIANGLES);
    glVertex3i(o->x + 1, o->y, o->z);
    glVertex3i(o->x + 1, o->y + 1, o->z);
    glVertex3i(o->x + 1, o->y + 1, o->z + 1);

    glVertex3i(o->x + 1, o->y, o->z);
    glVertex3i(o->x + 1, o->y + 1, o->z + 1);
    glVertex3i(o->x + 1, o->y, o->z + 1);
    glEnd();
  }
  if(mask ^ CUBE_FACE_NORTH) {
    glColor3f(0.0, 1.0, 1.0);
    
    glBegin(GL_TRIANGLES);
    glVertex3i(o->x, o->y, o->z + 1);
    glVertex3i(o->x + 1, o->y, o->z + 1);
    glVertex3i(o->x + 1, o->y + 1, o->z + 1);

    glVertex3i(o->x, o->y, o->z + 1);
    glVertex3i(o->x + 1, o->y + 1, o->z + 1);
    glVertex3i(o->x, o->y + 1, o->z + 1);
    glEnd();
    }
  if(mask ^ CUBE_FACE_SOUTH) {
    glColor3f(0.0, 0.0, 1.0);
    
    glBegin(GL_TRIANGLES);
    glVertex3i(o->x, o->y, o->z);
    glVertex3i(o->x + 1, o->y, o->z);
    glVertex3i(o->x + 1, o->y + 1, o->z);

    glVertex3i(o->x, o->y, o->z);
    glVertex3i(o->x + 1, o->y + 1, o->z);
    glVertex3i(o->x, o->y + 1, o->z);
    glEnd();
  }
  
}


void
display()
{

  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

  

  getAimVector();

  drawCubeFaces(&testCube, CUBE_FACE_ALL);


  glLoadIdentity ();          
  gluLookAt (pos.x, pos.y, pos.z,
	     pos.x + aim.x,
	     pos.y + aim.y,
	     pos.z + aim.z,
	     0.0, 1.0, 0.0
	     );
  

  
  glutSwapBuffers();
  glutPostRedisplay();
}

void
keyboard(unsigned char c, int x, int y)
{
  double delta = .1;

  press(c);
  keyTablePrint();

/*
  switch(c){

  case 'o':
    pos.x += delta * sin(aimFi);
    pos.z -= delta * cos(aimFi);
    break;
  case 'u':
    pos.x -= delta * sin(aimFi);
    pos.z += delta * cos(aimFi);
    break;
  case '.':
    pos.x += delta * aim.x;
    pos.y += delta * aim.y;
    pos.z += delta * aim.z;
    break;
  case 'e':
    pos.x -= delta * aim.x;
    pos.y -= delta * aim.y;
    pos.z -= delta * aim.z;
    break;
  case ' ':
    pos.y += delta;
    break;
  case 'j':
    pos.y -= delta;
    break;
  }
*/
}

void
keyboardUp(unsigned char c, int x, int y)
{
  release(c);
keyTablePrint(); 
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
  glFrustum (-1.0, 1.0, -1.0, 1.0, 1.0, 20.0);
  glMatrixMode (GL_MODELVIEW);

  printf("%d %d\n", windowW, windowH);

  glutWarpPointer(w / 2, h / 2);
  aimFi = -PI / 2;
  aimTheta = 0;

  getAimVector();
  
}

int
main (int argc, char **argv)
{

  testCube.x = 0;
  testCube.y = 0;
  testCube.z = 0;


  pos.x = 0;
  pos.y = 0;
  pos.z = 3;

  mouseX = 0;
  mouseY = 0;

  
  glutInit(&argc, argv);

  //set some states
  glutInitDisplayMode(GLUT_RGB | GLUT_DOUBLE | GLUT_DEPTH);
  glutSetKeyRepeat(GLUT_KEY_REPEAT_OFF);
  glClearColor(0, 0, 0, 0);

  glutCreateWindow("BlockCraft");
  glutFullScreen();
  glutSetCursor(GLUT_CURSOR_NONE);

  glutPassiveMotionFunc(passiveMotion);
  glutReshapeFunc(reshape);
  glutDisplayFunc(display);
  glutKeyboardFunc(keyboard);
glutKeyboardUpFunc(keyboardUp);

  glEnable(GL_DEPTH_TEST);

  glutMainLoop();
}
