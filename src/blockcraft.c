#include <stdio.h>
#include <stdlib.h>
#include <GL/glut.h>
#include <sys/time.h>

typedef char cubeFaces;
#define CUBE_FACE_NONE  0
#define CUBE_FACE_NORTH 1
#define CUBE_FACE_SOUTH 2
#define CUBE_FACE_UP    4
#define CUBE_FACE_DOWN  8
#define CUBE_FACE_EAST 16
#define CUBE_FACE_WEST 32


typedef struct {
  int x;
  int y;
  int z; 
} worldCoords;

typedef worldCoords chunkCoords;




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
    glColor3f(0.0, 0.0, 1.0);
    
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
    glColor3f(0.0, 0.0, 1.0);
    
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

  worldCoords c;
  c.x = 1;
  c.y = 1;
  c.z = 0;
  
  drawCubeFaces(&c, 63);

  glutSwapBuffers();

}

void
keyboard(unsigned char c, int x, int y)
{
  printf("keyboard\n");
}

void reshape (int w, int h)
{
   glViewport (0, 0, (GLsizei) w, (GLsizei) h); 
   glMatrixMode (GL_PROJECTION);
   glLoadIdentity ();
   glFrustum (-1.0, 1.0, -1.0, 1.0, 1.0, 20.0);
   glMatrixMode (GL_MODELVIEW);
}

int
main (int argc, char **argv)
{
  glutInit(&argc, argv);

  glutInitDisplayMode(GLUT_RGB | GLUT_DOUBLE | GLUT_DEPTH);
  
  glutInitWindowSize(1000, 1000);
  glutInitWindowPosition(100, 100);
  glutCreateWindow("BlockCraft");


  glClearColor(0, 0, 0, 0);

  glutDisplayFunc(display);
  glutReshapeFunc(reshape);
  glutKeyboardFunc(keyboard);

  glLoadIdentity ();             /* clear the matrix */
  /* viewing transformation  */
  gluLookAt (0.0, 0.0, 5.0, 0.0, 0.0, 0.0, 0.0, 1.0, 0.0);

  glEnable(GL_DEPTH_TEST);
  
  glutMainLoop();
}
