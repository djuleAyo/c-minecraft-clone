#include "keyStore.h"

keyEnt *keyTable = NULL;

keyEnt *KeyEnt(char c)
{
  keyEnt *new = malloc(sizeof(keyEnt));
  //  assert(

  //here must init to NULL for later checking
  new->next = NULL;

  return new;
}

void press(unsigned char c)
{
  keyEnt *e = KeyEnt(c);
  e->c = c;

  if(keyTable == NULL) keyTable = e;
  else {
    keyEnt *i = keyTable;

    while(i->next != NULL)
      i = i->next;

    i->next = e;
  }
}


void release(char c)
{
  if(keyTable == NULL) return;

  if(keyTable->c == c) {
    keyEnt *tmp = keyTable->next;
    free(keyTable);
    keyTable = tmp;
    return;
  }

  keyEnt *i = keyTable;

  while(i->next && i->next->c != c){
    i = i->next;
  }

  if(i->next){
    keyEnt *tmp = i->next;
    i->next = tmp->next;
    free(tmp);
  }
}

void keyEnd(char c)
{
  if(keyTable == NULL)
    return;
  if(keyTable->c == c){
    assert(gettimeofday(&(keyTable->end), NULL) == 0);
    return;
  }

  keyEnt *i = keyTable;

  while(i->next && i->c != c)
    i = i->next;

  if(i->c == c){
    assert(gettimeofday(&(i->end), NULL) == 0);
  }
}


void keyTablePrint()
{
  keyEnt *i = keyTable;

  while(i){
    printf("%c ", i->c);
    i = i->next;
  }

  printf("\n");
}
