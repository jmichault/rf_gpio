#include <stdint.h>

// Revizio nombro
#define REVNR 1.0
// Konstruado nombro
#define KONSTNR 01

extern uint8_t PakaNumero;

// sencimigo
extern char RFUDebug;
extern char QRFDebug;
// plena sencimigo
extern char RFDebug;

struct bufroKadro
{
long unuaTempo;
char proto[10]; // protokolo
  int nbBits;
  int D0;
  int D1;
  int D2;
  int DS;
  int salti;
int nb;		// nombro
char bin[200];	// binaraj datumoj 01
};

