/*************************************************************
 * 
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

#include <wiringPi.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <signal.h>

#define BUFRO_GRANDECO 256

int Verb=0;
FILE * FicTrace=NULL;
FILE * FicIn=NULL;

// enigo-pinglo
static int PIN_EN=2; // wiringPi GPIO 2 (P1.12)

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
static unsigned int sincNb = 0;
static unsigned int Traktenda = 0;

static int kmpUlong(const void * p1, const void *p2)
{
    return ( *(unsigned long * )p1 > *(unsigned long *)p2 ? 1:0 );
}

int nb2=0;
unsigned long tempoj2[24];
int val2=0;
char buffer2[150];
struct timespec tp2;

struct timespec tp0;
struct timespec tp;

void trakti();



void traktilo()
{
 long momento = micros();
 if(!nb2)
  {
    val2=digitalRead(PIN_EN);
    clock_gettime(CLOCK_REALTIME_COARSE,&tp2);
  }
  antauaDauro=dauro;
  dauro = momento - antauaMomento;
  antauaMomento = momento;
 trakti();
}

void trakti()
{
  if(Sinc>dauro*15) Sinc=dauro*15;
  tempoj2[nb2++]=dauro;
  if(nb2==24)
  {
    nb2=0;
    if( Verb >=3)
    {
      sprintf(buffer2,"T=%ld.%02ld,v1=%d,%5d,%5d,%5d,%5d,%5d,%5d,%5d,%5d,%5d,%5d,%5d,%5d,%5d,%5d,%5d,%5d,%5d,%5d,%5d,%5d,%5d,%5d,%5d,%5d"
	,tp2.tv_sec,tp2.tv_nsec/10000000,val2
	,tempoj2[0],tempoj2[1],tempoj2[2],tempoj2[3],tempoj2[4],tempoj2[5],tempoj2[6],tempoj2[7],tempoj2[8],tempoj2[9]
	,tempoj2[10],tempoj2[11],tempoj2[12],tempoj2[13],tempoj2[14],tempoj2[15],tempoj2[16],tempoj2[17],tempoj2[18],tempoj2[19]
	,tempoj2[20],tempoj2[21],tempoj2[22],tempoj2[23]);
      puts(buffer2);
    }
    if(FicTrace)
    {
      fprintf(FicTrace,"%5d,%5d,%5d,%5d,%5d,%5d,%5d,%5d,%5d,%5d,%5d,%5d,%5d,%5d,%5d,%5d,%5d,%5d,%5d,%5d,%5d,%5d,%5d,%5d\n"
	,tempoj2[0],tempoj2[1],tempoj2[2],tempoj2[3],tempoj2[4],tempoj2[5],tempoj2[6],tempoj2[7],tempoj2[8],tempoj2[9]
	,tempoj2[10],tempoj2[11],tempoj2[12],tempoj2[13],tempoj2[14],tempoj2[15],tempoj2[16],tempoj2[17],tempoj2[18],tempoj2[19]
	,tempoj2[20],tempoj2[21],tempoj2[22],tempoj2[23]);
    }
  }
//printf(" d : %ld\n",dauro);
  if(!sincNb && (dauro >2500) && (dauro< 100000) )
  { // signal long : début de séquence
    clock_gettime(CLOCK_REALTIME_COARSE,&tp0);
    tempojTemp[sincNb++]=antauaDauro;
    tempojTemp[sincNb++]=dauro;
    if(dauro<10000) Sinc=dauro; else Sinc=17000;
  }
  else if(dauro<60)
  { // anomalio: ni komencas de nulo
    sincNb=0;
  }
  else if( dauro>(Sinc*8)/10  && sincNb)
  {// fino de sekvenco
    memmove(&tp,&tp0,sizeof(tp));
    clock_gettime(CLOCK_REALTIME_COARSE,&tp0);
    if(dauro<10000) Sinc=dauro; else Sinc=17000;
    tempojTemp[sincNb++]=dauro;
    if( Traktenda)	// antaŭaj datumoj ankoraŭ ne prilaboritaj
    {
      sincNb=0;
      tempojTemp[sincNb++]=antauaDauro;
      tempojTemp[sincNb++]=dauro;
      return;
    }
    if(sincNb<20) 	// ne sufiĉas datumoj
    {
      sincNb=0;
      if(dauro<10000) Sinc=dauro; else Sinc=17000;
      tempojTemp[sincNb++]=antauaDauro;
      tempojTemp[sincNb++]=dauro;
      return;
    }
    memmove(tempoj,tempojTemp,sincNb*sizeof(long));
    Traktenda=sincNb;
    sincNb=0;
    tempojTemp[sincNb++]=antauaDauro;
    tempojTemp[sincNb++]=dauro;
  }
  else if ( sincNb>=BUFRO_GRANDECO )
  {// ne eblas trakti, ni provas trakti tion, kio jam ricevis.
    memmove(tempoj,tempojTemp,sincNb*sizeof(long));
    Traktenda=sincNb;
    sincNb=0;
  }
  else if(sincNb)
    tempojTemp[sincNb++]=dauro;
}

extern char *optarg;
extern int optind, opterr, optopt;


int main(int argc, char *argv[]){

  int opt;
  while( (opt=getopt(argc,argv,"vp:t:i:")) != -1)
  {
    switch(opt)
    {
     case 'i' :
      FicIn = fopen(optarg,"r");
      break;
     case 't' :
      FicTrace = fopen(optarg,"w");
      break;
     case 'p' :
      PIN_EN=atoi(optarg);
      break;
     case 'v' :
      Verb++;
      break;
    }
  }


  if(wiringPiSetup() == -1){
    printf("no wiring pi detected\n");
    return 0;
  }
  antauaMomento = micros();
  if(!FicIn)
    wiringPiISR(PIN_EN,INT_EDGE_BOTH,&traktilo);
  while( 1 ){
    if(Traktenda)
    {
      if(FicTrace) fflush(FicTrace);
      if(Verb) printf("en momento %ld.%02ld, ricevo de %d pulsadoj\n",tp.tv_sec,tp.tv_nsec/10000000,Traktenda);
      if(Verb >=2)
      {
        printf(" pulsadoj : ");
        for ( int i=0 ; i< Traktenda ; i++)
	{
          printf("%d,",tempoj[i]);
	}
        printf("\n");
      }
      // ordigi tempoj inter sinkronigi
      unsigned long tempojSort[BUFRO_GRANDECO+1];
      memcpy(tempojSort,&tempoj[2],sizeof(unsigned long)*(Traktenda-3));
      qsort(tempojSort,Traktenda-3,sizeof(unsigned long),kmpUlong);
      // kalkulo de averaĝaj daŭroj
      int tempoBase=0;
      int tempoBase0=0;
      int nbEch=0;// nombro de specimenoj
      unsigned long tempoRef=tempojSort[0];
      tempoBase=tempoRef;
      if(Verb) printf(" tempoj : ");
      int tempo[5];
      int nbTemps=0;
      for ( int i=1 ; i< Traktenda-3 ; i++)
      {
        if(tempojSort[i]>tempojSort[i-nbEch/2]*3/2 || i==(Traktenda-4))
	{ // ni estas je la fino de sekvenco de ekvivalentaj tempoj
          if(nbEch)
	  { // ni kalkulas la mezan tempon de ĉi tiu serio
	    tempoBase /= nbEch;
	    //tempo[nbTemps]=tempoBase;
	    int medianTempo=tempojSort[i-nbEch/2];
	    tempo[nbTemps]=medianTempo;
	    if(Verb) printf("  daŭro %d = %d-%d (%d) ;",nbTemps,tempoBase,medianTempo,nbEch);
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
      if(Verb) printf("\n header : %dT",round);
      round=tempoj[1]/tempo[0];
      if ( (tempoj[1]-round*tempo[0]) > (tempo[0]/2) ) round++;
      if(Verb) printf("-%dT ",round);
      // prilaboro piedo de kadro
      int DS=0;
      if (Traktenda&1)
      { // oni supozas, ke la piedlinio enhavas nur pulsadon
        round=tempoj[Traktenda-1]/tempo[0];
        if ( (tempoj[Traktenda-1]-round*tempo[0]) > (tempo[0]/2) ) round++;
        if(Verb) printf("footer : %dT\n",round);
	DS=tempoj[Traktenda-1];
      }
      else
      { // oni supozas, ke la piedlinio enhavas du pulsadojn
        round=tempoj[Traktenda-2]/tempo[0];
        if ( (tempoj[Traktenda-2]-round*tempo[0]) > (tempo[0]/2) ) round++;
        if(Verb) printf("footer : %dT",round);
        round=tempoj[Traktenda-1]/tempo[0];
        if ( (tempoj[Traktenda-1]-round*tempo[0]) > (tempo[0]/2) ) round++;
        if(Verb) printf("-%dT\n",round);
	DS=tempoj[Traktenda-1];
      }
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
      if (Verb) printf(" tempo-datumoj : %s\n",kodojn);
      // ni rigardas ĉu ni havas kodigon 2 kodoj = 1 bito.
      char code0[3],code1[3];
      memcpy(code0,kodojn,2); code0[2]=0;
      for(i=0 ; i < Traktenda-2 && kodojn[i] ; i+=2)
        if(memcmp(&kodojn[i],code0,2)) break;
      if(i==Traktenda-2 || !kodojn[i])
      {
	if(Verb) printf("ignorita kadro (nur 0)..\n");
	goto suite;
      }
      memcpy(code1,kodojn+i,2); code1[2]=0;
      for( ; i < Traktenda-2 && kodojn[i] ; i+=2)
        if(memcmp(&kodojn[i],code0,2) && memcmp(&kodojn[i],code1,2)) break;
      //if(i < Traktenda/2)
      int skipCodes=0;
      char bitoj[BUFRO_GRANDECO];
      int nbBit=0;
      if(i < Traktenda-4)
      {
        // provu kun tempo post sinkronigo
        memcpy(code0,kodojn+1,2); code0[2]=0;
        for(i=1 ; i < Traktenda-2 && kodojn[i] ; i+=2)
          if(memcmp(&kodojn[i],code0,2)) break;
        memcpy(code1,kodojn+i,2); code1[2]=0;
        for( ; i < Traktenda-2 && kodojn[i] ; i+=2)
          if(memcmp(&kodojn[i],code0,2) && memcmp(&kodojn[i],code1,2)) break;
        if(i >= Traktenda-4)
	{
	  skipCodes=1;
	}
	else
	{
          //RIPARU MIN : provi p001
          // tradukado de datumoj en bitoj:
          for(int k=0 ; k < Traktenda-3 && kodojn[k] ; k++,nbBit++)
            if(kodojn[k]=='0' && kodojn[k+1]=='0')
	    {
              bitoj[nbBit]='0';
	      k++;
	    }
            else if(kodojn[k]=='1')
	    {
              bitoj[nbBit]='1';
	    }
            else
	    {
              if( k< (Traktenda -4))
	      {
                if(Verb) printf("ignorita kadro (nekonata kodoprezento)..\n");
                goto suite;
	      }
	      nbBit--;
	    }
          bitoj[nbBit]=0;
// apliki la kodon manchester:
  char oldbit='1';
  for ( int i=0 ; i<nbBit ; i++)
  {
    if(bitoj[i] == '1')
    {
      if (oldbit=='0') oldbit='1';
      else oldbit='0';
    }
    bitoj[i] = oldbit;
  }
          printf(" %d pulsoj, protokolo : \"xxx;p001,bitoj=%d,D0=%d,D1=%d",Traktenda-3,nbBit,tempo[0],tempo[1]);
	  goto trt;
	}
      }
      // ni supozas, ke la plej malgranda kodo korespondas al bito 0
      if(strcmp(code0,code1) >0)
      {
        char buf[2];
	memcpy(buf,code0,2);
	memcpy(code0,code1,2);
	memcpy(code1,buf,2);
      }
      // tradukado de datumoj en bitoj:
      for(i=skipCodes ; i < Traktenda-4 && kodojn[i] ; i+=2)
        if(!memcmp(&kodojn[i],code0,2))
	  bitoj[i/2]='0';
	else
	  bitoj[i/2]='1';
      bitoj[i/2]=0;
      nbBit = i/2;
      printf(" %d pulsoj, protokolo : \"xxx;p%s%s,bitoj=%d,D0=%d,D1=%d",Traktenda-3,code0,code1,nbBit,tempo[0],tempo[1]);
trt:
      if(nbTemps>2)
        printf(",D2=%d",tempo[2]);
      if(skipCodes>0)
        printf(",skip=%d",skipCodes);
      printf(",DS=%d;ID:b1-b%d\"\n",DS,nbBit);
      printf("  duumaj datumoj : %s\n",bitoj);
      printf("  deksesumaj datumoj : ");
      for (int i=0 ; i<nbBit ; i += 4)
      {
        uint8_t val=0;
        for (int j=0; j<4 && j+i<nbBit ;j++)
	{
	  val = val<<1;
	  if(bitoj[i+j]=='1') val = val|1;
	}
	printf("%x",val);
      }
      printf("  \n\n");
        
suite:
      fflush(stdout);
      Traktenda=0;
    }
    else
    {
      if(FicIn)
      {
	if( fscanf(FicIn,"%d,",&dauro) <= 0) exit(0);
	trakti();
      }
      else delay(20);
    }
  }
  exit(0);
}
