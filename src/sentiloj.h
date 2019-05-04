struct sentilo
{
  const char * nomo;
  const char * proto;
  int nbBitoj; // nombro de bitoj
  int nbKadro;
  int D0; // daŭro de la plej mallongaj pulsoj (mikrosekundoj)
  int D1;
  int D2;
  int DS;// tempo de sinkronigo
  int salti;// impulsoj ignorendaj komence de la kadro
  const char * partoj;
};

int sentilo_sendu(char * komando);
int trakto_Kadro(struct bufroKadro *bufK);
int sentilo_legi();

