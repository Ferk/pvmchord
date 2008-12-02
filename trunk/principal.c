/*
 *    Algoritmo Chord
 *
 */

#define NPROC 8


#define PETITION 2
#define NOTIFY 3

#define ANTECESOR 4
#define KEY 5

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

  /* identificarme en pvm */
  mytid = pvm_mytid();

  /***************/
  /* Expansión de procesos nodo (creación del anillo) */
  pvm_catchout(stdout);
  info= pvm_spawn("nodo", (char**)0, 0, "", NPROC-1, &tids[1]);
  if( info != NPROC-1 ) {
    for(i=0;i<info;i++) pvm_kill(tids[i]);
    pvm_exit();
    return 1;
  }

  /***************************/
  /*** Peticion de claves  ***/

  printf("Introduce clave a buscar:");
  scanf("%d",&token);

  pvm_initsend( PvmDataDefault );
  msg = KEY;
  pvm_pkint( &msg, 1, 1);
  pvm_pkint( &token, 1, 1);
  pvm_send( tids[0], NOTIFY );

  printf("petición de token enviada\n");


  /****************/
  /* Programa terminado, salir de pvm */
  /* pvm_lvgroup( "anillo-chord" ); */
  pvm_exit();
  return 0;
}

