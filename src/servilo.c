#include <stdio.h>
#include <unistd.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#include "rf_gpio.h"
#include "sentiloj.h"

int Haveno=10000;


// Servilo konektilo («socket»)
static int ServKon;
// Servilo Adreso
static struct sockaddr_in ServAdr;


static fd_set Aktiva_fd_set, Legado_fd_set;


#define MAXMSG  512

void tcp_sendiCxiuj(const char *buffer,int len)
{
  for (int i = 0; i < FD_SETSIZE; ++i)
    if (FD_ISSET (i, &Legado_fd_set))
    {
      if (i != ServKon)
      {
        write(i ,buffer,len);
      }
    }

}

int
legado_de_kliento (int DosNum)
{
  char bufro[MAXMSG];
  bzero(bufro,sizeof(bufro));
  int nbytes;

  nbytes = read (DosNum, bufro, MAXMSG);
  if (nbytes < 0)
  {
      perror ("legi eraron");
      //exit (EXIT_FAILURE);
  }
  else if (nbytes == 0)
    // fino de dosiero
    return -1;
  else
  {
    // Datumoj legitaj.
    if(RFDEBUG) fprintf (stderr, "Servilo: ricevis mesaĝon : %s\n", bufro);
    if (strlen(bufro) > 7)
    {
      if (strncasecmp (bufro,"10;PING;",8) == 0)
      {
        sprintf(bufro,"20;%02X;PONG;\n",PakaNumero++);
        write(DosNum,bufro,strlen(bufro));
      }
      else if (strncasecmp(bufro,"10;REBOOT;",10)==0)
      {
        strcpy(bufro,"reboot");
        // RIPARU MIN Reboot();
      }
      else if (strncasecmp(bufro,"10;RFDEBUG=ON;",14) == 0)
      {
        RFDebug=true;                                           // plena sencimigo
        sprintf(bufro,"20;%02X;RFDEBUG=ON;",PakaNumero++);
        tcp_sendiCxiuj(bufro,strlen(bufro));
      }
      else if (strncasecmp(bufro,"10;RFDEBUG=OFF;",15) == 0)
      {
        RFDebug=false;                                          // ne plena sencimigi
        sprintf(bufro,"20;%02X;RFDEBUG=OFF;\n",PakaNumero++);
        tcp_sendiCxiuj(bufro,strlen(bufro));
      }
      else if (strncasecmp(bufro,"10;RFUDEBUG=ON;",15) == 0)
      {
        RFUDebug=true;                                          // sencimigi
        sprintf(bufro,"20;%02X;RFUDEBUG=ON;\n",PakaNumero++);
        tcp_sendiCxiuj(bufro,strlen(bufro));
      }
      else if (strncasecmp(bufro,"10;RFUDEBUG=OFF;",16) == 0)
      {
        RFUDebug=false;                                         // ne sencimigi
        sprintf(bufro,"20;%02X;RFUDEBUG=OFF;\n",PakaNumero++);
        tcp_sendiCxiuj(bufro,strlen(bufro));
      }
      else if (strncasecmp(bufro,"10;QRFDEBUG=ON;",15) == 0)
      {
        QRFDebug=true;                                         // sencimigi
        sprintf(bufro,"20;%02X;QRFDEBUG=ON;",PakaNumero++);
      }
      else if (strncasecmp(bufro,"10;QRFDEBUG=OFF;",15) == 0)
      {
        QRFDebug=false;                                        // ne sencimigi
        sprintf(bufro,"20;%02X;QRFDEBUG=OFF;",PakaNumero++);
      }
      else if (strncasecmp(bufro,"10;VERSION",10) == 0)
      {
        sprintf(bufro,"20;%02X;VER=1.1;REV=%02x;BUILD=%02x;\n",PakaNumero++,REVNR, KONSTNR);
        write(DosNum,bufro,strlen(bufro));
      }
      else if (strncasecmp(bufro,"10;",3) == 0)
      {
	// RIPARU MIN : TRISTATEINVERT/RTSCLEAN/RTSRECCLEAN/RTSSHOW/RTSINVERT/RTSLONGTX
	// ĝeneralaj komandoj
        if (sentilo_sendu(bufro+3))
	{
          sprintf(bufro,"20;%02X;OK;\n",PakaNumero++);
          write(DosNum,bufro,strlen(bufro));
        }
        else
	{
          sprintf(bufro,"20;%02X;CMD UNKNOWN;\n",PakaNumero++);
          write(DosNum,bufro,strlen(bufro));
        }
      }
      else if (strncasecmp(bufro,"11;",3) == 0)
      {
	// eĥo komandoj
	char bufro2[20];
        sprintf(bufro2,"20;%02X;OK;\n",PakaNumero++);
        write(DosNum,bufro2,strlen(bufro));
        sprintf(bufro2,"%02X",PakaNumero++);
        memcpy(bufro+6,bufro2,2);
        write(DosNum,bufro+3,strlen(bufro+3));
      }
      else
      {
	// nekonata komandoj
        sprintf(bufro,"20;%02X;CMD UNKNOWN;\n",PakaNumero++);
        write(DosNum,bufro,strlen(bufro));
      }
    }
  }
  return 0;
}

