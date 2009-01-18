
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

#include <unistd.h>


void mainloop( );
void ptransmit(int, int);
void pfound(int);

int findNext(int);
void stabilize( );

void reportState();
void reportEntering();
void reportExiting();

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


  /* Aleatoriamente se cerraría el nodo (dejamos huecos en el anillo)  */
  srand(mytid+mynode);


  if( rand() > RAND_MAX/4 ) {
    /******************************************/
    /* Procesos dentro del anillo */
    /*** BARRERA ***/
    //pvm_barrier("anillo-chord",NPROC);

    /* indicar la entrada del nodo en el anillo */
    reportEntering();

    /*********************/
    /* Creación/Unión al anillo */
    /* Inicialmente el predecesor se toma como inexistente */
    predecessor = -1;
    successor = findNext(mynode);
    
    //printf("%d: inicialmente, mi sucessor: %d (%d).\n",mynode,successor,pvm_getinst("anillo-chord",successor));

    reportState();
    
    /***************************************/
    /* El nodo trabajará durante un numero aleatorio de ciclos */
    int life= 20.0*rand()/RAND_MAX; 
    for(i=0; i<life; i++) {
      mainloop();
    }
    
  }  else {
    /******************************************/
    /* Procesos a expulsar del anillo  */
    /*** BARRERA ***/
    //pvm_barrier("anillo-chord",NPROC);
  }
    

  /********************/
  /* Programa terminado, salir del grupo y de pvm */
  pvm_lvgroup( "anillo-chord" );
  reportExiting();
  pvm_exit();
  return 0;
}



void mainloop()
{
  int token;

  int info;

  int msg, sender;
  struct timeval tmout; /* timeout para la rutina trecv */
  tmout.tv_sec=0; /* indica que trecv esperara estos segundos */
  tmout.tv_usec=1000000; /* y estos microsegundos */

  //printf("mi sucessor: %d (%d).\n",successor,pvm_getinst("anillo-chord",successor));

  /**************************/
  /*** Atender peticiones ***/
  info = pvm_trecv( -1, PETITION, &tmout );
  if ( info > 0 ) {
    pvm_bufinfo(info, NULL, NULL, &sender);
    pvm_upkint( &msg, 1, 1);
    pvm_initsend( PvmDataDefault );
    switch(msg) {
    case ANTECESOR: /* peticion de predecesor */
      //printf("%d: enviando mi predecesor (%d) a %d.\n",mynode,predecessor,sender);
      pvm_pkint( &predecessor, 1, 1);
      break;
    case SUCCESSOR: /* Petición de sucesor */
      //printf("enviando mi sucesor (%d) a %d.\n",successor,sender);
      pvm_pkint( &successor, 1, 1);
      break;
    }
    pvm_send(sender, msg );
  }

  /********************************************/
  /*** Rutinas de estabilización del anillo ***/
  stabilize();

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
      //printf("%d: nuevo predecesor, %d.\n",mynode,predecessor);
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


void stabilize()
{
  int nodo_x, msg, info;

  struct timeval tmout; /* timeout para la rutina trecv */
  tmout.tv_sec=0; /* indica que trecv esperara estos segundos */
  tmout.tv_usec=1000000; /* y estos microsegundos */

  /***************************************/
  /* Comprobar si el sucesor se ha caido */
  if ( pvm_getinst("anillo-chord",successor) < 0 ) {
    successor = findNext(mynode);
  }
  else {
  /***********************************/
  /*** Verificar sucesor inmediato ***/
  /* preguntando a mi sucesor si soy yo su antecesor */
    pvm_initsend( PvmDataDefault );
    msg=ANTECESOR; /* peticion de mensaje ANTECESOR */
    pvm_pkint( &msg, 1, 1);
    pvm_send(successor, PETITION ); /* envío de petición */
    //printf("petición ANTECESOR enviada a %d\n",successor);
    info = pvm_trecv( successor, ANTECESOR, &tmout ); /* recepción de petición */
    if ( info > 0 ) {
      //printf("petición atendida por %d.\n",successor);
    pvm_upkint( &nodo_x, 1, 1);
    if( nodo_x == -1) {
      /* Notificar a mi sucesor de que soy su antecesor */
      //printf("notifico a mi sucesor %d, de que yo le precedo\n",successor);
      pvm_initsend( PvmDataDefault );
      msg= ANTECESOR;
      pvm_pkint( &msg, 1, 1); /* mensaje de notificacion tipo antecesor */
      pvm_pkint( &mytid, 1, 1); /* conteniendo mi tid */
      pvm_send( successor, NOTIFY );
    }
    else if( nodo_x != mytid ) {
      /*** ¡Nuevo nodo! ***/
      /* Añadir el nuevo nodo como nuevo sucesor */
      //printf("tengo nuevo (%d) successor! reemplazo a %d\n",nodo_x,successor);
      successor = nodo_x;
      /* Notificar al nuevo nodo de que soy su antecesor */
      pvm_initsend( PvmDataDefault );
      msg= ANTECESOR;
      pvm_pkint( &msg, 1, 1); /* mensaje de notificacion tipo antecesor */
      pvm_pkint( &mytid, 1, 1); /* conteniendo mi tid */
      pvm_send( nodo_x, NOTIFY );
    }
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
int findNext(int nodeOrigin)
{
  int tid_dest, i;

  i= nodeOrigin; 
  do {
    /* Comprobar siguiente en el grupo */
    if(i>=NPROC) i=0;
    else i++;
    /* obtener su tid */
    tid_dest= pvm_gettid("anillo-chord",i);
    /* si el nodo está caido, pasar al siguiente */
  } while( tid_dest == PvmNoInst );

  return tid_dest;
}


/*******************************************/
/** Reportar el estado del nodo al padre,
    quien lo mostrará en la interfaz **/
void reportState()
{
  int pred;
  int succ; 
  int type=0;
  
  if( predecessor == -1) pred=-1;
  else pred= pvm_getinst("anillo-chord",predecessor);
  succ= pvm_getinst("anillo-chord",successor);
  
  //printf("mi predecesor: %d (%d)\n",pred,predecessor);

  pvm_initsend( PvmDataDefault );
  pvm_pkint(&type,1,1);
  pvm_pkint(&mynode,1,1);
  pvm_pkint(&pred,1,1);
  pvm_pkint(&succ,1,1);
  pvm_send(pvm_parent(), REPORT);

}



/*******************************************/
/** Reportar la entrada del nodo al padre,
    quien lo mostrará en la interfaz **/
void reportEntering()
{
  int type=1;
  pvm_initsend( PvmDataDefault );
  pvm_pkint(&type,1,1);
  pvm_pkint(&mynode,1,1);
  pvm_send(pvm_parent(), REPORT);
}


/*******************************************/
/** Reportar la salida del nodo al padre,
    quien lo mostrará en la interfaz **/
void reportExiting()
{
  int type=2;
  pvm_initsend( PvmDataDefault );
  pvm_pkint(&type,1,1);
  pvm_pkint(&mynode,1,1);
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
