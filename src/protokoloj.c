#include <wiringPi.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <time.h>

#include "rf_gpio.h"
#include "sentiloj.h"

// eligo pinglo
int PIN_EL=0;

struct timespec sekvaMomento;


void dormi( int microSecs)
{
  sekvaMomento.tv_nsec += microSecs * 1000;
  if(sekvaMomento.tv_nsec  >= 1000000000)
  {
    sekvaMomento.tv_nsec -= 1000000000;
    sekvaMomento.tv_sec++;
  }
  clock_nanosleep(CLOCK_MONOTONIC,TIMER_ABSTIME,&sekvaMomento,NULL);
}

void pravaloriziSendo()
{
  pinMode(PIN_EL, OUTPUT);
  digitalWrite(PIN_EL, LOW);
  clock_gettime(CLOCK_MONOTONIC, &sekvaMomento);
}

void sendiParon(int Tempo1,int Tempo2)
{
  digitalWrite(PIN_EL, HIGH);
  dormi( Tempo1 );
  digitalWrite(PIN_EL, LOW);
  dormi( Tempo2 );
}

// valorizi parton antaux sendi
void valPart(char *buffer,const char * part,uint32_t val)
{
  // aro da bitoj valorizenda
  uint8_t bitoj[32];
  uint8_t nbBitoj=0;
  // kalkulu la bitojn, kaj plenigu la aro da bitoj
  const char *ptr=part;
  const char * nextProp= strchr(ptr,',');
  const char * nextPtr= strchr(ptr,':');
  if(*ptr==' '||*ptr=='\t') ptr++;
  if(*ptr==' '||*ptr=='\t') ptr++;
  while(nextPtr && nextPtr<nextProp)
  {
    nextPtr=nextPtr+2;
    int firstbit=atoi(nextPtr);
    nextPtr = strchr(nextPtr,'-');
    nextPtr=nextPtr+1;
    int lastbit=atoi(nextPtr);
    if(lastbit>=firstbit)
    {
      for(int j=firstbit ; j<=lastbit ; j++)
        bitoj[nbBitoj++]=j;
    }
    else
    {
      for(int j=firstbit ; j>=lastbit ; j--)
        bitoj[nbBitoj++]=j;
    }
    nextPtr= strchr(nextPtr+1,':');
  }
  // valorizi bitoj
  for ( int i=0 ; i<nbBitoj ; i++)
  {
    buffer[bitoj[nbBitoj-i-1]-1] = ( val &(1<<i)) ? '1':'0';
  }
}

int const_kadro(char *buffer,char * komando, struct sentilo * sentilo)
{
  char ordo[20];
  for (int i=0 ; i < sentilo->nbBitoj ; i++)  buffer[i]='0';
  buffer[sentilo->nbBitoj]=0;
  buffer[sentilo->nbBitoj+1]=0;
  uint32_t id=-1,kanalo=-1;
  sscanf(komando,"%x;%d;%s",&id,&kanalo,ordo);
printf(" procesas komando %s , id=%x, kanalo=%x ordo=%s\n",komando,id,kanalo,ordo);
  // procesas la identigilo
  const char *ptr = strstr(sentilo->partoj,"ID:");
  int b0,b1;
  valPart(buffer,ptr,id);
  ptr = strstr(sentilo->partoj,"ID-inv:");
  if(ptr)
    valPart(buffer,ptr,~id);
  // procesas la kanalo
  // RIPARU MIN : no kanalo ekzemple : 10;Selectplus;001c33; => SelectPlus protocol;address
  if (kanalo >0 )
  {
    ptr = strstr(sentilo->partoj,"SWITCH:");
    if(ptr)
      valPart(buffer,ptr,kanalo);
    ptr = strstr(sentilo->partoj,"SWITCH-inv:");
    if(ptr)
      valPart(buffer,ptr,~kanalo);
  }
  // procesas la ordo
  // ON/OFF/ALLON/ALLOFF/UP/DOWN/STOP/PAIR//DISCO+/DISCO-/MODE0 - MODE8/BRIGHT/COLOR/DIM/CONFIRM/LIMIT
  int iOrdo=0;
  if( !strcasecmp(ordo,"ON;")) iOrdo=1;
  // RIPARU MIN : OFF/ALLON/ALLOFF/UP/DOWN/STOP/PAIR//DISCO+/DISCO-/MODE0 - MODE8/BRIGHT/COLOR/DIM/CONFIRM/LIMIT
  ptr = strstr(sentilo->partoj,"CMD:");
  if(ptr)
    valPart(buffer,ptr,iOrdo);
  ptr = strstr(sentilo->partoj,"CMD-inv:");
  if(ptr)
    valPart(buffer,ptr,~iOrdo);
  // foliumi ĉiujn proprietojn kaj trakti konstantojn
  ptr= sentilo->partoj;
  while (ptr && *ptr)
  {
    const char * nextPtr= strchr(ptr,':');
    const char * nextProp= strchr(ptr,',');
    const char * ptrEg= strchr(ptr,'=');
    const char * ptrPlus= strchr(ptr,'+');
    const char * ptrMul= strchr(ptr,'*');
    const char * ptrDiv= strchr(ptr,'/');
    bool hasEqual=false;
    if(*ptr==' '||*ptr=='\t') ptr++;
    if(*ptr==' '||*ptr=='\t') ptr++;
    if (ptrEg && ptrEg<nextProp)
      hasEqual=true;
    uint32_t val=0;
    uint8_t isDec=0;
    if(hasEqual)
    {
      uint32_t cst;
      sscanf(ptrEg+1,"%lx",&cst);
      valPart(buffer,ptr,cst);
    }
sekvante_part:
    ptr=nextProp;
    if(ptr) ptr++;
  }


  // RIPARU MIN : procezi kontrolsumojn
  // RIPARU MIN : procezi cyclajn kodojn
  return 1;
  
}