void *tcpServilo(void * x)
{
 size_t grandeco;
  // konektilo krei
  ServKon = socket(AF_INET, SOCK_STREAM, 0);
  if (ServKon == -1)
  {
    printf("malsukcesa kreo de konektilo...\n");
    exit(0);
  }
  else
    printf("Socket sukcese kreiĝis..\n");
  /* aktivigi SO_REUSEADDR */
  int reusaddr = 1;
  if (setsockopt(ServKon, SOL_SOCKET, SO_REUSEADDR, &reusaddr, sizeof(int)) < 0)
  {
    perror ("setsockopt(SO_REUSEADDR) malsukceso");
    exit(0);
  }

  bzero(&ServAdr, sizeof(ServAdr));

  // atribui IP, PORT
  ServAdr.sin_family = AF_INET;
  ServAdr.sin_addr.s_addr = htonl(INADDR_ANY);
  ServAdr.sin_port = htons(Haveno);

  // Ligo nove kreita konektilo al donita IP
  if ((bind(ServKon, (struct sockaddr *)&ServAdr, sizeof(ServAdr))) != 0)
  {
    printf("«bind» malsukceso...\n");
    exit(0);
  }

  // Nun servilo pretas aŭskulti
  if ((listen(ServKon, 5)) != 0)
  {
    printf("«Listen» malsukceso...\n");
    exit(0);
  }
  else
    printf("Servilo aŭskultado..\n");
  // Komenci la aron de aktivaj konektiloj
  FD_ZERO (&Aktiva_fd_set);
  FD_SET (ServKon, &Aktiva_fd_set);
  while (1)
  {
    // Bloki ĝis enigo alvenos al unu aŭ pli aktivaj konektilo
    Legado_fd_set = Aktiva_fd_set;
    if (select (FD_SETSIZE, &Legado_fd_set, NULL, NULL, NULL) < 0)
    {
      perror ("select");
      exit (EXIT_FAILURE);
    }

    // Servas ĉian konektilo kun enigo pritraktata.
    for ( int i = 0; i < FD_SETSIZE; ++i)
      if (FD_ISSET (i, &Legado_fd_set))
      {
        if (i == ServKon)
        {
          // Peto pri ligo sur originala konektilo.
          int newKon;
          grandeco = sizeof (ServAdr);
          newKon = accept (ServKon, (struct sockaddr *) &ServAdr, &grandeco);
          if (newKon < 0)
          {
            perror ("accept");
            exit (EXIT_FAILURE);
          }
          fprintf (stderr, "Servilo: konekto de la kliento %s, haveno %hd.\n"
			, inet_ntoa (ServAdr.sin_addr), ntohs (ServAdr.sin_port));
          FD_SET (newKon, &Aktiva_fd_set);
	  char bufro[256];
          sprintf(bufro,"20;00;Nodo RadioFrequencyLink - RFLink Gateway V1.1 - R%02x\n;",REVNR);
          write(newKon,bufro,strlen(bufro));
        }
        else
        {
          // Datumoj alvenantaj al jam-konektita ingo.
          if (legado_de_kliento (i) < 0)
          {
            close (i);
            FD_CLR (i, &Aktiva_fd_set);
          }
        }
      }
  }
};


