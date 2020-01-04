# rf_gpio

rf_gpio simulas RFLink kun dissendilo kaj ricevilo ligitaj rekte al la GPIO-havenoj de raspberry pi.


## INSTALO:
cd /home/pi/rf_gpio  
sudo cp -p rf_gpio.sh /etc/init.d  
sudo update-rc.d rf_gpio.sh default  
sudo servo rf_gpio.sh start  

konekti la ricevilon al gpio2 bcm27 (pinglo 13)  
konekti la dissendilon al gpio0 bcm17 (pinglo 11)  

## Uzu kun domoticz:  
aldonu RFLink-enirejon kun LAN-interfaca tipo-aparataro  
        fora adreso: 127.0.0.1  
        haveno: 10000  
se la aparataro estas rekonita, la integriĝo kun domoticz estas simpla: simple uzu la butonon "aŭtomata detekto" de la langeto "ŝaltiloj", aŭ enŝaltu la opcion "permesi dum 5 minutoj"

## Dissendiloj kaj Riceviloj provitaj:
* ĉina ilaro kun reaga ricevilo (asin = b00z9sznp0, mx-05v + mx-fs-03v), vidata sur amazono je 1 €.  
        la dissendilo estas bone, sed la ricevilo estas tre sensenta (ĉirkaŭ 4 m), por uzi nur por lernaj kodoj.  
* wl101-341 + wl102-341, superheterodina 433 mhz, vidata ĉe aliexpress je 1 €.  
        la ricevilo havas pli bonan sentemon ol la antaŭa.  

## rekonita ekipaĵo:
estis sukcese testita:  
* eniroj kyg (asin: b07dpmpvw1, markita intertek, vidpunktoj sur amazono) (konata kiel "impuls")  
* aneng termometro-higrometro (ĉina malalta kosto, kun ekrano lcd, vidata ĉe aliexpress)  
* digoo rg-8b-termometrilo-higrometro (ĉina malalta kosto, sen ekrano, vidata ĉe aliexpresso)

aliaj sensiloj estas antaŭdifinitaj en sentiloj.ini, sed ne estis testitaj.

## aldoni novan sensor:
Estas necese identigi la protokolon de transporto de datumoj kaj la formato de datumoj.  
### elekto 1:  
sekvu la eliron de rf_gpio, aŭ lanĉante ĝin en ŝelfenestro, aŭ konektante ĝin kun la komando "telnet 127.0.0.1 10000".  
Kiam la sentilo sendas datumojn kaj la transiga protokolo estas rekonita, vi vidos ion similan al ĉi tio:  
20;00;p0102,bitoj=36,D0=529,D1=949,D2=1926,DS=3865;duumaj=011100110000000011011000111100100000,deksesumu=7300d8f20;  
20; = iu ajn RFLink-dissenda kadro komencas tiel.  
00; = unua transdonita kadro.  
p0102,bitoj=36,D0=529,D1=949,D2=1926,DS=3865 = rf_gpio identigis protokolon de la tipo:  
        Bito 0 = D0 D1  
        bito 1 = D0 D2  
        36 pecoj da datumoj  
        Daŭro D0 = 520 μs  
        daŭro D1 = 957 μs  
        daŭro D2 = 1936 μs  
        Tempo de sinkronigo DS = 3881 μs  
duumaj=011100110000000011011000111100100000,deksesumaj=7300d8f20; : datumoj ricevitaj en duuma kaj deksesuma.

### elekto 2:
kuri en konko:  
./analizi  
kaj premu la butonon sur la fora kontrolo, aŭ atendu, ke la sensor sendu datumojn. Se la protokolo estas rekonita, ni vidas ion similan al ĉi tio:  
 73 impulsoj protokolo: "xxx; p0001,bitoj=36,D0=689,D1=1923,DS=3890;ID:b1-b36"  
  duumaj datumoj: 011100110000000011011000111100100000  
  hexa datumoj: 7300d8f20  

