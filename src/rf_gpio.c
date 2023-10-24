/*************************************************************
 * 
 *  komuna al ĉiuj protokoloj :
 * Kadra analizo, por protokoloj respektantaj:
 * Kadro enmarcado por du longaj tempoj de sincronización
 * tempo de sinkronigo> 2500μs
 * plej mallonga tempo> 100μs (falsa por Impuls?)
 * almenaŭ 8 signalojn por kadro
 * tempo de sinkronigo> 17 * plej mallonga tempo
 * ne pli ol tri malsamaj daŭroj por la datumoj
 * bitoj koditaj per du pulsadoj
 *
 *********************************************/

#include <lgpio.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <pthread.h>

#include "rf_gpio.h"
#include "sentiloj.h"
#include "protokoloj.h"

uint8_t PakaNumero=0;

char RFUDebug=0;
char RFDebug=0;
char QRFDebug=0;

#define BUFRO_GRANDECO 256

// enigo-pinglo
//static int PIN_EN=2; // wiringPi GPIO 2 (P1.12)
static int PIN_EN=27; //  BCM 27
// lgpio
int Lgpio_h=0;

/*************************************************
 * tabloj celantaj enhavi:
 *  en 0: la daŭro de la lasta pulso antaŭ la sinkronigo
 *  en 1: la daŭro de la sinkronigo
 *  tiam la pulsado-tempoj por la datumoj
 *  kaj laste la tempo de sinkronigo de la fino de la kadro
 */
static unsigned long tempojTemp[BUFRO_GRANDECO+1];
static unsigned long tempoj[BUFRO_GRANDECO+1];

// tablo enhavanta la kodojn de daŭro, kiuj respondas al la tempoj
static unsigned char kodojn[BUFRO_GRANDECO+1];
static unsigned long dauro = 0;
static unsigned long antauaDauro = 0;
static unsigned long antauaMomento = 0;
static unsigned long Sinc = 0;
static unsigned int ringIndex = 0;
static unsigned int sincNb = 0;
static unsigned int Traktenda = 0;
static int affiche=0;

int Verb=0;

void delay (unsigned int howLong)
{
  struct timespec sleeper, dummy ;
 
  sleeper.tv_sec  = (time_t)(howLong / 1000) ;
  sleeper.tv_nsec = (long)(howLong % 1000) * 1000000 ;
 
  nanosleep (&sleeper, &dummy) ;
}
unsigned int micros (void)
{
  uint64_t now ;
  struct  timespec ts ;

  clock_gettime (CLOCK_MONOTONIC_RAW, &ts) ;
  now  = (uint64_t)ts.tv_sec * (uint64_t)1000000 + (uint64_t)(ts.tv_nsec / 1000) ;
  return (uint32_t)(now) ;
}


static int kmpUlong(const void * p1, const void *p2)
{
    return ( *(unsigned long * )p1 > *(unsigned long *)p2 ? 1:0 );
}

void traktilo(int e, lgGpioAlert_p evt, void *data)
{
 long momento = micros();

int i;
   int secs, nanos;

   for (i=0; i<e; i++)
   {
      secs = evt[i].report.timestamp / 1000000000L;
      nanos = evt[i].report.timestamp % 1000000000L;
     
      printf("chip=%d gpio=%d level=%d time=%d.%09d\n",
         evt[i].report.chip, evt[i].report.gpio, evt[i].report.level,
         secs, nanos);
   }



  antauaDauro=dauro;
  dauro = momento - antauaMomento;
  if(Sinc>dauro*15) Sinc=dauro*15;
  antauaMomento = momento;
//printf(" d : %ld\n",dauro);
  if(!sincNb && (dauro >2500) && (dauro< 100000) )
  { // signal long : début de séquence
    //  if(Verb) fprintf(stderr,"départ.\n");
    tempojTemp[sincNb++]=antauaDauro;
    tempojTemp[sincNb++]=dauro;
    if(dauro<10000) Sinc=dauro; else Sinc=17000;
  }
  else if(dauro<60)
  { // anomalio: ni komencas de nulo
    //  if(Verb) fprintf(stderr,"signal trop court.\n");
    sincNb=0;
  }
  else if( dauro>(Sinc*8)/10  && sincNb)
  {// fino de sekvenco
    if(dauro<10000) Sinc=dauro; else Sinc=17000;
    tempojTemp[sincNb++]=dauro;
    if( Traktenda)	// antaŭaj datumoj ankoraŭ ne prilaboritaj
    {
      sincNb=0;
      tempojTemp[sincNb++]=antauaDauro;
      tempojTemp[sincNb++]=dauro;
//      fprintf(stderr,"trame avant fin de traitement précédent.\n");
      return;
    }
    if(sincNb<20) 	// ne sufiĉas datumoj
    {
      //if(Verb) fprintf(stderr,"trame trop courte, lgr=%d Sinc=%ld.\n",sincNb,Sinc);
      sincNb=0;
      tempojTemp[sincNb++]=antauaDauro;
      tempojTemp[sincNb++]=dauro;
      if(dauro<10000) Sinc=dauro; else Sinc=17000;
      return;
    }
    memmove(tempoj,tempojTemp,sincNb*sizeof(long));
    Traktenda=sincNb;
//    struct timespec tp;
//    clock_gettime(CLOCK_REALTIME_COARSE,&tp);
//    printf("\n fin au temps %ld.%02ld \n",tp.tv_sec,tp.tv_nsec/10000000);
    sincNb=0;
    tempojTemp[sincNb++]=antauaDauro;
    tempojTemp[sincNb++]=dauro;
  }
  else if( sincNb>=BUFRO_GRANDECO )
  {// ne eblas trakti, ni provas trakti tion, kio jam ricevis.
//      fprintf(stderr,"trame trop longue.\n");
    memmove(tempoj,tempojTemp,sincNb*sizeof(long));
    Traktenda=sincNb;
    sincNb=0;
  }
  else if(sincNb)
    tempojTemp[sincNb++]=dauro;
}

