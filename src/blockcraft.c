#include <stdio.h>
#include <stdlib.h>
#include <GL/glut.h>
#include <sys/time.h>
#include <math.h>
#include <assert.h>

//library for loading png files into memory
#include "lodepng.h"
//library for noise functions - perlin 2d and 3d
#include "perlin.h"
//key funtionality is suposed to be seperate from other code - as a module
//since a header
#include "keyStore.h"
//forwards for this file
#include "blockcraft.h"

//needed for computation of movement and aim vector
static const double PI = 3.14159265;

/* position of camera */
static vec3 pos;

/* looking direction
   mouse input is added to these angles which are then computed to
   R^3 vector named aim
*/
static double aimFi;
static double aimTheta;
static vec3 aim;

/* current position of mouse in the window */
static int mouseX;
static int mouseY;

//configurable mouse sens.- its parameter
static double mouseSensitivity = .5;

/* window height and width */
static int windowW;
static int windowH;

/* main game loop is called once for each frame
   if the function executes fast enough fps = rr where rr is refresh
   rate. So to benchmark above rr we compute the time needed for
   game loop execution. given values are in us - milisecs.
*/
static int refreshRate = 0;
static double minFR = 0;
static double maxFR = 0;
static double avrFR = 0;

/*these are used for frame rate computation and for
  animations based on time passed
*/
static struct timeval frameStart;
static struct timeval frameEnd;



/* measure given in chunks
   effects dimensions of terrain data structures
   effects projection far plane
   visibility is half of the memory reserved
   memory is reserved for 2 * visibility square 
   around player
*/
static int visibility = 10;


static chunk *blockData; // since volume is visibility dependant wich can be changed runtime this must be dynamic
static worldCoords bdwo; // block data world origin - tells where is first block in block data in the world
static worldCoords bdmo; // block data memory origin - first block might be on 1st mem positon but might not
//if not then modulus calculations are used to transform coords in between each
//all this for live world loading
//this way we wouldnt rewrite whole data structure on chunk setup change but only 2 rows/colums
//and update dbmo acordingly

/*visible block data - structure for holding visible blocks - only 
  ones that get drawn
*/
static VBV **vbd; 

/* minecraft texture pack
 */
static GLuint texPack;
static unsigned texPackW;
static unsigned texPackH;

//this is container for block faces tex- meaning 1 block can have different faces
//and it is indexed by blockType enum
static int texCoords[] = {
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
  //BLOCK_TYPE_SNOW
  TEX_SNOW, TEX_SNOW_SIDE, TEX_SNOW_SIDE, TEX_SNOW_SIDE, TEX_SNOW_SIDE, TEX_SNOW_SIDE,
  //BLOCK_TYPE_CLOUD
  TEX_SNOW, TEX_SNOW, TEX_SNOW, TEX_SNOW, TEX_SNOW, TEX_SNOW,
};

/* display list id*/
//since i tryed different methods for drawing to see which one is most quick there is a display list as well
//this remains here for systems where this might be faster, but is currently not used
int terrain;

//arrays that contain vertex coords and tex coords arent realloced-simply not implemented- so lets make sure
//we have enough mem. if seg fault fault on large visibility - this might be the cause
//all following arrays use given size
#define tmpLen 10000000
static GLint **tVerts;
static GLfloat **tTex;
static GLint **wVerts;
static GLfloat **wTex;

//counters for arrays defined above- used by drawing procedures of gl
int tVertsNum = 0;
int tTexNum = 0;
int wVertsNum = 0;
int wTexNum = 0;

//vert stats - 5 ints per chunk in this order: tVertsNum, tTexNum, wVertsNum, wTexNum, isEdited
int *chunkStats;

//define 3 dimenions for space in which tree is drawn- in blocks
#define TREE_MODEL_X 5
#define TREE_MODEL_Y 7
#define TREE_MODEL_Z 5

//here we will store for now the only three model
static int treeModel [ TREE_MODEL_X * TREE_MODEL_Y * TREE_MODEL_Z];

//these are coords of block that player is aiming at
wCoordX atAimX;
wCoordY atAimY;
wCoordZ atAimZ;
//flag if there is any block in given aim range
int hasAim;


