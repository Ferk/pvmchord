/*
 *    Algoritmo Chord
 *
 * http://www.mpi-inf.mpg.de/departments/d5/teaching/ws03_04/p2p-data/11-18-writeup1.pdf
 */

#define NPROC 8

#define PING 1

#define PETITION 2
#define NOTIFY 3

#define ANTECESOR 4
#define KEY 5


#include <stdio.h>
#include <stdlib.h>

#include <sys/types.h>
#include "pvm3.h"

void dowork( int, int );
void ptransmit(int, int);
void pfound(int);

void stabilize(int , int , int );

int findnext(int);


int predecessor;
int successor;

int main(int argc, char *argv[])
{
  int mytid;                  /* mi task id */
  int mynode;                 /* mi numero en el anillo */
  int i;


  /* identificarme en pvm */
  mytid = pvm_mytid();

  /* Unirme al grupo  */
  mynode = pvm_joingroup( "anillo-chord" );
  printf("me = %d mytid = %d\n",mynode,mytid);


  /***********/
  /* Creación/Unión al anillo */

  /* Inicialmente el predecesor se toma como inexistente */
  predecessor = -1;

  if( mynode == 0 ) {
    /* El primer nodo del anillo será el único nodo existente en ese momento */
    successor = mytid; /* y será su propio sucesor */
  }
  else {
    successor = findnext(mynode);
  }


  /******************************/
  /* Bucle de trabajo del nodo durante X ciclos */
  int cicles= 100; /* Luego lo pondremos aleatorio */
  for(i=0; i<cicles; i++) {
    dowork(mytid, mynode);
  }


  /********************/
  /* Programa terminado, salir del grupo y de pvm */
  pvm_lvgroup( "anillo-chord" );
  pvm_exit();
  return 0;
}





void dowork( int mytid, int mynode )
{
  int token;

  int info;
  int nodo_x;

  int msg, sender;
  struct timeval tmout; /* timeout para la rutina trecv */
  tmout.tv_sec=0; /* indica que trecv esperara estos segundos */
  tmout.tv_usec=100000; /* y estos microsegundos */

  /**************************/
  /*** Atender peticiones ***/
  info = pvm_trecv( -1, PETITION, &tmout );
  if ( info > 0 ) {
    pvm_bufinfo(info, NULL, NULL, &sender);
    pvm_upkint( &msg, 1, 1);
    switch(msg) {
    case ANTECESOR: /* peticion de predecesor */
      pvm_pkint( &predecessor, 1, 1);
      break;
    case PING: /* peticion de disponibilidad */
      
      break;
    }
    pvm_initsend( PvmDataDefault );
    pvm_send(sender, msg );
  }

  /***********************************/
  /*** Verificar sucesor inmediato ***/
  /* preguntando a mi sucesor si soy yo su antecesor */
  pvm_initsend( PvmDataDefault );
  info=ANTECESOR;
  pvm_pkint( &info, 1, 1);
  pvm_send(successor, PETITION );
  info = pvm_trecv( successor, ANTECESOR, &tmout );
  if ( info > 0 ) {
    pvm_upkint( &nodo_x, 1, 1);
    if( nodo_x != mytid ) {
      /*** ¡Nuevo nodo! ***/
      /* Añadir el nuevo nodo como nuevo predecesor */
      successor = nodo_x;
      /* Notificar al nuevo nodo de que soy su antecesor */
      pvm_initsend( PvmDataDefault );
      msg= ANTECESOR;
      pvm_pkint( &msg, 1, 1); /* mensaje de notificacion tipo antecesor */
      pvm_pkint( &mytid, 1, 1); /* conteniendo mi tid */
      pvm_send( nodo_x, NOTIFY );
    }
  }


  /*******************************/
  /*** Atender Notificaciones ***/
  info = pvm_trecv( -1, NOTIFY, &tmout );
  if ( info > 0 ) {
    pvm_bufinfo(info, NULL, NULL, &sender);
    pvm_upkint( &msg, 1, 1); /* tipo de mensaje de notificacion */
    switch(msg) {

    case ANTECESOR: /* Nuevo tid de antecesor */
      pvm_upkint( &predecessor, 1, 1);
      break;

    case KEY: /* Notificación de búsqueda de clave */
      pvm_upkint(&token,1,1);
      if(token == mynode) { /* si tiene el token, atiende la peticion */
        pfound(token);
      }
      else { /* de lo contrario transmite la notificación por el anillo */
        pvm_initsend( PvmDataDefault );
        pvm_pkint(&msg,1,1);
        pvm_pkint(&token,1,1);
        pvm_send(sender, NOTIFY );
      }
      break;

    }
  }

}





/***
    The pseudocode to find the successor node of an id is given below:

    // ask node n to find the successor of id
    n.find_successor(id)
    if (id\in(n, successor])
    return successor;        //Si id está entre n y su sucesor hay que 
    else                     //devolver sucessor (el sucesor de n)
    // forward the query around the circle
    n0 = closest_preceding_node(id); //Sino busca el sucesor mas cercano
    return n0.find_successor(id);

    // search the local table for the highest predecessor of id
    n.closest_preceding_node(id)
    for i = m downto 1
    if (finger[i]\in(n,id))
    return finger[i];
    return n;
***/

/********
         Devuelve el nodo activo siguiente
*/
int findnext(int me)
{
  int next, gnext;
  int info;
  int longitud,tidReceptor,tag;

  struct timeval tmout; /* timeout para la rutina trecv */
  tmout.tv_sec=0; /* indica que trecv esperara estos segundos */
  tmout.tv_usec=100000; /* y estos microsegundos */

  gnext=me;
  do {
    gnext++;
    if( gnext == NPROC ) next = pvm_gettid("anillo-chord", 0);
    else                 next= pvm_gettid("anillo-chord", gnext);

    pvm_initsend( PvmDataDefault );
    tag= PING;
    pvm_pkint(&tag,1,1);
    info = pvm_send( next, PETITION);
    info = pvm_trecv( next, PING, &tmout );
    if(info>0){
      pvm_bufinfo(info,&longitud,&tag,&tidReceptor);/* Saca información del buffer activo*/
      
      printf("Se ha recibido algo!\n");
    }
  } while(info == 0);

  return next;
}


/******************
Transmite la petición de un token
-OBSOLETO-
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



/*****
      The pseudocode to stabilize the chord ring/circle after node joins and departures is as follows:

      // create a new Chord ring.
      n.create()
      predecessor = nil;
      successor = n;

      // join a Chord ring containing node n'.
      n.join(n')
      predecessor = nil;
      successor = n'.find_successor(n);

      // called periodically. verifies n’s immediate
      // successor, and tells the successor about n.
      n.stabilize()
      x = successor.predecessor;
      if (x\in(n, successor))
      successor = x;
      successor.notify(n);

      // n' thinks it might be our predecessor.
      n.notify(n')
      if (predecessor is nil or n'\in(predecessor, n))
      predecessor = n';

      // called periodically. refreshes finger table entries.
      // next stores the index of the finger to fix
      n.fix_fingers()
      next = next + 1;
      if (next > m)
      next = 1;
      finger[next] = find_successor(n+2next − 1);

      // called periodically. checks whether predecessor has failed.
      n.check_predecessor()
      if (predecessor has failed)
      predecessor = nil;

*********/
