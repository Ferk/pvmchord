/*
 *    Algoritmo Chord
 *
 */

#define NPROC 8

#define PETITION 4
#define PING 1
#define PONG 2

#define TMOUT -5

#include <stdio.h>
#include <stdlib.h>


#include <sys/types.h>
#include "pvm3.h"


void dowork( int, int );
void ptransmit(int, int);
void pfound(int);

int findnext(int);

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

  int info;

  struct timeval tmout; /* timeout para la rutina trecv */
  tmout.tv_sec=10; /* indica que trecv esperara 10 segundos */
  tmout.tv_usec=10; /* antes de devolver el mensage "10" */

  /* Determinar los nodos vecinos en el anillo */
  // ante= findprev(); pvm_gettid("anillo-chord", me-1);
  post= findnext(me);
  // if( me == 0 )       ante = pvm_gettid("anillo-chord", NPROC-1);


 
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

    /* Espera a la recepción de la petición de algun token */
    info = pvm_trecv( -1, PETITION, &tmout );
    if ( info >= 0 ) {
      pvm_upkint( &token, count, stride );

      /* si tiene el token, atiende la peticion */
      /* de lo contrario transmite la petición por el anillo */
      if( token == me ) pfound(token);
      else ptransmit(token, post);
    }
    else printf("timeout! ninguna peticion recibida\n");
  }
}


/********
 Devuelve el nodo activo siguiente
 */
int findnext(int me)
{
  int next, gnext;
  int info;

  struct timeval tmout; /* timeout para la rutina trecv */
  tmout.tv_sec=1; /* indica que trecv esperara 10 segundos */
  tmout.tv_usec=TMOUT; /* antes de devolver el mensage TMOUT */
  
  gnext=me;
  do {
    gnext++;
    if( gnext == NPROC ) next = pvm_gettid("anillo-chord", 0);
    else                 next= pvm_gettid("anillo-chord", gnext);
    
    pvm_initsend( PvmDataDefault );
    info = pvm_send( next, PING);
    info = pvm_trecv( next, PONG, &tmout );
  } while(info == TMOUT);

  return next;
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