//fills vert and tex arrays based on data in VBV
void VBVtoArray(int i, int j)
{

  //for every block in VBV for given chunk on i j
  //copy vert and tex data in apropriate array
  //check if water or not since water goes in seperate arrays
  //this is cause of different drawing params

  
  VBV *v = vbd[i + j * 2 * visibility];

  GLint *curTVerts = tVerts[i + j * 2 * visibility];
  GLfloat *curTTex = tTex[i + j * 2 * visibility];

  GLint *curWVerts = wVerts[i + j * 2 * visibility];
  GLfloat *curWTex = wTex[i + j * 2 * visibility];

  int *curTVertsNum = &chunkStats[(i + j * 2 * visibility) * 5];
  int *curTTexNum = &chunkStats[(i + j * 2 * visibility) * 5 + 1];
  int *curWVertsNum = &chunkStats[(i + j * 2 * visibility) * 5 + 2];
  int *curWTexNum = &chunkStats[(i + j * 2 * visibility) * 5 + 3];


  for(int k = 0; k < v->length; k++){
    int *tex = &texCoords[v->data[k].type * 6];
	
    int mask = v->data[k].visibleFaces;
    int x = v->data[k].coords.x;
    int y = v->data[k].coords.y;
    int z = v->data[k].coords.z;

    if(v->data[k].type != BLOCK_TYPE_WATER){
      if(mask & CUBE_FACE_UP) {

	int texX = tex[0] % 16;
	int texY = tex[0] / 16;

	curTVerts[*curTVertsNum] = x;
	*curTVertsNum += 1;
	curTVerts[*curTVertsNum] = y + 1;
	*curTVertsNum += 1;
	curTVerts[*curTVertsNum] = z;
	*curTVertsNum += 1;

	curTTex[*curTTexNum] = texX / (float) 16;
	*curTTexNum += 1;
	curTTex[*curTTexNum] = texY / (float) 16;
	*curTTexNum += 1;

	curTVerts[*curTVertsNum] = x + 1;
	*curTVertsNum += 1;
	curTVerts[*curTVertsNum] = y + 1;
	*curTVertsNum += 1;
	curTVerts[*curTVertsNum] = z;
	*curTVertsNum += 1;
	  
	curTTex[*curTTexNum] = texX / (float) 16;
	*curTTexNum += 1;
	curTTex[*curTTexNum] = (texY + 1) / (float) 16;
	*curTTexNum += 1;

	curTVerts[*curTVertsNum] = x + 1;
	*curTVertsNum += 1;
	curTVerts[*curTVertsNum] = y + 1;
	*curTVertsNum += 1;
	curTVerts[*curTVertsNum] = z + 1;
	*curTVertsNum += 1;
	  
	curTTex[*curTTexNum] = (texX + 1) / (float) 16;
	*curTTexNum += 1;
	curTTex[*curTTexNum] = (texY + 1) / (float) 16;
	*curTTexNum += 1;

	curTVerts[*curTVertsNum] = x;
	*curTVertsNum += 1;
	curTVerts[*curTVertsNum] = y + 1;
	*curTVertsNum += 1;
	curTVerts[*curTVertsNum] = z + 1;
	*curTVertsNum += 1;
	  
	curTTex[*curTTexNum] = (texX + 1) / (float) 16;
	*curTTexNum += 1;
	curTTex[*curTTexNum] = texY / (float) 16;
	*curTTexNum += 1;
      }
  
      if(mask & CUBE_FACE_DOWN) {

	int texX = tex[1] % 16;
	int texY = tex[1] / 16;
	curTVerts[*curTVertsNum] = x;
	*curTVertsNum += 1;
	curTVerts[*curTVertsNum] = y;
	*curTVertsNum += 1;
	curTVerts[*curTVertsNum] = z;
	*curTVertsNum += 1;

	curTTex[*curTTexNum] = texX / (float) 16;
	*curTTexNum += 1;
	curTTex[*curTTexNum] = texY / (float) 16;
	*curTTexNum += 1;

	curTVerts[*curTVertsNum] = x + 1;
	*curTVertsNum += 1;
	curTVerts[*curTVertsNum] = y;
	*curTVertsNum += 1;
	curTVerts[*curTVertsNum] = z;
	*curTVertsNum += 1;
	  
	curTTex[*curTTexNum] = texX / (float) 16;
	*curTTexNum += 1;
	curTTex[*curTTexNum] = (texY + 1) / (float) 16;
	*curTTexNum += 1;

	curTVerts[*curTVertsNum] = x + 1;
	*curTVertsNum += 1;
	curTVerts[*curTVertsNum] = y;
	*curTVertsNum += 1;
	curTVerts[*curTVertsNum] = z + 1;
	*curTVertsNum += 1;
	  
	curTTex[*curTTexNum] = (texX + 1) / (float) 16;
	*curTTexNum += 1;
	curTTex[*curTTexNum] = (texY + 1) / (float) 16;
	*curTTexNum += 1;

	curTVerts[*curTVertsNum] = x;
	*curTVertsNum += 1;
	curTVerts[*curTVertsNum] = y;
	*curTVertsNum += 1;
	curTVerts[*curTVertsNum] = z + 1;
	*curTVertsNum += 1;
	  
	curTTex[*curTTexNum] = (texX + 1) / (float) 16;
	*curTTexNum += 1;
	curTTex[*curTTexNum] = texY / (float) 16;
	*curTTexNum += 1;
	  
      }
      if(mask & CUBE_FACE_WEST) {
	  
	int texX = tex[5] % 16;
	int texY = tex[5] / 16;

	curTVerts[*curTVertsNum] = x;
	*curTVertsNum += 1;
	curTVerts[*curTVertsNum] = y;
	*curTVertsNum += 1;
	curTVerts[*curTVertsNum] = z;
	*curTVertsNum += 1;
	  
	curTTex[*curTTexNum] = texX / (float) 16;
	*curTTexNum += 1;
	curTTex[*curTTexNum] = (texY + 1) / (float) 16;
	*curTTexNum += 1;

	curTVerts[*curTVertsNum] = x;
	*curTVertsNum += 1;
	curTVerts[*curTVertsNum] = y + 1;
	*curTVertsNum += 1;
	curTVerts[*curTVertsNum] = z;
	*curTVertsNum += 1;
	  
	curTTex[*curTTexNum] = texX / (float) 16;
	*curTTexNum += 1;
	curTTex[*curTTexNum] = texY / (float) 16;
	*curTTexNum += 1;

	curTVerts[*curTVertsNum] = x;
	*curTVertsNum += 1;
	curTVerts[*curTVertsNum] = y + 1;
	*curTVertsNum += 1;
	curTVerts[*curTVertsNum] = z + 1;
	*curTVertsNum += 1;
	  
	curTTex[*curTTexNum] = (texX + 1) / (float) 16;
	*curTTexNum += 1;
	curTTex[*curTTexNum] = texY / (float) 16;
	*curTTexNum += 1;

	curTVerts[*curTVertsNum] = x;
	*curTVertsNum += 1;
	curTVerts[*curTVertsNum] = y;
	*curTVertsNum += 1;
	curTVerts[*curTVertsNum] = z + 1;
	*curTVertsNum += 1;
	  
	curTTex[*curTTexNum] = (texX + 1) / (float) 16;
	*curTTexNum += 1;
	curTTex[*curTTexNum] = (texY + 1) / (float) 16;
	*curTTexNum += 1;
	 
      }
      if(mask & CUBE_FACE_EAST) {

	int texX = tex[3] % 16;
	int texY = tex[3] / 16;

	curTVerts[*curTVertsNum] = x + 1;
	*curTVertsNum += 1;
	curTVerts[*curTVertsNum] = y;
	*curTVertsNum += 1;
	curTVerts[*curTVertsNum] = z;
	*curTVertsNum += 1;
	  
	curTTex[*curTTexNum] = texX / (float) 16;
	*curTTexNum += 1;
	curTTex[*curTTexNum] = (texY + 1) / (float) 16;
	*curTTexNum += 1;

	curTVerts[*curTVertsNum] = x + 1;
	*curTVertsNum += 1;
	curTVerts[*curTVertsNum] = y + 1;
	*curTVertsNum += 1;
	curTVerts[*curTVertsNum] = z;
	*curTVertsNum += 1;
	  
	curTTex[*curTTexNum] = texX / (float) 16;
	*curTTexNum += 1;
	curTTex[*curTTexNum] = texY / (float) 16;
	*curTTexNum += 1;

	curTVerts[*curTVertsNum] = x + 1;
	*curTVertsNum += 1;
	curTVerts[*curTVertsNum] = y + 1;
	*curTVertsNum += 1;
	curTVerts[*curTVertsNum] = z + 1;
	*curTVertsNum += 1;
	  
	curTTex[*curTTexNum] = (texX + 1) / (float) 16;
	*curTTexNum += 1;
	curTTex[*curTTexNum] = texY / (float) 16;
	*curTTexNum += 1;

	curTVerts[*curTVertsNum] = x + 1;
	*curTVertsNum += 1;
	curTVerts[*curTVertsNum] = y;
	*curTVertsNum += 1;
	curTVerts[*curTVertsNum] = z + 1;
	*curTVertsNum += 1;
	  
	curTTex[*curTTexNum] = (texX + 1) / (float) 16;
	*curTTexNum += 1;
	curTTex[*curTTexNum] = (texY + 1) / (float) 16;
	*curTTexNum += 1;
	     
      }
      if(mask & CUBE_FACE_NORTH) {

	int texX = tex[2] % 16;
	int texY = tex[2] / 16;

	curTVerts[*curTVertsNum] = x;
	*curTVertsNum += 1;
	curTVerts[*curTVertsNum] = y;
	*curTVertsNum += 1;
	curTVerts[*curTVertsNum] = z + 1;
	*curTVertsNum += 1;
	  
	curTTex[*curTTexNum] = texX / (float) 16;
	*curTTexNum += 1;
	curTTex[*curTTexNum] = (texY + 1) / (float) 16;
	*curTTexNum += 1;

	curTVerts[*curTVertsNum] = x + 1;
	*curTVertsNum += 1;
	curTVerts[*curTVertsNum] = y;
	*curTVertsNum += 1;
	curTVerts[*curTVertsNum] = z + 1;
	*curTVertsNum += 1;
	  
	curTTex[*curTTexNum] = (texX + 1) / (float) 16;
	*curTTexNum += 1;
	curTTex[*curTTexNum] = (texY + 1) / (float) 16;
	*curTTexNum += 1;

	curTVerts[*curTVertsNum] = x + 1;
	*curTVertsNum += 1;
	curTVerts[*curTVertsNum] = y + 1;
	*curTVertsNum += 1;
	curTVerts[*curTVertsNum] = z + 1;
	*curTVertsNum += 1;
	  
	curTTex[*curTTexNum] = (texX + 1) / (float) 16;
	*curTTexNum += 1;
	curTTex[*curTTexNum] = texY / (float) 16;
	*curTTexNum += 1;

	curTVerts[*curTVertsNum] = x;
	*curTVertsNum += 1;
	curTVerts[*curTVertsNum] = y + 1;
	*curTVertsNum += 1;
	curTVerts[*curTVertsNum] = z + 1;
	*curTVertsNum += 1;
	  
	curTTex[*curTTexNum] = texX / (float) 16;
	*curTTexNum += 1;
	curTTex[*curTTexNum] = texY / (float) 16;
	*curTTexNum += 1;
	 
      }
      if(mask & CUBE_FACE_SOUTH) {

	int texX = tex[4] % 16;
	int texY = tex[4] / 16;

	curTVerts[*curTVertsNum] = x;
	*curTVertsNum += 1;
	curTVerts[*curTVertsNum] = y;
	*curTVertsNum += 1;
	curTVerts[*curTVertsNum] = z;
	*curTVertsNum += 1;
	  
	curTTex[*curTTexNum] = (texX + 1) / (float) 16;
	*curTTexNum += 1;
	curTTex[*curTTexNum] = (texY + 1) / (float) 16;
	*curTTexNum += 1;

	curTVerts[*curTVertsNum] = x + 1;
	*curTVertsNum += 1;
	curTVerts[*curTVertsNum] = y;
	*curTVertsNum += 1;
	curTVerts[*curTVertsNum] = z;
	*curTVertsNum += 1;
	  
	curTTex[*curTTexNum] = texX / (float) 16;
	*curTTexNum += 1;
	curTTex[*curTTexNum] = (texY + 1) / (float) 16;
	*curTTexNum += 1;

	curTVerts[*curTVertsNum] = x + 1;
	*curTVertsNum += 1;
	curTVerts[*curTVertsNum] = y + 1;
	*curTVertsNum += 1;
	curTVerts[*curTVertsNum] = z;
	*curTVertsNum += 1;
	  
	curTTex[*curTTexNum] = texX / (float) 16;
	*curTTexNum += 1;
	curTTex[*curTTexNum] = texY / (float) 16;
	*curTTexNum += 1;

	curTVerts[*curTVertsNum] = x;
	*curTVertsNum += 1;
	curTVerts[*curTVertsNum] = y + 1;
	*curTVertsNum += 1;
	curTVerts[*curTVertsNum] = z;
	*curTVertsNum += 1;
	  
	curTTex[*curTTexNum] = (texX + 1) / (float) 16;
	*curTTexNum += 1;
	curTTex[*curTTexNum] = texY / (float) 16;
	*curTTexNum += 1;
	 
      }
    } else {
      if(mask & CUBE_FACE_UP) {

	int texX = tex[0] % 16;
	int texY = tex[0] / 16;

	curWVerts[*curWVertsNum] = x;
	*curWVertsNum += 1;
	curWVerts[*curWVertsNum] = y + 1;
	*curWVertsNum += 1;
	curWVerts[*curWVertsNum] = z;
	*curWVertsNum += 1;
	  
	curWTex[*curWTexNum] = texX / (float) 16;
	*curWTexNum += 1;
	curWTex[*curWTexNum] = texY / (float) 16;
	*curWTexNum += 1;

	curWVerts[*curWVertsNum] = x + 1;
	*curWVertsNum += 1;
	curWVerts[*curWVertsNum] = y + 1;
	*curWVertsNum += 1;
	curWVerts[*curWVertsNum] = z;
	*curWVertsNum += 1;
	  
	curWTex[*curWTexNum] = texX / (float) 16;
	*curWTexNum += 1;
	curWTex[*curWTexNum] = (texY + 1) / (float) 16;
	*curWTexNum += 1;

	curWVerts[*curWVertsNum] = x + 1;
	*curWVertsNum += 1;
	curWVerts[*curWVertsNum] = y + 1;
	*curWVertsNum += 1;
	curWVerts[*curWVertsNum] = z + 1;
	*curWVertsNum += 1;
	  
	curWTex[*curWTexNum] = (texX + 1) / (float) 16;
	*curWTexNum += 1;
	curWTex[*curWTexNum] = (texY + 1) / (float) 16;
	*curWTexNum += 1;

	curWVerts[*curWVertsNum] = x;
	*curWVertsNum += 1;
	curWVerts[*curWVertsNum] = y + 1;
	*curWVertsNum += 1;
	curWVerts[*curWVertsNum] = z + 1;
	*curWVertsNum += 1;
	  
	curWTex[*curWTexNum] = (texX + 1) / (float) 16;
	*curWTexNum += 1;
	curWTex[*curWTexNum] = texY / (float) 16;
	*curWTexNum += 1;
      }
  
      if(mask & CUBE_FACE_DOWN) {

	int texX = tex[1] % 16;
	int texY = tex[1] / 16;
	curWVerts[*curWVertsNum] = x;
	*curWVertsNum += 1;
	curWVerts[*curWVertsNum] = y;
	*curWVertsNum += 1;
	curWVerts[*curWVertsNum] = z;
	*curWVertsNum += 1;
	  
	curWTex[*curWTexNum] = texX / (float) 16;
	*curWTexNum += 1;
	curWTex[*curWTexNum] = texY / (float) 16;
	*curWTexNum += 1;

	curWVerts[*curWVertsNum] = x + 1;
	*curWVertsNum += 1;
	curWVerts[*curWVertsNum] = y;
	*curWVertsNum += 1;
	curWVerts[*curWVertsNum] = z;
	*curWVertsNum += 1;
	  
	curWTex[*curWTexNum] = texX / (float) 16;
	*curWTexNum += 1;
	curWTex[*curWTexNum] = (texY + 1) / (float) 16;
	*curWTexNum += 1;

	curWVerts[*curWVertsNum] = x + 1;
	*curWVertsNum += 1;
	curWVerts[*curWVertsNum] = y;
	*curWVertsNum += 1;
	curWVerts[*curWVertsNum] = z + 1;
	*curWVertsNum += 1;
	  
	curWTex[*curWTexNum] = (texX + 1) / (float) 16;
	*curWTexNum += 1;
	curWTex[*curWTexNum] = (texY + 1) / (float) 16;
	*curWTexNum += 1;

	curWVerts[*curWVertsNum] = x;
	*curWVertsNum += 1;
	curWVerts[*curWVertsNum] = y;
	*curWVertsNum += 1;
	curWVerts[*curWVertsNum] = z + 1;
	*curWVertsNum += 1;
	  
	curWTex[*curWTexNum] = (texX + 1) / (float) 16;
	*curWTexNum += 1;
	curWTex[*curWTexNum] = texY / (float) 16;
	*curWTexNum += 1;
	  
      }
      if(mask & CUBE_FACE_WEST) {
	  
	int texX = tex[5] % 16;
	int texY = tex[5] / 16;

	curWVerts[*curWVertsNum] = x;
	*curWVertsNum += 1;
	curWVerts[*curWVertsNum] = y;
	*curWVertsNum += 1;
	curWVerts[*curWVertsNum] = z;
	*curWVertsNum += 1;
	  
	curWTex[*curWTexNum] = texX / (float) 16;
	*curWTexNum += 1;
	curWTex[*curWTexNum] = (texY + 1) / (float) 16;
	*curWTexNum += 1;

	curWVerts[*curWVertsNum] = x;
	*curWVertsNum += 1;
	curWVerts[*curWVertsNum] = y + 1;
	*curWVertsNum += 1;
	curWVerts[*curWVertsNum] = z;
	*curWVertsNum += 1;
	  
	curWTex[*curWTexNum] = texX / (float) 16;
	*curWTexNum += 1;
	curWTex[*curWTexNum] = texY / (float) 16;
	*curWTexNum += 1;

	curWVerts[*curWVertsNum] = x;
	*curWVertsNum += 1;
	curWVerts[*curWVertsNum] = y + 1;
	*curWVertsNum += 1;
	curWVerts[*curWVertsNum] = z + 1;
	*curWVertsNum += 1;
	  
	curWTex[*curWTexNum] = (texX + 1) / (float) 16;
	*curWTexNum += 1;
	curWTex[*curWTexNum] = texY / (float) 16;
	*curWTexNum += 1;

	curWVerts[*curWVertsNum] = x;
	*curWVertsNum += 1;
	curWVerts[*curWVertsNum] = y;
	*curWVertsNum += 1;
	curWVerts[*curWVertsNum] = z + 1;
	*curWVertsNum += 1;
	  
	curWTex[*curWTexNum] = (texX + 1) / (float) 16;
	*curWTexNum += 1;
	curWTex[*curWTexNum] = (texY + 1) / (float) 16;
	*curWTexNum += 1;
	 
      }
      if(mask & CUBE_FACE_EAST) {

	int texX = tex[3] % 16;
	int texY = tex[3] / 16;

	curWVerts[*curWVertsNum] = x + 1;
	*curWVertsNum += 1;
	curWVerts[*curWVertsNum] = y;
	*curWVertsNum += 1;
	curWVerts[*curWVertsNum] = z;
	*curWVertsNum += 1;
	  
	curWTex[*curWTexNum] = texX / (float) 16;
	*curWTexNum += 1;
	curWTex[*curWTexNum] = (texY + 1) / (float) 16;
	*curWTexNum += 1;

	curWVerts[*curWVertsNum] = x + 1;
	*curWVertsNum += 1;
	curWVerts[*curWVertsNum] = y + 1;
	*curWVertsNum += 1;
	curWVerts[*curWVertsNum] = z;
	*curWVertsNum += 1;
	  
	curWTex[*curWTexNum] = texX / (float) 16;
	*curWTexNum += 1;
	curWTex[*curWTexNum] = texY / (float) 16;
	*curWTexNum += 1;

	curWVerts[*curWVertsNum] = x + 1;
	*curWVertsNum += 1;
	curWVerts[*curWVertsNum] = y + 1;
	*curWVertsNum += 1;
	curWVerts[*curWVertsNum] = z + 1;
	*curWVertsNum += 1;
	  
	curWTex[*curWTexNum] = (texX + 1) / (float) 16;
	*curWTexNum += 1;
	curWTex[*curWTexNum] = texY / (float) 16;
	*curWTexNum += 1;

	curWVerts[*curWVertsNum] = x + 1;
	*curWVertsNum += 1;
	curWVerts[*curWVertsNum] = y;
	*curWVertsNum += 1;
	curWVerts[*curWVertsNum] = z + 1;
	*curWVertsNum += 1;
	  
	curWTex[*curWTexNum] = (texX + 1) / (float) 16;
	*curWTexNum += 1;
	curWTex[*curWTexNum] = (texY + 1) / (float) 16;
	*curWTexNum += 1;
	     
      }
      if(mask & CUBE_FACE_NORTH) {

	int texX = tex[2] % 16;
	int texY = tex[2] / 16;

	curWVerts[*curWVertsNum] = x;
	*curWVertsNum += 1;
	curWVerts[*curWVertsNum] = y;
	*curWVertsNum += 1;
	curWVerts[*curWVertsNum] = z + 1;
	*curWVertsNum += 1;
	  
	curWTex[*curWTexNum] = texX / (float) 16;
	*curWTexNum += 1;
	curWTex[*curWTexNum] = (texY + 1) / (float) 16;
	*curWTexNum += 1;

	curWVerts[*curWVertsNum] = x + 1;
	*curWVertsNum += 1;
	curWVerts[*curWVertsNum] = y;
	*curWVertsNum += 1;
	curWVerts[*curWVertsNum] = z + 1;
	*curWVertsNum += 1;
	  
	curWTex[*curWTexNum] = (texX + 1) / (float) 16;
	*curWTexNum += 1;
	curWTex[*curWTexNum] = (texY + 1) / (float) 16;
	*curWTexNum += 1;

	curWVerts[*curWVertsNum] = x + 1;
	*curWVertsNum += 1;
	curWVerts[*curWVertsNum] = y + 1;
	*curWVertsNum += 1;
	curWVerts[*curWVertsNum] = z + 1;
	*curWVertsNum += 1;
	  
	curWTex[*curWTexNum] = (texX + 1) / (float) 16;
	*curWTexNum += 1;
	curWTex[*curWTexNum] = texY / (float) 16;
	*curWTexNum += 1;

	curWVerts[*curWVertsNum] = x;
	*curWVertsNum += 1;
	curWVerts[*curWVertsNum] = y + 1;
	*curWVertsNum += 1;
	curWVerts[*curWVertsNum] = z + 1;
	*curWVertsNum += 1;
	  
	curWTex[*curWTexNum] = texX / (float) 16;
	*curWTexNum += 1;
	curWTex[*curWTexNum] = texY / (float) 16;
	*curWTexNum += 1;
	 
      }
      if(mask & CUBE_FACE_SOUTH) {

	int texX = tex[4] % 16;
	int texY = tex[4] / 16;

	curWVerts[*curWVertsNum] = x;
	*curWVertsNum += 1;
	curWVerts[*curWVertsNum] = y;
	*curWVertsNum += 1;
	curWVerts[*curWVertsNum] = z;
	*curWVertsNum += 1;
	  
	curWTex[*curWTexNum] = (texX + 1) / (float) 16;
	*curWTexNum += 1;
	curWTex[*curWTexNum] = (texY + 1) / (float) 16;
	*curWTexNum += 1;

	curWVerts[*curWVertsNum] = x + 1;
	*curWVertsNum += 1;
	curWVerts[*curWVertsNum] = y;
	*curWVertsNum += 1;
	curWVerts[*curWVertsNum] = z;
	*curWVertsNum += 1;
	  
	curWTex[*curWTexNum] = texX / (float) 16;
	*curWTexNum += 1;
	curWTex[*curWTexNum] = (texY + 1) / (float) 16;
	*curWTexNum += 1;

	curWVerts[*curWVertsNum] = x + 1;
	*curWVertsNum += 1;
	curWVerts[*curWVertsNum] = y + 1;
	*curWVertsNum += 1;
	curWVerts[*curWVertsNum] = z;
	*curWVertsNum += 1;
	  
	curWTex[*curWTexNum] = texX / (float) 16;
	*curWTexNum += 1;
	curWTex[*curWTexNum] = texY / (float) 16;
	*curWTexNum += 1;

	curWVerts[*curWVertsNum] = x;
	*curWVertsNum += 1;
	curWVerts[*curWVertsNum] = y + 1;
	*curWVertsNum += 1;
	curWVerts[*curWVertsNum] = z;
	*curWVertsNum += 1;
	  
	curWTex[*curWTexNum] = (texX + 1) / (float) 16;
	*curWTexNum += 1;
	curWTex[*curWTexNum] = texY / (float) 16;
	*curWTexNum += 1;
	 

      }
    }
  }

}

