#include <sys/time.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

typedef struct keyEnt_ {
  struct timeval start;
  struct timeval end;
  struct keyEnt_ *next;
  char c;
} keyEnt;

extern keyEnt *keyTable;

keyEnt *KeyStore(char c);

void press(unsigned char c);

void release(char c);

void keyEnd(char c);

void keyTablePrint();
