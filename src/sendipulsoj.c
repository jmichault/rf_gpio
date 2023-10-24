#include <lgpio.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <time.h>

// eligo-pinglo 
int PIN_EL=17;
int Lgpio_h;

struct timespec SekvaMomento;


void dormi( int microSecs)
{
  SekvaMomento.tv_nsec += microSecs * 1000;
  if(SekvaMomento.tv_nsec  >= 1000000000)
  {
    SekvaMomento.tv_nsec -= 1000000000;
    SekvaMomento.tv_sec++;
  }
  clock_nanosleep(CLOCK_MONOTONIC,TIMER_ABSTIME,&SekvaMomento,NULL);
}


void initSend()
{
  lgGpioClaimOutput(Lgpio_h, 0, PIN_EL, 0);
  lgGpioWrite(Lgpio_h, PIN_EL, 0);
  clock_gettime(CLOCK_MONOTONIC, &SekvaMomento);
}

void sendiParon(int Tempo1,int Tempo2)
{
  lgGpioWrite(Lgpio_h, PIN_EL, 1);
  dormi( Tempo1 );
  lgGpioWrite(Lgpio_h, PIN_EL, 0);
  dormi( Tempo2 );
}


int main(int argc, char *argv[])
{
  Lgpio_h=lgGpiochipOpen(0);
  initSend();
  sendiParon(500,500);
  int nivelo=1;
  for (int i=0 ; i<7 ; i++)
  {
    char * ptr=argv[1];
    do
    {
      int pulse=atoi(ptr);
      lgGpioWrite(Lgpio_h, PIN_EL, nivelo);
      if (nivelo==1) nivelo=0;
      else nivelo=1;
      ptr = strstr(ptr,",");
      dormi( pulse );
    } while (ptr++);
  }
  sendiParon(25000,500);
}
