/*
*    Algoritmo Chord
*    
*/

#define NPROC 8

#include <stdio.h>
#include <sys/types.h>
#include "pvm3.h"

int main(int argc, char *argv[])
{
    int mytid;                  /* mi task id */
    int tids[NPROC];            /* array de task id */
    int me;                     /* mi numero de proceso */
    int info;
    int i;

    /* enroll in pvm */
    mytid = pvm_mytid();

    /* Unirme al grupo, y si soy el primer proceso */
    /* (me=0) expandir mas copias de mi mismo    */

    me = pvm_joingroup( "anillo-chord" );
    printf("me = %d mytid = %d\n",me,mytid);

    if( me == 0 ) {
      
      info= pvm_spawn("spmd", (char**)0, 0, "", NPROC-1, &tids[1]);

      if( info != NPROC-1 ) {

        for(i=0;i<info;i++) pvm_kill(tids[i]);
                
        pvm_lvgroup( "anillo-chord" );
        pvm_exit();
        exit(0);
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

void dowork( me, nproc )
     int me;
     int nproc;
{
     int token;
     int src, dest;
     int count  = 1;
     int stride = 1;
     int msgtag = 4;

     /* Determinar los nodos vecinos en el anillo */
     src = pvm_gettid("anillo-chord", me-1);
     dest= pvm_gettid("anillo-chord", me+1);
     if( me == 0 )       src = pvm_gettid("anillo-chord", NPROC-1);
     if( me == NPROC-1 ) dest = pvm_gettid("anillo-chord", 0);

     if( me == 0 )
     { 
        token = dest;
        pvm_initsend( PvmDataDefault );
        pvm_pkint( &token, count, stride );
        pvm_send( dest, msgtag );
        pvm_recv( src, msgtag );
        printf("token ring done\n");
     }
     else
     {
        pvm_recv( src, msgtag );
        pvm_upkint( &token, count, stride );
        pvm_initsend( PvmDataDefault );
        pvm_pkint( &token, count, stride );
        pvm_send( dest, msgtag );
     }
}