//copies all data from VBD - all stored VBVs
void VBDtoArrays()
{
  printf("starting arrays init\n");
  for(int i = 0; i < 2 * visibility; i++){
    for(int j = 0; j < 2 * visibility; j++){
      VBVtoArray(i, j);
    }
  }
  printf("ended arrays init\n");
}


//this was used in testing of editing block data
int roofLine = 0;

void drawRoof(){
  printf("drawroof %d\n", roofLine);
  for(int i = 0; i < 2 * visibility; i++)
    for(int j = 0; j < CHUNK_DIM; j++){
      setCacheBlock(i * CHUNK_DIM + j, 127, roofLine, BLOCK_TYPE_STONE);
    }

  
  for(int i = 0; i < 2 * visibility; i++){
    //here should place over old not reserve new memory
    free(vbd[i + (roofLine / 16) * 2 * visibility]->data);
    vbd[i + (roofLine / 16) * 2 * visibility]->data = malloc(sizeof(visibleBlock) * INITIAL_VBV_VOLUME);
    vbd[i + (roofLine / 16) * 2 * visibility]->volume = INITIAL_VBV_VOLUME;
    vbd[i + (roofLine / 16) * 2 * visibility]->length = 0;
    
    
    chunkToVBV(i * CHUNK_DIM, 0, roofLine / 16 * CHUNK_DIM,
	       &blockData[i + (roofLine / 16) * 2 * visibility],
	       vbd[i + (roofLine / 16) * 2 * visibility]);
    VBVtoArray(i, roofLine / 16);
  }
      
  glutTimerFunc(1000, drawRoof, 1);

  roofLine++;

}