extern void *tcpServilo(void *);
extern int Haveno;

extern char * iniFile;

struct bufroKadro bufK[100];
int nbBuf=0;

void traite_buf()
{
      nbBuf++;

      if(nbBuf>1)
      {
        if(Verb) printf("  nbBuf=%d;proto=%s;bin=%s\n",nbBuf,bufK[nbBuf-1].proto,bufK[nbBuf-1].bin);
        for(int i=0 ; i < nbBuf-1 ; i++)
        {
          if(bufK[i].unuaTempo < (bufK[nbBuf-1].unuaTempo - 2000))
          {
            memmove(&bufK[i],&bufK[i+1],(nbBuf-i-1)*sizeof(bufK[0]));
            nbBuf--;
            i--;
            continue;
          }
          if(Verb>1) printf("   nb=%d;proto=%s;bin=%s\n",bufK[i].nb,bufK[i].proto,bufK[i].bin);
          if(!strcmp(bufK[i].bin,bufK[nbBuf-1].bin) && !strcmp(bufK[i].proto,bufK[nbBuf-1].proto))
          {
            nbBuf--;
            bufK[i].nb++;
            if(bufK[i].nb == 2)
            {
                if ( !trakto_Kadro(&bufK[i])) // dans sentiloj.c
                { // kadro ne traktita
                  if(strchr(bufK[i].bin,'1')&&strchr(bufK[i].bin,'0'))
		  {
                    printf(" ne traktita :  20;%02X;%s,bitoj=%d,D0=%d,D1=%d",PakaNumero++
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
      PIN_EN=atoi(optarg);
      break;
     case 'l' : // eligo pinglo
      break;
     case 'v' :
      Verb++;
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

  Lgpio_h=lgGpiochipOpen(0);

  antauaMomento = micros();
  lgGpioSetAlertsFunc(Lgpio_h, PIN_EN, traktilo, NULL);
  lgGpioClaimAlert(Lgpio_h, 0, LG_BOTH_EDGES, PIN_EN, -1);

  int raz=20;
  while(true){
    if(Traktenda)
    {
      raz=20;
      struct timespec tp;
      clock_gettime(CLOCK_REALTIME_COARSE,&tp);
      if(Verb) printf("au temps %ld.%02ld, réception de %d pulsations\n",tp.tv_sec,tp.tv_nsec/10000000,Traktenda);
      //if(Verb) printf(" %ld.%02ld;",tp.tv_sec,tp.tv_nsec/10000000);
      if(Traktenda<10)
      {
	goto suite;
      }
      // ordigi tempoj inter sinkronigi
      unsigned long tempojSort[BUFRO_GRANDECO+1];
      memcpy(tempojSort,&tempoj[2],sizeof(unsigned long)*(Traktenda-3));
      qsort(tempojSort,Traktenda-3,sizeof(unsigned long),kmpUlong);
      // kalkulo de averaĝaj daŭroj
      int tempoBase=0;
      int tempoBase0=0;
      int nbEch=0;//nb échantillons
      unsigned long tempoRef=tempojSort[0];
      tempoBase=tempoRef;
      int tempo[5];
      int nbTemps=0;
      for ( int i=1 ; i< Traktenda-3 ; i++)
      {
	if(tempojSort[i]<10) continue;
        if(tempojSort[i]>tempojSort[i-nbEch/2]*3/2 || i==(Traktenda-4))
        { // ni estas je la fino de sekvenco de ekvivalentaj tempoj
//RIPARU MIN : si nbTemps>=2 et nbEch faible (<10), alors on a surement plusieurs trames avec un temps de synchro intermédiaire court (voir Plugin_010)...
          if(nbEch)
          { // ni kalkulas la mediana tempon de ĉi tiu serio
            tempo[nbTemps]=tempojSort[i-nbEch/2];
            nbTemps++;
          }
          tempoRef=tempojSort[i];
          tempoBase=tempoRef;
          nbEch=1;
        }
        else
         { tempoBase += tempojSort[i] ; nbEch++; }
      }
      // RIPARU MIN : uzas tempo 3 kiel tempo de sinkronigo
      // prilaboro kadro kaplinio
      if(tempo[0]<10) goto suite;
      int round=tempoj[0]/tempo[0];
      if ( (tempoj[0]-round*tempo[0]) > (tempo[0]/2) ) round++;
      round=tempoj[1]/tempo[0];
      if ( (tempoj[1]-round*tempo[0]) > (tempo[0]/2) ) round++;
      // prilaboro piedo de kadro
      int DS=0;
      if (Traktenda&1)
      { // oni supozas, ke la piedlinio enhavas nur pulsadon
        round=tempoj[Traktenda-1]/tempo[0];
        if ( (tempoj[Traktenda-1]-round*tempo[0]) > (tempo[0]/2) ) round++;
        DS=tempoj[Traktenda-1];
      }
      else
      { // oni supozas, ke la piedlinio enhavas du pulsadojn
        round=tempoj[Traktenda-2]/tempo[0];
        if ( (tempoj[Traktenda-2]-round*tempo[0]) > (tempo[0]/2) ) round++;
        round=tempoj[Traktenda-1]/tempo[0];
        if ( (tempoj[Traktenda-1]-round*tempo[0]) > (tempo[0]/2) ) round++;
        DS=tempoj[Traktenda-1];
      }
      bufK[nbBuf].D0=tempo[0];
      bufK[nbBuf].D1=tempo[1];
      bufK[nbBuf].D2=tempo[2];
      bufK[nbBuf].DS=DS;
      bufK[nbBuf].salti=0;
      bufK[nbBuf].unuaTempo=tp.tv_sec*1000+tp.tv_nsec/1000000;
      bufK[nbBuf].nb=1;
      // la pulsadoj estas transformitaj en kodojn (0 = mallonga, 1 = dua daŭro, ...)
      int i;
      for ( i=2 ; i< Traktenda ; i++)
      {
        int j=0;
        for ( ; j<nbTemps-1 ; j++)
          if (tempoj[i] < (tempo[j]+tempo[j+1])/2 )
          {
            kodojn[i-2]='0'+j;
            break;
          }
        if(j==nbTemps-1)
          kodojn[i-2]='0'+j;
      }
      kodojn[i-2]=0;
      kodojn[i-1]=0;
      // maintenant, on teste les divers protocoles :
      // ni rigardas ĉu ni havas kodigon 2 kodoj = 1 bito.
      for( int saltiCodes=0 ; saltiCodes<=2 ; saltiCodes++)
      {
        if( testo_p(saltiCodes,kodojn,Traktenda,&bufK[nbBuf]) )
        {
          bufK[nbBuf].salti=saltiCodes;
	  traite_buf();
          bufK[nbBuf].D0=tempo[0];
          bufK[nbBuf].D1=tempo[1];
          bufK[nbBuf].D2=tempo[2];
          bufK[nbBuf].DS=DS;
          bufK[nbBuf].unuaTempo=tp.tv_sec*1000+tp.tv_nsec/1000000;
          bufK[nbBuf].salti=0;
          bufK[nbBuf].nb=1;
	  break;
        }
      }
      if( testo_p001(0,kodojn,Traktenda,&bufK[nbBuf]) )
      {
	  traite_buf();
          bufK[nbBuf].D0=tempo[0];
          bufK[nbBuf].D1=tempo[1];
          bufK[nbBuf].D2=tempo[2];
          bufK[nbBuf].DS=DS;
          bufK[nbBuf].salti=0;
          bufK[nbBuf].unuaTempo=tp.tv_sec*1000+tp.tv_nsec/1000000;
          bufK[nbBuf].nb=1;
      }
suite:
      Traktenda=0;
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
