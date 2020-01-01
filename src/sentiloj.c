#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

#include "rf_gpio.h"
#include "sentiloj.h"
#include "protokoloj.h"

void tcp_sendiCxiuj(const char *buffer,int len);

struct sentilo *konatajSensiloj;
int NbSent=0;

char * iniFile=0;

int sentilo_legi()
{
 FILE * dosEn;
  if(iniFile)
    dosEn=fopen(iniFile,"r");
  else 
    dosEn=fopen("sentiloj.ini","r");
  if(!dosEn)
  {
    printf(" fichier ini pas trouvé.\n");
    return 0;
  }
  konatajSensiloj=(struct sentilo *) malloc(500*sizeof(struct sentilo));
  char * bufLine=malloc(10002);
  int nbLine=0;
  int nbComment=0;
  while(NbSent<500 && fgets(bufLine,10000,dosEn))
  {
    nbLine++;
    if (bufLine[0]=='#')
    {
      nbComment++;
      continue;
    }
    int len=strlen(bufLine);
    while(bufLine[len-2]=='\\')
    {
       fgets(bufLine+len-2,10000-len+2,dosEn);
       len=strlen(bufLine);
    }
    bufLine[len-1]=',';
    char * savtok1=0;
    char * strnom=strtok_r(bufLine,";",&savtok1);
    konatajSensiloj[NbSent].nomo=strdup(strnom);
    char * strprot=strtok_r(0,";",&savtok1);
    char * savtok2=0;
    if(!strprot) continue;
    strnom=strtok_r(strprot,",",&savtok2);
    konatajSensiloj[NbSent].proto=strdup(strnom);
    while(strnom=strtok_r(0,",",&savtok2))
    {
      char *ptr=strstr(strnom,"=");
      int val = atoi(ptr+1);
      if(!memcmp(strnom,"D0",2))
	konatajSensiloj[NbSent].D0=val;
      else if(!memcmp(strnom,"D1",2))
	konatajSensiloj[NbSent].D1=val;
      else if(!memcmp(strnom,"D2",2))
	konatajSensiloj[NbSent].D2=val;
      else if(!memcmp(strnom,"DS",2))
	konatajSensiloj[NbSent].DS=val;
      else if(!memcmp(strnom,"bitoj",4))
	konatajSensiloj[NbSent].nbBitoj=val;
      else if(!memcmp(strnom,"salti",4))
	konatajSensiloj[NbSent].salti=val;
    }
    
    char * strProp=strtok_r(0,";",&savtok1);
    if(strProp)
    {
      konatajSensiloj[NbSent].partoj=strdup(strProp);
      // RIPARU MIN : enlever les espaces et les tabulations
//    printf("sentilo lega : %s ; %s ; %s\n",konatajSensiloj[NbSent].nomo,konatajSensiloj[NbSent].proto,konatajSensiloj[NbSent].partoj);
      // RIPARU MIN : analyse ?
    }
    
    NbSent++;
  }
  printf(" %d/%d  ŝarĝitaj sentiloj \n",NbSent,nbLine);
  fclose(dosEn);
//  for (int i=0 ; i<NbSent ; i++)
//    printf("%s;%s,D0=%d,D1=%d,D2=%d,DS=%d;%s\n",konatajSensiloj[i].nomo,konatajSensiloj[i].proto,konatajSensiloj[i].D0,konatajSensiloj[i].D1,konatajSensiloj[i].D2,konatajSensiloj[i].DS,konatajSensiloj[i].partoj);
}


int sentilo_sendu(char * komando)
{
  char * ptr=strchr(komando,';');
  if(!ptr) return 0;
  int longoN= (ptr - komando);
  for ( int i=0; konatajSensiloj[i].proto ; i++ )
  {
    if ( strncmp(komando,konatajSensiloj[i].nomo,longoN) || konatajSensiloj[i].nomo[longoN]) continue;
    printf("trovita sentilo : protokolo %s, baza %d\n",konatajSensiloj[i].proto,konatajSensiloj[i].D0);
    if(konatajSensiloj[i].proto[0]=='p' ||konatajSensiloj[i].proto[0]=='P')
      sendo_p(ptr+1,&konatajSensiloj[i]);
    else if(konatajSensiloj[i].proto[0]=='t'||konatajSensiloj[i].proto[0]=='T')
      sendo_t(ptr+1,&konatajSensiloj[i]);
    else
    {
      printf("nekonata protokolo\n");
      return 0;
    }
    return 1;
  }
  return 0;
}