int
main (int argc, char **argv)
{
  //set initial value for variables
  hasAim = 0;
  
  tVerts = malloc(sizeof(int *) * 2 * visibility * 2 * visibility);
  tTex = malloc(sizeof(float *) * 2 * visibility * 2 * visibility);
  assert(tVerts && tTex);
  for(int i = 0; i < 4 * pow(visibility, 2); i++){
    tVerts[i] = malloc(sizeof(int) * tmpLen);
    tTex[i] = malloc(sizeof(float) * tmpLen);
    assert(tVerts[i] && tTex[i]);
  }

  wVerts = malloc(sizeof(int *) * 2 * visibility * 2 * visibility);
  wTex = malloc(sizeof(float *) * 2 * visibility * 2 * visibility);
  assert(wVerts && wTex);
  for(int i = 0; i < 4 * pow(visibility, 2); i++){
    wVerts[i] = malloc(sizeof(int) * tmpLen);
    wTex[i] = malloc(sizeof(float) * tmpLen);
    assert(wVerts[i] && wTex[i]);
  }

  chunkStats = calloc(4 * pow(visibility, 2) * 5 , sizeof(int));
  assert(chunkStats);
  
  pos.x = 0;
  pos.y = 100;
  pos.z = 160;

  mouseX = 0;
  mouseY = 0;

  bdwo.x = 0;
  bdwo.y = 0;
  bdwo.z = 0;

  initTreeModel();

  //four since data for both sides of player is kept
  //as aim can be changed quickly
  blockData = malloc(sizeof(chunk) * 4 * visibility * visibility);
  assert(blockData);

  BDinit();

  //  blockData[ 0 ] [ 1 + 16 + 256 * 16] = BLOCK_TYPE_SAND;
  
  VBDinit();
  //  VBVprint(vbd[0]);
  VBDtoArrays();
  printf("finished VBVto arrays\n");



  //seting up glut
  glutInit(&argc, argv);

  glutInitDisplayMode(GLUT_RGB | GLUT_DOUBLE | GLUT_DEPTH);
  glutSetKeyRepeat(GLUT_KEY_REPEAT_OFF);

  glutCreateWindow("BlockCraft");
  glutFullScreen();
  glutSetCursor(GLUT_CURSOR_NONE);
  glClearColor(0.25, 0.5, 0.8, 1);
  //glEnable(GL_CULL_FACE);
  //glCullFace(GL_BACK);

  loadTexPack("./media/texturePackGreen.png");

  glEnable(GL_TEXTURE_2D);

  glBindTexture(GL_TEXTURE_2D, texPack);
  glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);

  glColor4f(0., 1., 0., 0.);
  
  terrain = glGenLists(1);
  glNewList(terrain, GL_COMPILE);
  drawVBD();
  glEndList();

  
  //glutTimerFunc(1000, drawRoof, 1);
  
  glutPassiveMotionFunc(passiveMotion);
  glutReshapeFunc(reshape);
  glutDisplayFunc(display);
  glutKeyboardFunc(keyboard);
  glutKeyboardUpFunc(keyboardUp);
  //glutTimerFunc(1000, showRefreshRate, 0);

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


