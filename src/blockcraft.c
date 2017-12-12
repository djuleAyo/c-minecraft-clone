#include <stdio.h>
#include <stdlib.h>
#include <GL/glut.h>

typedef char cubeFaces;
#define CUBE_FACE_NONE  0
#define CUBE_FACE_NORTH 1
#define CUBE_FACE_SOUTH 2
#define CUBE_FACE_UP    4
#define CUBE_FACE_DOWN  8
#define CUBE_FACE_EAST 16
#define CUBE_FACE_WEST 32

/*
void
drawCubeFaces(cubeFaces mask)
{
  
}
*/

int
main (int argc, char **argv)
{
  glutInit(&argc, argv);

  glutInitWindowSize(1000, 1000);
  glutInitWindowPosition(100, 100);
  glutCreateWindow("BlockCraft");


  glClearColor(0, 0, 0, 0);
  glClear(GL_COLOR_BUFFER_BIT);

  glutInitDisplayMode(GLUT_RGBA | GLUT_DOUBLE);
  
  glutSwapBuffers();
  
  glutMainLoop();
}
