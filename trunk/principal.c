/*
 *    Algoritmo Chord
 *
 */

#define NPROC 8

#define PETITION 4

#include <stdio.h>
#include <stdlib.h>


#include <sys/types.h>
#include "pvm3.h"


void dowork( int, int );
void ptransmit(int, int);
void pfound(int);

int main(int argc, char *argv[])
{
  int mytid;                  /* mi task id */
  int tids[NPROC];            /* array de task id */
  int me;                     /* mi numero de proceso */
  int info;
  int i;

  /* identificarme en pvm */
  mytid = pvm_mytid();

  /* Unirme al grupo, y si soy el primer proceso */
  /* (me=0) expandir mas copias de mi mismo    */

  me = pvm_joingroup( "anillo-chord" );
  printf("me = %d mytid = %d\n",me,mytid);

  if( me == 0 ) {

    pvm_catchout(stdout);

    info= pvm_spawn("principal", (char**)0, 0, "", NPROC-1, &tids[1]);

    if( info != NPROC-1 ) {

      for(i=0;i<info;i++) pvm_kill(tids[i]);

      pvm_lvgroup( "anillo-chord" );
      pvm_exit();
      return 0;
    }
  }


  /* Esperar a que todos terminen antes de continuar. */
  pvm_freezegroup("anillo-chord", NPROC);
  pvm_barrier( "anillo-chord", NPROC );
  /*--------------------------------------------------------------------------*/

  dowork( me, NPROC );

  /* Programa terminado, salir del grupo y de pvm */
  pvm_lvgroup( "anillo-chord" );
  pvm_exit();
  return 0;
}



/* Simple example passes a token around a ring */

void dowork( int me, int nproc )
{
  int token;

  int ante, post;
  //int *finger;

  int count  = 1;
  int stride = 1;

  /* Determinar los nodos vecinos en el anillo */
  ante= pvm_gettid("anillo-chord", me-1);
  post= pvm_gettid("anillo-chord", me+1);
  if( me == 0 )       ante = pvm_gettid("anillo-chord", NPROC-1);
  if( me == NPROC-1 ) post = pvm_gettid("anillo-chord", 0);

 
  srand(0);
  

  if( me == 0 ) {

    printf("Introduce token a buscar:");
    scanf("%d",&token);

    pvm_initsend( PvmDataDefault );
    pvm_pkint( &token, count, stride );
    pvm_send( post, PETITION );

    printf("petición de token enviada\n");
  }

  else {

    /* Espera a la recepción de una petición de token */
    pvm_recv( ante, PETITION );
    pvm_upkint( &token, count, stride );

    if(token==me) {
      pfound(token);
    }
    else {
      /* Si no tiene el token, transmite la petición por el anillo */
      ptransmit(token, post);
    }
  }
}



/******************
Transmite la petición de un token
 */
void ptransmit(int token, int destination)
{
  printf("transmitiendo peticion a %d\n",destination);

  pvm_initsend( PvmDataDefault );
  pvm_pkint( &token, 1, 1);
  pvm_send( destination, PETITION );
}



/*************************
 Atender la peticion del token
 */
void pfound(int token)
{
  printf("Yo tengo el %d!\n",token);
}
