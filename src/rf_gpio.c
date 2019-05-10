/*************************************************************
 * 
 *  komuna al ĉiuj protokoloj :
trame encadrée par deux temps longs de synchronisation
temps de synchro > 2500µs
temps de base > 100µs
signal le plus long (hors synchro) < 9 temps de base
au moins 8 signaux par trame
 *********************************************/

#include <wiringPi.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <time.h>
#include <pthread.h>
#include <unistd.h>

#include "rf_gpio.h"
#include "sentiloj.h"
#include "protokoloj.h"

uint8_t PakaNumero=0;

char RFUDebug=0;
char RFDebug=0;
char QRFDebug=0;

#define BUFFER_SIZE 256

// enigo-pinglo
#define PIN_EN 2  // wiringPi GPIO 2 (P1.12)

/*************************************************
 *  tableaux destinés à contenir :
 * en 0 : la durée de la dernière pulsation avant la synchro
 * en 1 : la durée de la synchronisation
 * puis les durées des pulsations pour les données
 * et en dernier le temps de la synchro de fin de trame
 */
static unsigned long timingsTemp[BUFFER_SIZE+1];
static unsigned long timings[BUFFER_SIZE+1];

static char codes[BUFFER_SIZE+1];
static unsigned long duration = 0;
static unsigned long oldDuration = 0;
static unsigned long lastTime = 0;
static unsigned long Sync = 0;
static unsigned int ringIndex = 0;
static unsigned int syncCount = 0;
static unsigned int ATraiter = 0;
static int affiche=0;

int debug=0;


void handler()
{
 int valeur=digitalRead(PIN_EN);
 long time = micros();
  oldDuration=duration;
  duration = time - lastTime;
  if(Sync>duration*15) Sync=duration*15;
  lastTime = time;
//printf(" d : %ld\n",duration);
  if(!syncCount && (duration >1500) && (duration< 100000) )
  { // signal long : début de séquence
    //  if(debug) fprintf(stderr,"départ.\n");
    timingsTemp[syncCount++]=oldDuration;
    timingsTemp[syncCount++]=duration;
    if(duration<10000) Sync=duration; else Sync=17000;
  }
  else if(duration<80)
  { // anomalie : on repart à zéro
    //  if(debug) fprintf(stderr,"signal trop court.\n");
    syncCount=0;
  }
  else if( duration>(Sync*8)/10  && syncCount)
  {// fin de séquence 
    if(duration<10000) Sync=duration; else Sync=17000;
    timingsTemp[syncCount++]=duration;
    if( ATraiter)	// données précédentes pas encore traitées
    {
      syncCount=0;
      timingsTemp[syncCount++]=oldDuration;
      timingsTemp[syncCount++]=duration;
//      fprintf(stderr,"trame avant fin de traitement précédent.\n");
      return;
    }
    if(syncCount<20) 	// pas assez de données
    {
      //if(debug) fprintf(stderr,"trame trop courte, lgr=%d Sync=%ld.\n",syncCount,Sync);
      syncCount=0;
      timingsTemp[syncCount++]=oldDuration;
      timingsTemp[syncCount++]=duration;
      if(duration<10000) Sync=duration; else Sync=17000;
      return;
    }
    memmove(timings,timingsTemp,syncCount*sizeof(long));
    ATraiter=syncCount;
//    struct timespec tp;
//    clock_gettime(CLOCK_REALTIME_COARSE,&tp);
//    printf("\n fin au temps %ld.%02ld \n",tp.tv_sec,tp.tv_nsec/10000000);
    syncCount=0;
    timingsTemp[syncCount++]=oldDuration;
    timingsTemp[syncCount++]=duration;
  }
  else if( syncCount>=BUFFER_SIZE )
  {// pas possible à traiter, on essaie de traiter ce qui est déjà reçu.
//      fprintf(stderr,"trame trop longue.\n");
    memmove(timings,timingsTemp,syncCount*sizeof(long));
    ATraiter=syncCount;
    syncCount=0;
  }
  else if(syncCount)
    timingsTemp[syncCount++]=duration;
}

extern void *tcpServilo(void *);
extern int Haveno;

static int cmpUlong(const void * p1, const void *p2)
{
    return ( *(unsigned long * )p1 > *(unsigned long *)p2 ? 1:0 );
}
extern char * iniFile;

struct bufroKadro bufK[100];
int nbBuf=0;