Nun vi bezonas analizi la binarajn datumojn por identigi la signifon de ĉiu peco.  
Vi povas tiam aldoni linion en la dosieron sentiloj.ini, ĉiu linio konsistas el tri elementoj apartigitaj per punktokomo:  
* unua elemento: nomo de la materialo. Atento, se ĝi estas ŝaltilo, ĝi devas esti parto de la listo de elementoj rekonitaj de domoticz.  
* dua elemento: protokolo. Kopiu tion, kio montras rf_gpio aŭ analizo.  
        ekzemplo: p0102,bitoj=36,D0=561,D1=1899,D2=3845,DS=9158  
                signifas: protokolo p0102 (bito 0 = D0 D1, bito 1 = D0 D2), 36 bitoj per kadro, daŭro D0 = 561 μs, daŭro D1 = 1899 μs, daŭro D2 = 3845 μs, tempo de sinkronigo DS = 9158 μs  
                la partoj de protokolo, bito, D0 kaj D1 estas uzataj por diferenci la aparatojn.  
* tria elemento: priskribo de la datumoj, ĉiu kampo apartigita de la aliaj per komo  
        ekzemplo por mult-kanala fora kontrolo, ni povas havi: ID:b1-20,CMD:b21-21,SWITCH:b22-24  
                ID:b1-20 signifas, ke la ID de la fora kontrolo estas en bitoj 1 ĝis 20 (bito 1 = unua bitoj transdonita)  
                CMD:b21-21 signifas, ke la komando transdonita (0/1) estas en la bito 21.  
                SWITCH:b22-24 signifas, ke la nombro de la funkciigita frapeto estas en bitoj 22 ĝis 24.  
        Por esti rekonita de domoticz, la nomo de la kampo devas esti en la agnoskita listo (vidu sendiloj.txt). Tamen ni povas meti tion, kion ni volas, simple domoticz ignoros la kampon.  
        Pluraj sekvencoj de bitoj povas esti kunmetitaj, ekz.: CMD:b17-17:b15-15:b16-16 konkatenos bitojn 17 kaj 16 en ĉi tiu ordo.  
        Unu povas testi la valoron de iuj bitoj, ekzemple: CST2:b43-48=1 kontrolos, ke la bitoj 43 ĝis 48 enhavas la valoron 1 (deksesuma) en ricevo, kaj influos ĉi tiujn bitojn en eligo, CST2:b43-48!1 kontrolos, ke la bitoj 43 ĝis 48 ne enhavas la valoron 1 (deksesuma) en ricevo.  
        Vi povas deklari BCD-koditajn kampojn: metu B anstataŭ b. ekzemplo: TEMP:B12-15:B16-19:B20-23 deklaras temperaturtempon kies unua cifero estas en bitoj 12-15, la dua cifero en bitoj 16-20 kaj la tria en bitoj 21-23.  
        Kampo finanta per "-inv" estas speciala kampo kiu prenos la kontraŭan valoron (onia komplemento) de ĝia samnoma kampo en la spektaklo.  
        Vi povas atribui valoron al kampo kiu ne estas en la datumoj uzante "=". ekzemplo: CMD=ON  
        Ni povas fari simplajn kalkulojn: aldono, multipliko kaj subtraho estas eblaj (en ĉi tiu ĝusta ordo), la konstantoj estas deksesumaj.  
                 Ekzemplo por konverti datumojn, kiuj estas en dekonoj de °F+900 al dekonoj de °C:  
                 TEMP: b17-28+-4c4*5/9 (sekve: fina_data = (kruda_data -1220) * 5 / 9)  


Ricevinte ĉiujn liniojn, kiuj kontentigas la kondiĉon, generos linion, se vi volas eviti falsajn pozitivaĵojn, vi povas komenti aŭ forigi la liniojn, kiuj ne kongruas kun via materialo.  
En sendado, nur la unua linio kun la ĝusta aparata nomo estos uzata.  


Se la protokolo ne estas rekonita, vi povas uzi analizi por studi ĝin per pliigo de detaloj per la eblo -v, -vv aŭ -vvv. Sed rf_gpio ne povos rekoni ĝin sen plua evoluo.  


## PROTOKOLOJ apogitaj:

Nur protokoloj kun almenaŭ la sekvaj karakterizaĵoj estas rekonitaj:  
* premas tempo ĉiam> 100 μs  
* ne pli ol tri malsamaj daŭroj por reprezenti la datumojn.  
* ĉiu bito estas kodita per du pulsos, ĉiam sammaniere.  
* ĉiu kadro estas ĉirkaŭita de du sinkronigaj signaloj kun tempo> 2500μs.  
* Kadro ne enhavas pli ol 200 pulsos.  

En sendado, la rotaciaj kodoj kaj kontumoj ne estas administrataj.  
