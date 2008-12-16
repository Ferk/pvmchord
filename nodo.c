/*
 *    Algoritmo Chord
 *
 * http://www.mpi-inf.mpg.de/departments/d5/teaching/ws03_04/p2p-data/11-18-writeup1.pdf
 */

#define NPROC 8

#define REPORT 1

#define PETITION 2
#define NOTIFY 3

#define ANTECESOR 4
#define KEY 5
#define SUCCESSOR 6


#include <stdio.h>
#include <stdlib.h>

#include <sys/types.h>
#include "pvm3.h"

void dowork( int, int );
void ptransmit(int, int);
void pfound(int);

void stabilize(int , int , int );

int findNext();
void reportState();

int mytid;                  /* mi task id */
int mynode;                 /* mi numero en el anillo */
int predecessor;
int successor;

int main(int argc, char *argv[])
{
  int i;

  /* identificarme en pvm */
  mytid = pvm_mytid();

  /* Unirme al grupo  */
  mynode = pvm_joingroup( "anillo-chord" );
  printf("me = %d mytid = %d\n",mynode,mytid);


  /******************************************/
  /*** BARRERA ***/
  pvm_barrier("anillo-chord",NPROC);


  /********************************************************************/
  /* Aleatoriamente se cerraría el nodo (dejamos huecos en el anillo)  */


  

  /*********************/
  /* Creación/Unión al anillo */

  /* Inicialmente el predecesor se toma como inexistente */
  predecessor = -1;

  successor = findNext();


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
    case SUCCESSOR: /* Petición de sucesor */
      pvm_pkint( &successor, 1, 1);
      break;
    }
    pvm_initsend( PvmDataDefault );
    pvm_send(sender, msg );
  }

  /***********************************/
  /*** Verificar sucesor inmediato ***/
  /* preguntando a mi sucesor si soy yo su antecesor */
  pvm_initsend( PvmDataDefault );
  msg=ANTECESOR;
  pvm_pkint( &msg, 1, 1);
  pvm_send(successor, PETITION );
  info = pvm_trecv( successor, ANTECESOR, &tmout );
  if ( info > 0 ) {
    pvm_upkint( &nodo_x, 1, 1);
    if( nodo_x == -1) {
      /* Notificar a mi sucesor de que soy su antecesor */
      pvm_initsend( PvmDataDefault );
      msg= ANTECESOR;
      pvm_pkint( &msg, 1, 1); /* mensaje de notificacion tipo antecesor */
      pvm_pkint( &mytid, 1, 1); /* conteniendo mi tid */
      pvm_send( successor, NOTIFY );
    }
    if( nodo_x != mytid ) {
      /*** ¡Nuevo nodo! ***/
      /* Añadir el nuevo nodo como nuevo sucesor */
      successor = nodo_x;
      /* Notificar al nuevo nodo de que soy su antecesor */
      pvm_initsend( PvmDataDefault );
      msg= ANTECESOR;
      pvm_pkint( &msg, 1, 1); /* mensaje de notificacion tipo antecesor */
      pvm_pkint( &mytid, 1, 1); /* conteniendo mi tid */
      pvm_send( nodo_x, NOTIFY );
    }
  }

  /*******************************************/
  /** Reportar el estado del nodo para ser mostrado en la interfaz **/
  reportState();

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


    id: nodo que busca su sucesor
     n: nodo al que le pregunta, que segun nuestro sistema es el anterior a id (id-1)

    // ask node n to find the successor of id
    n.find_successor(id)
    if (id\in(n, successor])
    return successor;        //Si id está entre n y su sucesor hay que 
    else                     //devolver sucessor (el sucesor de n)
    // forward the query around the circle
    // con nuestro sistema NO

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
int findNext()
{
  int tid_dest, i;

  i= mynode+1;
  do {
    tid_dest= pvm_gettid("anillo-chord",i);
    if(i>NPROC-1) i=0;
    else i++;
  } while( tid_dest == PvmNoInst );

  return tid_dest;
}


/*******************************************/
/** Reportar el estado del nodo al padre,
    quien lo mostrará en la interfaz **/
void reportState()
{
  int pred= pvm_getinst("anillo-chord",predecessor);
  int succ= pvm_getinst("anillo-chord",successor);

  pvm_initsend( PvmDataDefault );
  pvm_pkint(&mynode,1,1);
  pvm_pkint(&pred,1,1);
  pvm_pkint(&succ,1,1);
  pvm_send(pvm_parent(), REPORT);

}

 
/************************
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