void traite_buf()
{
      nbBuf++;

      if(nbBuf>1)
      {
        if(debug) printf("  nbBuf=%d;proto=%s;bin=%s\n",nbBuf,bufK[nbBuf-1].proto,bufK[nbBuf-1].bin);
        for(int i=0 ; i < nbBuf-1 ; i++)
        {
          if(bufK[i].unuaTempo < (bufK[nbBuf-1].unuaTempo - 10000))
          {
            memmove(&bufK[i],&bufK[i+1],(nbBuf-i-1)*sizeof(bufK[0]));
            nbBuf--;
            i--;
            continue;
          }
          if(debug) printf("   nb=%d;proto=%s;bin=%s\n",bufK[i].nb,bufK[i].proto,bufK[i].bin);
          if(!strcmp(bufK[i].bin,bufK[nbBuf-1].bin) && !strcmp(bufK[i].proto,bufK[nbBuf-1].proto))
          {
            nbBuf--;
            bufK[i].nb++;
            if(bufK[i].nb == 2)
            {
                if ( !trakto_Kadro(&bufK[i])) // dans sentiloj.c
                {
                  if(strchr(bufK[i].bin,'1')&&strchr(bufK[i].bin,'0'))
		  {
                    printf("20;%02X;%s,bitoj=%d,D0=%d,D1=%d",PakaNumero++
                        ,bufK[i].proto
                        ,bufK[i].nbBitoj,bufK[i].D0,bufK[i].D1);
		    if(bufK[i].D2>1 && strchr(bufK[i].proto,'2'))
                      printf(",D2=%d",bufK[i].D2);
		    if(bufK[i].salti>0)
                      printf(",salti=%d",bufK[i].salti);
                    printf(",DS=%d;binary=%s,hex=" ,bufK[i].DS ,bufK[i].bin);
		    for(int bit=0 ; bit<bufK[i].nbBitoj ; bit+=4)
		    {
		      uint8_t hex=0;
		      for(int j=0 ; j<4 ; j++)
		      {
			hex = hex<<1;
		        if(bufK[i].bin[bit+j]=='1') hex |=1;
		      }
		      printf("%x",hex);
		    }
		    printf(";\n");
		  }
                }
            }
          }
        }
      }

}