void setCacheBlock(wCoordX x, wCoordY y, wCoordZ z, blockType b){
 worldCoords delta;
  delta.x = x - bdwo.x;
  delta.y = y - bdwo.y;
  delta.z = z - bdwo.z;

 (blockData[(delta.x / CHUNK_DIM) + (delta.z / CHUNK_DIM) * 2 * visibility])
    [(delta.x % CHUNK_DIM)
     + (delta.y) * CHUNK_DIM
     + (delta.z % CHUNK_DIM) * CHUNK_DIM * MAX_HEIGHT] = b;
 
}

/* make VBV representation of a chunk */
void chunkToVBV(wCoordX oX, wCoordY oY, wCoordZ oZ, chunk *c, VBV *v)
{
  for(int i = 0; i < CHUNK_DIM; i++)
    for(int j = 0; j < MAX_HEIGHT; j++)
      for(int k = 0; k < CHUNK_DIM; k++){
	//if(j == 127)
	  //printf("%d ", *c[i + j * CHUNK_DIM + k * CHUNK_DIM * MAX_HEIGHT]);
	if(!(*c)[i + j * CHUNK_DIM + k * CHUNK_DIM * MAX_HEIGHT]) continue;
	short faces = 0;

	blockType b = readCacheBlock(oX + i , j, oZ + k);
	
	if(isCached(oX + i  - 1, j, k + oZ)
	   && ((!readCacheBlock(oX + i - 1, j, k + oZ)
		|| (b!= BLOCK_TYPE_WATER && readCacheBlock(oX + i - 1, j, k + oZ) == BLOCK_TYPE_WATER))))
	  faces |= CUBE_FACE_WEST;
	
	if(isCached(oX + i  + 1, j, k + oZ)
	   && ((!readCacheBlock(oX + i  + 1, j, k + oZ)
		|| (b!= BLOCK_TYPE_WATER && readCacheBlock(oX + i  + 1, j, k + oZ) == BLOCK_TYPE_WATER))))
	  faces |= CUBE_FACE_EAST;
	
	if(isCached(i + oX, oY + j - 1, k + oZ)
	   && ((!readCacheBlock(i + oX, oY + j - 1, k + oZ)
		|| (b!= BLOCK_TYPE_WATER &&readCacheBlock(i + oX, oY + j - 1, k + oZ) == BLOCK_TYPE_WATER))))
	  faces |= CUBE_FACE_DOWN;
	
	if(isCached(i + oX, j  + 1, k + oZ)
	   && ((!readCacheBlock(i + oX, j + 1, k + oZ)
		|| (b!= BLOCK_TYPE_WATER &&readCacheBlock(i + oX, j + 1, k + oZ) == BLOCK_TYPE_WATER))))
	  faces |= CUBE_FACE_UP;
	
	if(isCached(i + oX, j, oZ + k  - 1)
	   && ((!readCacheBlock(i + oX, j, oZ + k  - 1)
		|| (b!= BLOCK_TYPE_WATER &&readCacheBlock(i + oX, j, oZ + k  - 1) == BLOCK_TYPE_WATER))))
	  faces |= CUBE_FACE_SOUTH;
	
	if(isCached(i + oX, j, oZ + k  + 1)
	   && ((!readCacheBlock(i + oX, j, oZ + k  + 1)
		|| (b!= BLOCK_TYPE_WATER && readCacheBlock(i + oX, j, oZ + k  + 1) == BLOCK_TYPE_WATER))))
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

//gets num of us needed to finish execution of display func
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
  blockType b = vecColide(pos.x, pos.y, pos.z, aim.x, aim.y, aim.z, 3);

  if(hasAim) {
    printf("%d at aim at %d %d %d\n", b, atAimX, atAimY, atAimZ);
    blockUpdate(atAimX, atAimY, atAimZ, BLOCK_TYPE_AIR);
  }
  
  callActions();

  //drawBD();
  //drawVBD();
  //glCallList(terrain);


  //check if updates are needed
  for(int i = 0; i < 2 * visibility; i++)
    for( int j = 0; j < 2 * visibility; j++){
      if(chunkStats[(i + j * 2 * visibility) * 5 + 4]){
	free(vbd[i + j * 2 * visibility]->data);
	vbd[i + j * 2 * visibility]->data = malloc(sizeof(visibleBlock) * INITIAL_VBV_VOLUME);
	vbd[i + j * 2 * visibility]->volume = INITIAL_VBV_VOLUME;
	vbd[i + j * 2 * visibility]->length = 0;
    
    
	chunkToVBV(i * CHUNK_DIM, 0, j * CHUNK_DIM,
		   &blockData[i + j * 2 * visibility],
		   vbd[i + j * 2 * visibility]);
	VBVtoArray(i, j);

	chunkStats[(i + j * 2 * visibility) * 5 + 4] = 0;
      }
    }

  
  glDisable(GL_BLEND);
  
  //set terrain for drawing
  glEnableClientState(GL_VERTEX_ARRAY);
  glEnableClientState(GL_TEXTURE_COORD_ARRAY);


  for(int i = 0; i < 2 * visibility; i++){
    for(int j = 0; j < 2 * visibility; j++){
      
      glVertexPointer(3, GL_INT, 0, tVerts[i + j * 2 * visibility]);
      glTexCoordPointer(2, GL_FLOAT, 0, tTex[i + j * 2 * visibility]);

      glDrawArrays(GL_QUADS, 0, chunkStats[(i + j * 2 * visibility) * 5] / 3);      
    }
  }

  //set opacity
  glEnable(GL_BLEND);
  glBlendFunc(GL_ZERO, GL_SRC_COLOR);

  

  //set water for drawing
  glEnableClientState(GL_VERTEX_ARRAY);
  glEnableClientState(GL_TEXTURE_COORD_ARRAY);


  for(int i = 0; i < 2 * visibility; i++){
    for(int j = 0; j < 2 * visibility; j++){
      
      glVertexPointer(3, GL_INT, 0, wVerts[i + j * 2 * visibility]);
      glTexCoordPointer(2, GL_FLOAT, 0, wTex[i + j * 2 * visibility]);

      glDrawArrays(GL_QUADS, 0, chunkStats[(i + j * 2 * visibility) * 5 + 2] / 3);      
    }
  }

  if(hasAim) {
    printf("drawing aim face %d\n", getAimFace);
    drawAimFace(atAimX, atAimY, atAimZ, getAimFace);
  }

  //draw water
  //  glDrawArrays(GL_QUADS, 0, wVertsNum / 3);


  //camera position
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
  
  double delta = .1;

  int deltaX = mouseX - x;
  mouseX = x;
  int deltaY = mouseY - y;
  mouseY = y;

  aimFi -= deltaX * delta / 10;
  
  aimTheta += deltaY * delta / 10; 
  if(aimTheta > PI/ 2) aimTheta = PI / 2;
  if(aimTheta < - PI / 2) aimTheta = -PI / 2;

  //stop mouse leaving window
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
  //here we make the world
  int treeCount = 0;
  
  printf("initing block data\n");
  for(int i = 0; i < 2 * visibility; i++)
    for(int j = 0; j < 2 * visibility; j++)
      for(int x = 0; x < CHUNK_DIM; x++)
	for(int z = 0; z < CHUNK_DIM; z++){
	  int worldX = bdwo.x + x + i * CHUNK_DIM ;
	  int worldZ = bdwo.z + z + j * CHUNK_DIM;

	  
	  int height = (int)(pnoise2d(worldX / 30.0, worldZ/ 30.0,
					.7, 5, 0) * 20 + 64 );
	  //this is used to amplify or minify effect of height map - meaning how flat therain is - less of 1 for flater and more for extra non-flat
	  double exp = pnoise2d(worldX / 50.0,
				worldZ / 50.0,
				.7, 1, 0)  / 2.5 + 2/(float) 5;
	  
	  terrainType currentTerrainType = getTerrainType(exp);
	  //both flatness and orignal value of height change final height of terrain - the one that is drawn
	  terrainHeight currentTerrainHeight = getTerrainHeight(pow(height, exp));
	  
	  double humidityNoise = pnoise2d(worldX / 100.0, worldZ / 100.0,
				   .7, 2, 100) ;
	  //humidity is used to decide which biome is in given coords
	  humidity currentHumidity = getHumidity(humidityNoise);

	  //sea map overloads height- meaning where is see height is inverted below sea level
	  double seaMap = pnoise2d(worldX /100.0, worldZ / 100.0,
				   .7, 5, 0);

	  //lets use a map for clouds to get a shape
	  double cloudMap = pnoise2d(worldX / 100.0, worldZ / 100.0,
				     .7, 5, 100);
	  //based on humidity, height and flatness choose a biome
	  biome currentBiome = getBiome(currentHumidity, currentTerrainHeight, currentTerrainType);



	  for(int y = 0; y < MAX_HEIGHT; y++) {
	    blockType *block = &(blockData[i + j * 2 * visibility])[x + y * CHUNK_DIM + z * CHUNK_DIM * MAX_HEIGHT];

	    if(seaMap < 0) {
	      if(y > SEA_LEVEL) {
		if(y == 127 && cloudMap > .5)
		  *block = BLOCK_TYPE_CLOUD;
		else
		  *block = BLOCK_TYPE_AIR;
	      }
	      else if(y <= SEA_LEVEL && y > SEA_LEVEL - pow(height, exp)){
		*block = BLOCK_TYPE_WATER;
	      }
	      else
		*block = BLOCK_TYPE_SAND;
	    } else {

	      if(y < pow(height, exp) + SEA_LEVEL ) {
		*block = BLOCK_TYPE_SOIL;

		//try to add a tree
		if( y == floor(pow(height, exp)) + SEA_LEVEL){
		  *block = getBlockType(worldX, y, worldZ, currentBiome);

		  if( (currentBiome == BIOME_FOREST || currentBiome == BIOME_GRASSY)
		      && *block == BLOCK_TYPE_GRASS
		      && pnoise3d(worldX, y, worldZ, 1, 1, 1) / 2 + .5
		      < (currentBiome == BIOME_FOREST ? 1/100.0: 1/1000.0)){


		    //		  if(treeCount < )
		    for(int t_i = 0; t_i < TREE_MODEL_X; t_i++)
		      for(int t_j = 0; t_j < TREE_MODEL_Y; t_j++)
			for(int t_k = 0; t_k < TREE_MODEL_Z; t_k++){
			  if(isCached(worldX + t_i - 2, y + t_j + 1, worldZ + t_k - 2))
			    setCacheBlock(worldX + t_i - 2, y + t_j + 1, worldZ + t_k - 2,
					  treeModel[t_i + t_j * TREE_MODEL_X + t_k * TREE_MODEL_X * TREE_MODEL_Y]);
			}
		    treeCount++;
		  }
	      
		}
	          
	      }

	      else if(!(*block)){
		if(y == 127 && cloudMap > .5)
		  *block = BLOCK_TYPE_CLOUD;
		else 
		  *block = BLOCK_TYPE_AIR;
	      }
	    }
	  }
	}
  printf("block data init finished \n");
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

terrainHeight getTerrainHeight(double h)
{
  if(h < TERRAIN_HEIGHT_LOW_VAL){
    return TERRAIN_HEIGHT_LOW;
  } else if(h < TERRAIN_HEIGHT_HIGH_VAL){
    return TERRAIN_HEIGHT_MID;
  } else
    return TERRAIN_HEIGHT_HIGH;
}

terrainType getTerrainType(double e){
  if(e < .4)
    return TERRAIN_TYPE_VALE;
  if(e < .7)
    return TERRAIN_TYPE_HILL;
  return TERRAIN_TYPE_MOUNT;
}
humidity getHumidity(double h){
  if(h < HUMIDITY_LOW_VAL)
    return HUMIDITY_LOW;
  if(h < HUMIDITY_HIGH_VAL)
    return HUMIDITY_MID;
  return HUMIDITY_HIGH;
}

biome getBiome(humidity w, terrainHeight h, terrainType g){

  if(h == TERRAIN_HEIGHT_HIGH){
    
    if(g == TERRAIN_TYPE_VALE){
      switch(w){
      case HUMIDITY_LOW:
	return BIOME_SNOWY;
	break;
      case HUMIDITY_MID:
	return BIOME_GRASSY;
	break;
      case HUMIDITY_HIGH:
	return BIOME_SNOWY;
	break;
      }
    }
    if(g == TERRAIN_TYPE_HILL){
      switch(w){
      case HUMIDITY_LOW:
	return BIOME_ROCKY;	
	break;
      case HUMIDITY_MID:
	return BIOME_SNOWY;
	break;
      case HUMIDITY_HIGH:
	return BIOME_SNOW_TOP;
	break;
      }
    }
    if(g == TERRAIN_TYPE_MOUNT){
      switch(w){
      case HUMIDITY_LOW:
	return BIOME_ROCKY;
	break;
      case HUMIDITY_MID:
	return BIOME_SNOWY;
	break;
      case HUMIDITY_HIGH:
	return BIOME_SNOW_TOP;
	break;
      }
    }
  }


  if(h == TERRAIN_HEIGHT_MID){
    
    if(g == TERRAIN_TYPE_VALE){
      switch(w){
      case HUMIDITY_LOW:
	return BIOME_DESERT;
	break;
      case HUMIDITY_MID:
	return BIOME_GRASSY;
	break;
      case HUMIDITY_HIGH:
	return BIOME_FOREST;
	break;
      }
    }
    if(g == TERRAIN_TYPE_HILL){
      switch(w){
      case HUMIDITY_LOW:
	return BIOME_DESERT;
	break;
      case HUMIDITY_MID:
	return BIOME_GRASSY;
	break;
      case HUMIDITY_HIGH:
	return BIOME_FOREST;
	break;
      }
    }
    
    if(g == TERRAIN_TYPE_MOUNT){
      switch(w){
      case HUMIDITY_LOW:
	return BIOME_ROCKY;
	break;
      case HUMIDITY_MID:
	return BIOME_ROCKY;
	break;
      case HUMIDITY_HIGH:
	return BIOME_SNOWY;
	break;
      }
    }
  }


    if(h == TERRAIN_HEIGHT_LOW){
    
    if(g == TERRAIN_TYPE_VALE){
      switch(w){
      case HUMIDITY_LOW:
	return BIOME_DESERT;
	break;
      case HUMIDITY_MID:
	return BIOME_GRASSY;
	break;
      case HUMIDITY_HIGH:
	return BIOME_FOREST;
	break;
      }
    }
    if(g == TERRAIN_TYPE_HILL){
      switch(w){
      case HUMIDITY_LOW:
	return BIOME_DESERT;
	break;
      case HUMIDITY_MID:
	return BIOME_GRASSY;
	break;
      case HUMIDITY_HIGH:
	return BIOME_FOREST;
	break;
      }
    }
    if(g == TERRAIN_TYPE_MOUNT){
      switch(w){
      case HUMIDITY_LOW:
	return BIOME_ROCKY;
	break;
      case HUMIDITY_MID:
	return BIOME_GRASSY;
	break;
      case HUMIDITY_HIGH:
	return BIOME_FOREST;
	break;
      }
    }
  }

  
}

blockType getBlockType(wCoordX x, wCoordY y, wCoordZ z, biome b){

  double n = pnoise3d(x , y , z ,
		      .7, 1, 10) / 2 + .5 ;

  
  switch(b){
  case BIOME_FOREST:
    if(n < .1){
      return BLOCK_TYPE_STONE;
    } 
    else
      return BLOCK_TYPE_GRASS;

    break;

    
  case BIOME_GRASSY:
    if(n < .01){
      return BLOCK_TYPE_STONE;
    }  else
      return BLOCK_TYPE_GRASS;
    break;


  case BIOME_DESERT:
    return BLOCK_TYPE_SAND;
    break;


  case BIOME_SNOW_TOP:
    return BLOCK_TYPE_SNOW;
    break;


  case BIOME_SNOWY:
    if(n < .1){
      return BLOCK_TYPE_GRASS;
    } else
    if(n < .5 && n > .4){
      return BLOCK_TYPE_STONE;
    } 
    else
      return BLOCK_TYPE_SNOW;
    break;


  case BIOME_ROCKY:
    if(n < .3){
      return BLOCK_TYPE_GRASS;
    } else
    if(n < .30 && n > .5){
      return BLOCK_TYPE_SOIL;
    } else
      return BLOCK_TYPE_STONE;
    break;

  }
  
}

void initTreeModel(){
  treeModel[2 + 0 * TREE_MODEL_X + 2 * TREE_MODEL_X * TREE_MODEL_Y] = BLOCK_TYPE_OAK_WOOD;

  for(int i = 0; i < TREE_MODEL_X; i++)
    for(int j = 0; j < TREE_MODEL_Y; j++)
      for(int k = 0; k < TREE_MODEL_Z; k++){
	if(j < 3 && k == 2 && i == 2) 
	  treeModel[i + j * TREE_MODEL_X + k * TREE_MODEL_Y * TREE_MODEL_X] = BLOCK_TYPE_OAK_WOOD;
	else if(j >= 3 && j < 5){
	  if(i == 2 && k == 2)
	    treeModel[i + j * TREE_MODEL_X + k * TREE_MODEL_Y * TREE_MODEL_X] = BLOCK_TYPE_OAK_WOOD;
	  else
	    treeModel[i + j * TREE_MODEL_X + k * TREE_MODEL_Y * TREE_MODEL_X] = BLOCK_TYPE_OAK_LEAF;
	}
	else if(j >= 5){
	  if(i > 0 && i < TREE_MODEL_X - 1 && k > 0 && k < TREE_MODEL_Z - 1)
	    treeModel[i + j * TREE_MODEL_X + k * TREE_MODEL_Y * TREE_MODEL_X] = BLOCK_TYPE_OAK_LEAF;
	}
      }
  
}

blockType vecColide(double x, double y, double z, double vecX, double vecY, double vecZ, double range) {
  double norm = sqrt(vecX * vecX + vecY * vecY + vecZ * vecZ);

  vecX /= norm;
  vecY /= norm;
  vecZ /= norm;

  for(int i = 0; i <= range; i++) {
    blockType b = readCacheBlock( floor( x + i * vecX ),
				  floor( y + i * vecY ),
				  floor( z + i * vecZ ));
    if(b){
      atAimX = floor( x + i * vecX);
      atAimY = floor( y + i * vecY);
      atAimZ = floor( z + i * vecZ);
      hasAim = 1;
      return b;
    }
  }

  hasAim = 0;
  return BLOCK_TYPE_AIR;
}

void blockUpdate(wCoordX x, wCoordY y, wCoordZ z, blockType b) {
  setCacheBlock(x, y, z, b);

  int deltaX = x - bdwo.x;
  int deltaZ = z - bdwo.z;
  chunkStats[(deltaX / 16 + deltaZ / 16 * 2 * visibility) * 5 + 0] = 0;
  chunkStats[(deltaX / 16 + deltaZ / 16 * 2 * visibility) * 5 + 1] = 0;
  chunkStats[(deltaX / 16 + deltaZ / 16 * 2 * visibility) * 5 + 2] = 0;
  chunkStats[(deltaX / 16 + deltaZ / 16 * 2 * visibility) * 5 + 3] = 0;
  chunkStats[(deltaX / 16 + deltaZ / 16 * 2 * visibility) * 5 + 4] = 1;
  
}

cubeFaces getAimFace() {
  double absX = fabs(aim.x);
  double absY = fabs(aim.y);
  double absZ = fabs(aim.z);

  if(absX > absY && absX > absZ){
    if(aim.x > 0) return CUBE_FACE_WEST;
    else return CUBE_FACE_EAST;
  } else if(absY > absX && absY > absZ){
    if(aim.y > 0) return CUBE_FACE_DOWN;
    else return CUBE_FACE_UP;
  } else {
    if(aim.z > 0) return CUBE_FACE_SOUTH;
    else return CUBE_FACE_NORTH;
  }
}
void drawAimFace(wCoordX x, wCoordY y, wCoordZ z, cubeFaces f) {
  glColor3f(.0, .0, .0);
  glLineWidth(3);

  
  switch(f) {
  case CUBE_FACE_UP:
    glBegin(GL_QUADS);
    glVertex3f(x, y + 1, z);
    glVertex3f(x + 1, y + 1, z);
    glVertex3f(x + 1, y + 1, z + 1);
    glVertex3f(x, y + 1, z + 1);
    glEnd();
    break;
  case CUBE_FACE_DOWN:
    glBegin(GL_QUADS);
    glVertex3f(x, y, z);
    glVertex3f(x + 1, y, z);
    glVertex3f(x + 1, y, z + 1);
    glVertex3f(x, y, z + 1);
    glEnd();
    
    break;
  case CUBE_FACE_EAST:
    glBegin(GL_QUADS);
    glVertex3f(x + 1, y + 1, z);
    glVertex3f(x + 1, y + 1, z);
    glVertex3f(x + 1, y + 1, z + 1);
    glVertex3f(x + 1, y, z + 1);
    glEnd();
    
    break;
  case CUBE_FACE_WEST:
    glBegin(GL_QUADS);
    glVertex3f(x, y + 1, z);
    glVertex3f(x, y + 1, z);
    glVertex3f(x, y + 1, z + 1);
    glVertex3f(x, y, z + 1);
    glEnd();
    
    break;
  case CUBE_FACE_SOUTH:
    glBegin(GL_QUADS);
    glVertex3f(x, y, z);
    glVertex3f(x + 1, y, z);
    glVertex3f(x + 1, y + 1, z);
    glVertex3f(x, y + 1, z);
    glEnd();
    
    break;
  case CUBE_FACE_NORTH:
    glBegin(GL_QUADS);
    glVertex3f(x, y, z + 1);
    glVertex3f(x + 1, y, z + 1);
    glVertex3f(x + 1, y + 1, z + 1);
    glVertex3f(x, y + 1, z + 1);
    glEnd();
    
    break;
  }
  
}