int sendo_kadro(char * buffer, struct sentilo * sentilo)
{
  pravaloriziSendo();
  sendiParon(sentilo->D0,sentilo->D0);
  sendiParon(sentilo->D0,sentilo->DS);
  for( int j=0 ; j <6 ; j++)
  {
    for (int i=0 ; buffer[i] ; i++)
      if(buffer[i]=='0')
      {
	int t1,t2;
	if(sentilo->proto[1]=='0') t1=sentilo->D0;
	else if(sentilo->proto[1]=='1') t1=sentilo->D1;
	else t1=sentilo->D2;
	if(sentilo->proto[2]=='0') t2=sentilo->D0;
	else if(sentilo->proto[2]=='1') t2=sentilo->D1;
	else t2=sentilo->D2;
	sendiParon(t1,t2);
      }
      else
      {
	int t1,t2;
	if(sentilo->proto[3]=='0') t1=sentilo->D0;
	else if(sentilo->proto[3]=='1') t1=sentilo->D1;
	else t1=sentilo->D2;
	if(sentilo->proto[4]=='0') t2=sentilo->D0;
	else if(sentilo->proto[4]=='1') t2=sentilo->D1;
	else t2=sentilo->D2;
	sendiParon(t1,t2);
      }
    sendiParon(sentilo->D0,sentilo->DS);
  }
  return 1;
}

int sendo_p(char * komando, struct sentilo * sentilo)
{
 char buffer[256];
  if ( !const_kadro(buffer, komando, sentilo) ) return 0;
  return sendo_kadro( buffer, sentilo);
}

int sendo_t(char * komando, struct sentilo * sentilo)
{
 char buffer[256];
  if ( !const_kadro(buffer, komando, sentilo) ) return 0;
  for (int i=strlen(buffer) ; i>=0 ; i--)
  {
    if(buffer[i]=='0')
    {
      buffer[2*i]='0';
      buffer[2*i+1]='1';
    }
    else if(buffer[i]=='1')
    {
      buffer[2*i]='1';
      buffer[2*i+1]='0';
    }
    else
    {
      buffer[2*i]='0';
      buffer[2*i+1]='0';
    }
  }
  return sendo_kadro( buffer, sentilo);
}

int testo_p001(int skipCodes,const char *codes,int ATraiter,struct bufroKadro *bufK)
{
          //RIPARU MIN : provi p001
          // décodage des données en bits :
	  int nbBit=0;
          for(int k=0 ; k < ATraiter-3 && codes[k] ; k++,nbBit++)
            if(codes[k]=='0' && codes[k+1]=='0')
            {
              bufK->bin[nbBit]='0';
              k++;
            }
            else if(codes[k]=='1')
            {
              bufK->bin[nbBit]='1';
            }
            else
            {
              if( k< (ATraiter -4))
              {
                return false;
              }
              nbBit--;
            }
          bufK->bin[nbBit]=0;
          bufK->nbBitoj=nbBit;
// applique le code manchester :
  char oldbit='1';
  for ( int i=0 ; i<nbBit ; i++)
  {
    if(bufK->bin[i] == '1')
    {
      if (oldbit=='0') oldbit='1';
      else oldbit='0';
    }
    bufK->bin[i] = oldbit;
  }
          strcpy(bufK->proto,"p001");
          //printf(" %d pulsations protocole : \"xxx;p001,bitoj=%d,D0=%d,D1=%d",ATraiter-3,nbBit,temps[0],temps[1]);
          return true;

}

int testo_p(int skipCodes,const char *codes,int ATraiter,struct bufroKadro *bufK)
{
 char code0[3],code1[3];
 int i;
  memcpy(code0,codes+skipCodes,2); code0[2]=0;
  for(i=skipCodes ; i < ATraiter-2 && codes[i] ; i+=2)
    if(memcmp(&codes[i],code0,2)) break;
  if(i==ATraiter-2 || !codes[i])
    return false;
  memcpy(code1,codes+i,2); code1[2]=0;
  for( ; i < ATraiter-2 && codes[i] ; i+=2)
    if(memcmp(&codes[i],code0,2) && memcmp(&codes[i],code1,2)) break;
  int nbBit=0;
  if(i < ATraiter-4)
    return false;
    // on suppose que le code le plus petit correspond au bit 0
  if(strcmp(code0,code1) >0)
  {
    char buf[2];
    memcpy(buf,code0,2);
    memcpy(code0,code1,2);
    memcpy(code1,buf,2);
  }
  // décodage des données en bits :
  for(i=skipCodes ; i < ATraiter-4 && codes[i] ; i+=2)
    if(!memcmp(&codes[i],code0,2))
      bufK->bin[i/2]='0';
    else
      bufK->bin[i/2]='1';
  bufK->nbBitoj=i/2;
  bufK->bin[i/2]=0;
  sprintf(bufK->proto,"p%s%s",code0,code1);
  return true;
}