// propriétés à afficher en décimal :
const char *decVals=":HUM:WINDIR:SOUND:CO2:RGB:BLIND:SET_LEVEL:HSTATUS:BFORECAST:";



int trakto_Kadro(struct bufroKadro *bufK)
{
 int trovita = 0;
  //printf(" trame en trt : proto %s T %d %d , bitoj %d . %s.\n",bufK->proto+1
  //          ,bufK->D0,bufK->D1,strlen(bufK->bin));
  for ( int i=0; i<NbSent ;  )
  {
    if ( !strcmp(bufK->proto+1,konatajSensiloj[i].proto+1)
	&& ( konatajSensiloj[i].D0*80/100 < bufK->D0 ) 
	&& ( bufK->D0 < konatajSensiloj[i].D0*108/100)
	&& ( konatajSensiloj[i].D1*92/100 < bufK->D1 ) 
	&& ( bufK->D1 < konatajSensiloj[i].D1*108/100)
	&& konatajSensiloj[i].nbBitoj == strlen(bufK->bin)
	&& konatajSensiloj[i].salti == bufK->salti )
    {
      // printf(" trame en trt : %s.\n",bufK->bin);
      char buffer[256];
      char modBuffer[300];
      const char * ptrData;
      sprintf(buffer,"20;%02X;%s;",PakaNumero++,konatajSensiloj[i].nomo);
      if (konatajSensiloj[i].proto[0] == 'P')
      {
	int j=0;
	for (  ; bufK->bin[j] ; j++)
        {
	  if(bufK->bin[j]=='0') modBuffer[j]='1';
	  else if(bufK->bin[j]=='1') modBuffer[j]='0';
	  else modBuffer[j]=bufK->bin[j];
        }
	modBuffer[j]=0;
        ptrData= modBuffer;
      }
      else if (konatajSensiloj[i].proto[0] == 't' || konatajSensiloj[i].proto[0] == 'T')
      {
	// tristate
	// RIPARU MIN
	int j=0;
	for ( ; bufK->bin[2*j] ; j++)
	{
	  if(bufK->bin[2*j]=='0' && bufK->bin[2*j+1]=='1') modBuffer[j]='0';
	  else if(bufK->bin[2*j]=='1' && bufK->bin[2*j+1]=='0') modBuffer[j]='1';
	  else if(bufK->bin[2*j]=='0' && bufK->bin[2*j+1]=='0') modBuffer[j]='2';
	  else goto sekvaSensilo;
	}
	modBuffer[j]=0;
        ptrData= modBuffer;
      }
      else  ptrData=bufK->bin;
      const char * ptr= konatajSensiloj[i].partoj;
      while (ptr && *ptr)
      {
        const char * nextPtr= strchr(ptr,':');
        const char * nextProp= strchr(ptr,',');
        const char * ptrEg= strchr(ptr,'=');
        const char * ptrNot= strchr(ptr,'!');
        const char * ptrPlus= strchr(ptr,'+');
        const char * ptrMul= strchr(ptr,'*');
        const char * ptrDiv= strchr(ptr,'/');
	bool hasEqual=false;
	bool hasNot=false;
	if(*ptr==' '||*ptr=='\t') ptr++;
	if(*ptr==' '||*ptr=='\t') ptr++;
        if (ptrNot && ptrNot<nextProp)
	  hasNot=true;
        if (ptrEg && ptrEg<nextProp)
	  hasEqual=true;
 	if (hasEqual && (!nextPtr || nextPtr>nextProp) )
	{
	  strncat(buffer,ptr,nextProp-ptr);
	  strcat(buffer,";");
	  goto sekvante_part;
	}
	int32_t val=0;
	uint8_t isDec=0;
        if(!memcmp(nextPtr-4,"-inv",4))
	  goto sekvante_part;
	// 
        if (!hasEqual && !hasNot)
	{
	  strncat(buffer,ptr,nextPtr-ptr);
	  strcat(buffer,"=");
          char nomo[30];
	  strcpy(nomo,":");
          if(nextPtr-ptr>18)
	    printf("erreur : nom trop long.\n");
	  else
	  {
	    strncat(nomo+1,ptr,nextPtr-ptr);
	    strcat(nomo,":");
	    if(strstr(decVals,nomo)) isDec=1;
	  }
	}
	while(nextPtr && nextPtr<nextProp)
	{
         bool isBCD=false;
	 int valtmp=0;
	  if(nextPtr[1]=='B') //BCD
          {
	    isBCD=true;
	    valtmp=val*10;
	    val=0;
          }
	  nextPtr=nextPtr+2;
	  int firstbit=atoi(nextPtr);
	  nextPtr = strchr(nextPtr,'-');
	  nextPtr=nextPtr+1;
	  int lastbit=atoi(nextPtr);
	  if(lastbit>=firstbit)
	  {
            for(int j=firstbit ; j<=lastbit ; j++)
	    {
	      if(ptrData[j-1]=='0')
	        val = val<<1;
	      else
	        val = (val<<1) + 1;
	    }
	  }
	  else
	  {
            for(int j=firstbit ; j>=lastbit ; j--)
	    {
	      if(ptrData[j-1]=='0')
	        val = val<<1;
	      else
	        val = (val<<1) + 1;
	    }
	  }
          if(isBCD) val+=valtmp;
          nextPtr= strchr(nextPtr+1,':');
	}
        if (ptrPlus && ptrPlus<nextProp)
        { // 
	  int32_t cst;
	  sscanf(ptrPlus+1,"%lx",&cst);
	  val += cst;
	}
        if (ptrMul && ptrMul<nextProp)
        { // 
	  int32_t cst;
	  sscanf(ptrMul+1,"%lx",&cst);
	  val *= cst;
	}
        if (ptrDiv && ptrDiv<nextProp)
        { // 
	  int32_t cst;
	  sscanf(ptrDiv+1,"%lx",&cst);
	  val /= cst;
	}
	if(isDec)
	  sprintf(buffer+strlen(buffer),"%ld",val);
	else if(    !strncmp(ptr,"CMD:",4) )
        {
          // ON/OFF/ALLON/ALLOFF/UP/DOWN/STOP/PAIR//DISCO+/DISCO-/MODE0 - MODE8/BRIGHT/COLOR/DIM/CONFIRM/LIMIT
          if(val==0) strcat(buffer,"OFF");
          else if(val==1) strcat(buffer,"ON");
          else if(val==2) strcat(buffer,"ALLOFF");
          else if(val==3) strcat(buffer,"ALLON");
          else if(val==6) strcat(buffer,"BRIGHT");
          else if(val==7) strcat(buffer,"DIM");
          // RIPARU MIN : UP/DOWN/STOP/PAIR//DISCO+/DISCO-/MODE0 - MODE8/BRIGHT/COLOR/DIM/CONFIRM/LIMIT
        }
	else
        {
          if (!hasEqual && !hasNot)
	  {
	    if(    !strncmp(ptr,"TEMP:",5) )
	    {
	      uint16_t val16=val;
	      if (val16 >0x800) val16 = ((0-val16)&0x7ff)|0x8000;
	      sprintf(buffer+strlen(buffer),"%04x",val16);
	    }
	    else
	      sprintf(buffer+strlen(buffer),"%lx",val);
          }
	  else
	  {// constante à comparer
	    uint32_t cst;
	    if (hasEqual)
	      sscanf(ptrEg+1,"%lx",&cst);
	    else if (hasNot)
	      sscanf(ptrNot+1,"%lx",&cst);
	    if (( hasEqual && val != cst) || ( hasNot && val == cst))
	      goto sekvaSensilo;
          }
        }
        if (!hasEqual && !hasNot)
          strcat(buffer,";");
sekvante_part:
	ptr=nextProp;
	if(ptr) ptr++;
      }
      strcat(buffer,"\n");
      puts(buffer);
      tcp_sendiCxiuj(buffer,strlen(buffer));
      trovita = 1;
    }
   sekvaSensilo:
     i++;
  }
  return trovita;
}
