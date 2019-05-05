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

int main(int argc, char *argv[]){
 int nbr=6;
  int opt;
  memset(&Sentilo,0,sizeof(Sentilo));
  char proto[10]="p0110";
  Sentilo.proto=proto;
  while( (opt=getopt(argc,argv,"?h:p:P:d:D:2:S:")) != -1)
  {
    switch(opt)
    {
     case '?' : 
      printf(" ekzemple : bsend -P p0102 -d 560 -D 1890 -2 3846 -S 9000 010111001011100000001110001100100111\n");
      break;
     case 'n' : // nombro de ripetoj
      Sentilo.nbKadro=atoi(optarg);
      break;
     case 'p' : // pinglo 
      PIN_EL=atoi(optarg);
      break;
     case 'P' : // protokolo
        strcpy(proto,optarg);
	if(optarg[0] != 'p' && optarg[0] != 't')
	{
	  printf("protokolo nekonata %s\n",optarg);
	  exit(1);
	}
      break;
     case 'd' : // Daŭro 0
      Sentilo.D0=atoi(optarg);
      break;
     case 'D' : // Daŭro 1
      Sentilo.D1=atoi(optarg);
      break;
     case '2' : // Daŭro 2
      Sentilo.D2=atoi(optarg);
      break;
     case 'S' : // Daŭro Sinkronigo
      Sentilo.DS=atoi(optarg);
      break;
    }
  }
  if (!Sentilo.D0) Sentilo.D0=500;
  if (!Sentilo.D1) Sentilo.D1=3*Sentilo.D0;
  if (!Sentilo.D2) Sentilo.D2=2*Sentilo.D1;
  if (!Sentilo.DS) Sentilo.DS=10*Sentilo.D1;

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
