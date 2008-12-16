/*
 *    Algoritmo Chord
 *
 */

#define NPROC 8


#define PETITION 2
#define NOTIFY 3

#define ANTECESOR 4
#define KEY 5

#define REPORT 1

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include "pvm3.h"

int main(int argc, char *argv[])
{
  int mytid;                  /* mi task id */
  int tids[NPROC];            /* array de task id */
  int info;
  int i;
  int msg, token;

  int predecessor, successor, node;

  /* identificarme en pvm */
  mytid = pvm_mytid();

  /***************/
  /* Expansión de procesos nodo (creación del anillo) */
  pvm_catchout(stdout);
  info= pvm_spawn("nodo", (char**)0, 0, "", NPROC-1, tids);
  if( info != NPROC-1 ) {
    for(i=0;i<info;i++) pvm_kill(tids[i]);
    pvm_exit();
    return 1;
  }


  struct timeval tmout; /* timeout para la rutina trecv */
  tmout.tv_sec=0; /* indica que trecv esperara estos segundos */
  tmout.tv_usec=100000; /* y estos microsegundos */

  /*************************************/
  /*** Recabar y mostrar información ***/
  for(i=0; i<100; i++) {
    info = pvm_trecv( -1, REPORT, &tmout );
    if ( info > 0 ) {
      pvm_upkint(&node,1,1);
      pvm_upkint(&predecessor,1,1);
      pvm_upkint(&successor,1,1);

      printf("Nodo%d\t-->\tNodo%d\t-->\tNodo%d\n", predecessor, node, successor);
    }

  }



  /***************************/
  /*** Peticion de claves  ***/

  /*

  printf("Introduce clave a buscar:");
  scanf("%d",&token);

  pvm_initsend( PvmDataDefault );
  msg = KEY;
  pvm_pkint( &msg, 1, 1);
  pvm_pkint( &token, 1, 1);
  pvm_send( tids[0], NOTIFY );

  printf("petición de token enviada\n");
  */

  /****************/
  /* Programa terminado, salir de pvm */
  /* pvm_lvgroup( "anillo-chord" ); */
  printf("Finalizando nodos...\n");
  for(i=0;i<info;i++) pvm_kill(tids[i]);
  printf("Proceso padre saliendo...\n");
  pvm_exit();
  return 0;
}

