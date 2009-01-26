/*
 *    Trabajo PVM: Chord
 *	  Ampliación de Sistemas Operativos
 *	  Fernando Carmona Varo
 *    Rafael Jesús García del Pino
 * 
 *    Archivo: nodo.c
 * 	  Fecha 23/01/2009
 */

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include "pvm3.h"
#include "macros.h"

/* Funcion mainloop
 * No devuelve parametros, no recibe parametros
 * Realiza las rutinas principales de cada nodo, tales como 
 * atender peticiones o enviar peticiones a nodos contiguos
 */
void mainloop( );

/* Funcion pfound
 * No devuelve parametros, recibe un token para atender su peticion
 * Atiende la peticion del token que se le pasa como parametro
 */
void pfound(int);

/* Funcion findNext
 * Devuelve el nodo activo siguiente al nodo
 * que se le pase como parametro
 */
int findNext(int);

/* Funcion stabilize
 * No recibe parametros, no devuelve parametros
 * Estabiliza el nodo
 * Verifica que el nodo tiene su sucesor y predecesor activos
 */
void stabilize( );

/* Funcion reportState
 * Envia informacion acerca del estado del nodo al proceso padre
 */
void reportState();

/* Funcion reportEntering
 * Envia informacion acerca de la entrada de un nodo al anillo
 * al proceso padre
 */
void reportEntering();

/* Funcion reportExiting
 * Envia informacion acerca de la salida de un nodo del anillo
 * al proceso padre
 */
void reportExiting();

/* Funcion reportKeyInit
 * Envia informacion sobre el inicio de busqueda de una clave
 * al proceso padre
 */
void reportKeyInit(int);

/* Funcion reportKeyTrans
 * Envia informacion sobre la transmision de una clave
 * para su busqueda a través del anillo al proceso padre
 */
void reportKeyTrans(int);

/* Funcion reportKeyFound
 * Envia informacion acerca de el encuentro de una clave en el anillo
 * al proceso padre
 */
void reportKeyFound(int);

int mytid;                  /* mi task id */
int mynode;                 /* mi numero en el anillo */
int predecessor;
int successor;

int pending_keys;



int main(int argc, char *argv[])
{
  int i;
  int life;

  /* identificarme en pvm */
  mytid = pvm_mytid();

  /* Unirme al grupo  */
  mynode = pvm_joingroup( "anillo-chord" );


  /* Aleatoriamente se cerraría el nodo (dejamos huecos en el anillo)  */
  srand(mytid+mynode);


    /******************************************/
    /* Procesos dentro del anillo */

    /* indicar la entrada del nodo en el anillo */
    reportEntering();

    /*********************/
    /* Creación/Unión al anillo */
    /* Inicialmente el predecesor se toma como inexistente */
    predecessor = -1;
    successor = findNext(mynode);

    reportState();
    
    /***************************************/
    /* El nodo trabajará durante un numero aleatorio de ciclos */
    if( rand()*NPROC > RAND_MAX/10 ) 
      life= 10.0*rand()/RAND_MAX; 
    else 
      life= 100.0*rand()/RAND_MAX; 
    

    for(i=0; i<life; i++) {
      mainloop();
    }
    

  /******************************************/
  /* Procesos a expulsar del anillo  */  

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
  
  /**************************/
  /*** Atender peticiones ***/
  info = pvm_trecv( -1, PETITION, &tmout );
  if ( info > 0 ) {
    pvm_bufinfo(info, NULL, NULL, &sender);
    pvm_upkint( &msg, 1, 1);
    pvm_initsend( PvmDataDefault );
    switch(msg) {
    case ANTECESOR: /* peticion de predecesor */
      pvm_pkint( &predecessor, 1, 1);
      break;
    case SUCCESSOR: /* Petición de sucesor */
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
      break;

    case KEY: /* Notificación de búsqueda de clave */
      pvm_upkint(&token,1,1);
      if( K_VALUE <= mynode-token) { /* si tiene el token, atiende la peticion */
        pfound(token);
      }
      else { /* de lo contrario transmite la notificación por el anillo */
        pvm_initsend( PvmDataDefault );
        pvm_pkint(&msg,1,1);
        pvm_pkint(&token,1,1);
        pvm_send(sender, NOTIFY );
        reportKeyTrans(token);
      }
      break;
    }
  }

  if( rand() < RAND_MAX/80 ) {  
    token=( (double) rand()*NPROC )/( (double) RAND_MAX);
    msg=KEY;
    pvm_initsend( PvmDataDefault );
    pvm_pkint(&msg,1,1);
    pvm_pkint(&token,1,1);
    pvm_send(successor, NOTIFY );
    reportKeyInit(token);
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
    info = pvm_trecv( successor, ANTECESOR, &tmout ); /* recepción de petición */
    if ( info > 0 ) {
      pvm_upkint( &nodo_x, 1, 1);    
      if( pvm_getinst("anillo_chord",nodo_x) < 0) {
        /* Si mi sucesor no tiene antecesor, notificarle que soy yo */
        pvm_initsend( PvmDataDefault );
        msg= ANTECESOR;
        pvm_pkint( &msg, 1, 1); /* mensaje de notificacion tipo antecesor */
        pvm_pkint( &mytid, 1, 1); /* conteniendo mi tid */
        pvm_send( successor, NOTIFY );
      }
    else if( nodo_x != mytid ) {
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
  }
}


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
  int type=STATE;
  
  if( predecessor == -1) pred=-1;
  else pred= pvm_getinst("anillo-chord",predecessor);
  succ= pvm_getinst("anillo-chord",successor);
  
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
  int type=ENTER;
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
  int type=EXIT;
  pvm_initsend( PvmDataDefault );
  pvm_pkint(&type,1,1);
  pvm_pkint(&mynode,1,1);
  pvm_send(pvm_parent(), REPORT);
}

/*******************************************/
/** Reportar la busqueda de una clave al nodo padre,
    quien lo mostrará en la interfaz **/
void reportKeyInit(int token)
{
  int type=KEYINIT;
  pvm_initsend( PvmDataDefault );
  pvm_pkint(&type,1,1);
  pvm_pkint(&mynode,1,1);
  pvm_pkint(&token,1,1);
  pvm_send(pvm_parent(), REPORT);
}

/*******************************************/
/** Reportar la salida del nodo al padre,
    quien lo mostrará en la interfaz **/
void reportKeyTrans(int token)
{
  int type=KEYTRANS;
  pvm_initsend( PvmDataDefault );
  pvm_pkint(&type,1,1);
  pvm_pkint(&mynode,1,1);
  pvm_pkint(&token,1,1);
  pvm_send(pvm_parent(), REPORT);
}


/*******************************************/
/** Reportar la salida del nodo al padre,
    quien lo mostrará en la interfaz **/
void reportKeyFound(int token)
{
  int type=KEYFOUND;
  pvm_initsend( PvmDataDefault );
  pvm_pkint(&type,1,1);
  pvm_pkint(&mynode,1,1);
  pvm_pkint(&token,1,1);
  pvm_send(pvm_parent(), REPORT);
}

 
/************************
 Atender la peticion del token
*/ 
void pfound(int token)
{  
  reportKeyFound(token);
} 
