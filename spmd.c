/*
*    SPMD example using PVM 3
*    also illustrating group functions
*/

#define NPROC 8

#include <stdio.h>
#include <sys/types.h>
#include "pvm3.h"

main()
{
    int mytid;                  /* my task id */
    int tids[NPROC];            /* array of task id */
    int me;                     /* my process number */
    int i;

    /* enroll in pvm */
    mytid = pvm_mytid();

    /* Join a group and if I am the first instance */
    /* i.e. me=0 spawn more copies of myself       */

    me = pvm_joingroup( "foo" );
    printf("me = %d mytid = %d\n",me,mytid);

    if( me == 0 )
       pvm_spawn("spmd", (char**)0, 0, "", NPROC-1, &tids[1]);

    /* Wait for everyone to startup before proceeding. */
    pvm_freezegroup("foo", NPROC);
    pvm_barrier( "foo", NPROC );
/*--------------------------------------------------------------------------*/
     
     dowork( me, NPROC );

     /* program finished leave group and exit pvm */
     pvm_lvgroup( "foo" );
     pvm_exit();
     exit(1);
}

/* Simple example passes a token around a ring */

dowork( me, nproc )
     int me;
     int nproc;
{
     int token;
     int src, dest;
     int count  = 1;
     int stride = 1;
     int msgtag = 4;

     /* Determine neighbors in the ring */
     src = pvm_gettid("foo", me-1);
     dest= pvm_gettid("foo", me+1);
     if( me == 0 )       src = pvm_gettid("foo", NPROC-1);
     if( me == NPROC-1 ) dest = pvm_gettid("foo", 0);

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
