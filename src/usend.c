/* 
 */

#include <wiringPi.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include "rf_gpio.h"
#include "sentiloj.h"
#include "protokoloj.h"

void sendiParon(int Tempo1,int Tempo2);

void tcp_sendiCxiuj(const char *buffer,int len)
{
}

uint8_t PakaNumero=0;

// eligo-pinglo
extern int PIN_EL;  // wiringPi GPIO 0 

extern char *optarg;
extern int optind, opterr, optopt;

struct sentilo Sentilo;
int sendo_kadro(char * buffer, struct sentilo * sentilo);

void uzo()
{
  printf(" ekzemple : usend -P p0110,D0=150,D1=490,DS=4800 011100000011110111100011\n");
}

int main(int argc, char *argv[]){
 int nbr=6;
  int opt;
  int verb=0;// Paroleco
  memset(&Sentilo,0,sizeof(Sentilo));
  char proto[10]="p0110";
  Sentilo.proto=proto;
  while( (opt=getopt(argc,argv,"?vn:p:P:")) != -1)
  {
    switch(opt)
    {
     case '?' : 
      uzo();
      break;
     case 'n' : // nombro de ripetoj
      Sentilo.nbKadro=atoi(optarg);
      break;
     case 'p' : // pinglo 
      PIN_EL=atoi(optarg);
      break;
     case 'P' : // protokolo
      {
       char * savtok=0;
       char * strnom=strtok_r(optarg,",",&savtok);
       Sentilo.proto=strdup(strnom);
       while(strnom=strtok_r(0,",",&savtok))
       {
         char *ptr=strstr(strnom,"=");
         int val = atoi(ptr+1);
         if(!memcmp(strnom,"D0",2))
	   Sentilo.D0=val;
         else if(!memcmp(strnom,"D1",2))
   	   Sentilo.D1=val;
         else if(!memcmp(strnom,"D2",2))
	   Sentilo.D2=val;
         else if(!memcmp(strnom,"DS",2))
	   Sentilo.DS=val;
         else if(!memcmp(strnom,"bitoj",4))
	   Sentilo.nbBitoj=val;
         else if(!memcmp(strnom,"salti",4))
	   Sentilo.salti=val;
       }
      }
      break;
     case 'v' : 
      verb++;
      break;
    }
  }
  if(optind==argc)
  {
    uzo();
    exit(1);
  }
  if (!Sentilo.D0) Sentilo.D0=500;
  if (!Sentilo.D1) Sentilo.D1=3*Sentilo.D0;
  if (!Sentilo.D2) Sentilo.D2=2*Sentilo.D1;
  if (!Sentilo.DS) Sentilo.DS=10*Sentilo.D1;
  if(verb)
  {
    printf(" proto : %s , D0=%d, D1=%d, D2=%d, DS=%d\n",Sentilo.proto,Sentilo.D0,Sentilo.D1,Sentilo.D2,Sentilo.DS);
  }

  if(wiringPiSetup() == -1){
    printf("ne detektis «wiring pi»\n");
    return 0;
  }
  if ( strlen(argv[optind]) >300)
  {
    fprintf(stderr," Datumoj tro longe (300 maksimumo).\n");
    exit(1);
  }
  sendo_kadro(argv[optind], &Sentilo);

  dormi( 100000);
  
  exit(0);
}