int main(int argc, char *argv[])
{
  int opt;
  int demono=0;
  while( (opt=getopt(argc,argv,"dh:n:l:vi:")) != -1)
  {
    switch(opt)
    {
     case 'h' : // TCP haveno
      Haveno=atoi(optarg);
      break;
     case 'd' : 
	demono=1;
      break;
     case 'i' :
	iniFile = optarg;
      break;
     case 'n' : // enigo pinglo
      break;
     case 'l' : // eligo pinglo
      break;
     case 'v' :
	debug++;
      break;
    }
  }
  int pid;
  if(demono)
  {
    pid = fork();
    if (pid < 0)
      exit(EXIT_FAILURE);
    if (pid > 0)
      exit(EXIT_SUCCESS);
  }

  
  sentilo_legi();

  //start TCP server in a new thread :
  pthread_t server_thread;
  if(pthread_create(&server_thread,NULL,tcpServilo, NULL) < 0)
  {
    perror("could not create thread");
    exit(1);
  }


  if(wiringPiSetup() == -1)
  {
    printf("no wiring pi detected\n");
    return 0;
  }
  lastTime = micros();
  wiringPiISR(PIN_EN,INT_EDGE_BOTH,&handler);
  int raz=20;
  while(true){
    if(ATraiter)
    {
      raz=20;
      struct timespec tp;
      clock_gettime(CLOCK_REALTIME_COARSE,&tp);
      if(debug) printf("au temps %ld.%02ld, réception de %d pulsations\n",tp.tv_sec,tp.tv_nsec/10000000,ATraiter);
      //if(debug) printf(" %ld.%02ld;",tp.tv_sec,tp.tv_nsec/10000000);
      if(ATraiter<10)
      {
	goto suite;
      }
      // tri des timings autres que les synchros :
      unsigned long timingsSort[BUFFER_SIZE+1];
      memcpy(timingsSort,&timings[2],sizeof(unsigned long)*(ATraiter-3));
      qsort(timingsSort,ATraiter-3,sizeof(unsigned long),cmpUlong);
      // calcul des temps moyens
      int tempsBase=0;
      int tempsBase0=0;
      int nbEch=0;//nb échantillons
      unsigned long tempsRef=timingsSort[0];
      tempsBase=tempsRef;
      int temps[5];
      int nbTemps=0;
      for ( int i=1 ; i< ATraiter-3 ; i++)
      {
	if(timingsSort[i]<10) continue;
        if(timingsSort[i]>timingsSort[i-1]*3/2 || i==(ATraiter-4))
        { // on est à la fin d'une suite de temps équivalents
//RIPARU MIN : si nbTemps>=2 et nbEch faiale (<10), alors on a surement plusieurs trames avec un temps de synchro intermédiaire court (voir Plugin_010)...
          if(nbEch)
          { // on calcule le temps moyen de cette suite
            tempsBase /= nbEch;
            temps[nbTemps]=tempsBase;
            nbTemps++;
          }
          tempsRef=timingsSort[i];
          tempsBase=tempsRef;
          nbEch=1;
        }
        else
         { tempsBase += timingsSort[i] ; nbEch++; }
      }
      // RIPARU MIN : utilisation du temps 3 comme temps de synchro
      // traitement du header
      if(temps[0]<10) goto suite;
      int round=timings[0]/temps[0];
      if ( (timings[0]-round*temps[0]) > (temps[0]/2) ) round++;
      round=timings[1]/temps[0];
      if ( (timings[1]-round*temps[0]) > (temps[0]/2) ) round++;
      // traitement du footer
      int DS=0;
      if (ATraiter&1)
      { // on suppose que le footer ne contient qu'une pulsation
        round=timings[ATraiter-1]/temps[0];
        if ( (timings[ATraiter-1]-round*temps[0]) > (temps[0]/2) ) round++;
        DS=timings[ATraiter-1];
      }
      else
      { // on suppose que le footer contient deux pulsations
        round=timings[ATraiter-2]/temps[0];
        if ( (timings[ATraiter-2]-round*temps[0]) > (temps[0]/2) ) round++;
        round=timings[ATraiter-1]/temps[0];
        if ( (timings[ATraiter-1]-round*temps[0]) > (temps[0]/2) ) round++;
        DS=timings[ATraiter-1];
      }
      bufK[nbBuf].D0=temps[0];
      bufK[nbBuf].D1=temps[1];
      bufK[nbBuf].D2=temps[2];
      bufK[nbBuf].DS=DS;
      bufK[nbBuf].salti=0;
      bufK[nbBuf].unuaTempo=tp.tv_sec*1000+tp.tv_nsec/1000000;
      bufK[nbBuf].nb=1;
      // on transforme les pulsations en codes (0=court, 1= 2e durée, ...)
      int i;
      for ( i=2 ; i< ATraiter ; i++)
      {
        int j=0;
        for ( ; j<nbTemps-1 ; j++)
          if (timings[i] < (temps[j]+temps[j+1])/2 )
          {
            codes[i-2]='0'+j;
            break;
          }
        if(j==nbTemps-1)
          codes[i-2]='0'+j;
      }
      codes[i-2]=0;
      codes[i-1]=0;
      // maintenant, on teste les divers protocoles :
      // on regarde si on a un codage de type 2 codes = 1 bit.
      for( int saltiCodes=0 ; saltiCodes<=2 ; saltiCodes++)
      {
        if( testo_p(saltiCodes,codes,ATraiter,&bufK[nbBuf]) )
        {
          bufK[nbBuf].salti=saltiCodes;
	  traite_buf();
          bufK[nbBuf].D0=temps[0];
          bufK[nbBuf].D1=temps[1];
          bufK[nbBuf].D2=temps[2];
          bufK[nbBuf].DS=DS;
          bufK[nbBuf].unuaTempo=tp.tv_sec*1000+tp.tv_nsec/1000000;
          bufK[nbBuf].salti=0;
          bufK[nbBuf].nb=1;
	  break;
        }
      }
      if( testo_p001(0,codes,ATraiter,&bufK[nbBuf]) )
      {
	  traite_buf();
          bufK[nbBuf].D0=temps[0];
          bufK[nbBuf].D1=temps[1];
          bufK[nbBuf].D2=temps[2];
          bufK[nbBuf].DS=DS;
          bufK[nbBuf].salti=0;
          bufK[nbBuf].unuaTempo=tp.tv_sec*1000+tp.tv_nsec/1000000;
          bufK[nbBuf].nb=1;
      }
suite:
      ATraiter=0;
      delay(2);
    }
    else
    {
      if (--raz <=0) {raz=20;nbBuf=0;}
      delay(20);
    }
  }
  exit(0);
}
