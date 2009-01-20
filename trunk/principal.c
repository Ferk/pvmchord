/*
 *    Trabajo PVM: Chord
 *	  Ampliación de Sistemas Operativos
 *	  Fernando Carmona Varo
 *    Rafael Jesús García del Pino
 * 
 *    Archivo:principal.c
 *    Fecha 23/01/2009
 */

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include "pvm3.h"
#include "macros.h"

int main(int argc, char *argv[])
{
  int mytid;                  /* mi task id */
  int tids[NPROC];            /* array de task id */
  int info, msg;			  
  int i;
  int nproc;
  int insertions=0, max_insert=5;

  int predecessor, successor, node, token;

  /* identificarme en pvm */
  mytid = pvm_mytid();

  /* Sembrar semilla aleatoria */
  srand(mytid);

  /***************/
  /* Expansión de procesos nodo (creación del anillo) */
  pvm_catchout(stdout);
  nproc= pvm_spawn("nodo", (char**)0, 0, "", NPROC, tids);
  if( nproc != NPROC ) {
    for(i=0;i<info;i++) pvm_kill(tids[i]);
    pvm_exit();
    return 1;
  }

  struct timeval tmout; /* timeout para la rutina trecv */
  tmout.tv_sec=0; /* indica que trecv esperara estos segundos */
  tmout.tv_usec=100000; /* y estos microsegundos */

  /*************************************/
  /*** Recabar y mostrar información ***/
  for(i=0; i<40; i++) {
    info = pvm_trecv( -1, REPORT, &tmout );
    if ( info > 0 ) {
      i=0; /* Pone el contador a cero, para seguir esperando informacion */
      pvm_upkint(&msg,1,1);
      switch (msg) {
      case STATE:
        pvm_upkint(&node,1,1);
        pvm_upkint(&predecessor,1,1);
        pvm_upkint(&successor,1,1);

        if( predecessor < 0 ) printf("*null*\t");
        else printf("Nodo%d\t",predecessor);
        printf("-->\tNodo%d\t-->\t",node);
        if( successor < 0 ) printf("*null*\n");
        else printf("Nodo%d\n",successor);
        break;
      case ENTER:
        pvm_upkint(&node,1,1);
        printf("()<--- El nodo %d entra al anillo\n", node);
        break;
      case EXIT:
        pvm_upkint(&node,1,1);
        printf("()---> El nodo %d sale del anillo\n", node);
        nproc--;
        break;
      case KEYINIT:
        pvm_upkint(&node,1,1);
        pvm_upkint(&token,1,1);
        printf("*** %d Inicializa petición de la clave %d ***\n",node,token);
        break;
      case KEYTRANS:
        pvm_upkint(&node,1,1);
        pvm_upkint(&token,1,1);
        printf(" ** %d transmite petición de la clave %d **\n",node,token);
        break;
      case KEYFOUND:
        pvm_upkint(&node,1,1);
        pvm_upkint(&token,1,1);
        printf("***¡¡¡Se encontró en %d la clave %d buscada!!!***\n",node,token);
        break;
      }
    }

    /* Inserción de nuevos nodos */
    if( insertions<max_insert && nproc<NPROC && (rand() < RAND_MAX/4) ) {
      printf("Spawneando un nodo\n");
      pvm_spawn("nodo", (char**)0, 0, "", 1, tids);
      nproc++;
      insertions++;
    }

  }
  printf("Se ha dejado de recibir información del anillo.\n");

  /****************/
  /* Programa terminado, salir de pvm */
  printf("Proceso padre saliendo...\n");
  pvm_exit();
  return 0;
}
