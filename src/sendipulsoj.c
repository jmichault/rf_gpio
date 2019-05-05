#include <wiringPi.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <time.h>

// eligo-pinglo 
int PIN_EL=0;

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
  pinMode(PIN_EL, OUTPUT);
  digitalWrite(PIN_EL, LOW);
  clock_gettime(CLOCK_MONOTONIC, &SekvaMomento);
}

void sendiParon(int Tempo1,int Tempo2)
{
  digitalWrite(PIN_EL, HIGH); 
  dormi( Tempo1 );
  digitalWrite(PIN_EL, LOW);
  dormi( Tempo2 );
}


int main(int argc, char *argv[])
{
  if(wiringPiSetup() == -1){
    printf("ne detektis «wiring pi»\n");
    return 0;
  }
  initSend();
  sendiParon(500,500);
  int nivelo=HIGH;
  for (int i=0 ; i<7 ; i++)
  {
    char * ptr=argv[1];
    do
    {
      int pulse=atoi(ptr);
      digitalWrite(PIN_EL, nivelo); 
      if (nivelo==HIGH) nivelo=LOW;
      else nivelo=HIGH;
      ptr = strstr(ptr,",");
      dormi( pulse );
    } while (ptr++);
  }
  sendiParon(25000,500);
}
