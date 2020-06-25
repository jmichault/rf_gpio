# rf_gpio
rf_gpio permet de simuler un RFLink avec un émetteur et un récepteur branchés directement sur les ports GPIO d'un raspberry pi.


## INSTALLATION :
cd /home/pi/rf_gpio  
sudo cp -p rf_gpio.sh /etc/init.d  
sudo update-rc.d rf_gpio.sh defaults  
sudo service rf_gpio.sh start  

connecter le récepteur sur gpio2 bcm27 (broche 13)  
connecter l'émetteur sur gpio0 bcm17 (broche 11)  

## utilisation avec domoticz :
ajouter un matériel de type «RFLink Gateway with LAN interface»  
	adresse distante : 127.0.0.1  
	Port : 10000  
Si le matériel est reconnu, l'intégration avec domoticz est simple : il suffit d'utiliser le bouton «détection auto» de l'onglet «Interrupteurs», ou d'activer l'option «Autoriser pendant 5 minutes»  

## Émetteurs-récepteurs testés :
* kit chinois avec un récepteur à réaction (ASIN=B00Z9SZNP0, MX-05V + MX-FS-03V), vu sur amazon à 1€.  
	l'émetteur est OK, mais le récepteur est très peu sensible (portée d'environ 4m), à n'utiliser que pour l'apprentissage des codes.  
* WL101-341 + WL102-341 , superhétérodyne 433 Mhz, vu sur aliexpress à 1€.  
	le récepteur a une meilleure sensibilité que le précédent.  
	
Remarque: Pour de bons résultats, le récepteur doit être protégé contre les parasites émis par le raspberry-pi. Pour les tests, vous pouvez utiliser une feuille d'aluminium placée dans une feuille de papier pliée en deux.

## MATÉRIELS RECONNUS :
ont été testés avec succès :  
* prises KYG (ASIN : B07DPMPVW1 , marquées Intertek, vues  sur amazon)(reconnues comme «Impuls»)  
* thermomètre-hygromètre Aneng ( chinois à bas coût, avec affichage LCD, vu sur aliexpress.)  
* thermomètre-hygromètre Digoo RG-8B ( chinois à bas coût, sans affichage, vu sur aliexpress.)  

D'autres capteurs sont prédéfinis dans sentiloj.ini, mais n'ont pas été testés.  

## pour ajouter un nouveau capteur :
Il faut identifier le protocole de transfert des données et le format des données.  
### option 1 :
suivre la sortie de rf_gpio, soit en le lançant dans une fenêtre shell, soit en s'y connectant avec la commande «telnet 127.0.0.1 10000».  
Quand le capteur envoie des données, et que le protocole de transfert est reconnu, vous verrez quelque chose qui ressemble à ça :  
20;00;p0102,bits=36,D0=529,D1=949,D2=1926,DS=3865;binary=011100110000000011011000111100100000,hex=7300d8f20;  
  20; = toute trame d'émission RFLink commence comme ça.  
  00; = première trame émise.  
  p0102,bits=36,D0=529,D1=949,D2=1926,DS=3865 = rf_gpio a identifié un protocole du type :  
	bit 0 = D0 D1  
	bit 1 = D0 D2  
	36 bits de données  
	durée D0 = 520 µs  
	durée D1 = 957 µs  
	durée D2 = 1936 µs  
	durée de synchro DS = 3881 µs  
  binary=011100110000000011011000111100100000,hex=7300d8f20; : données reçues en binaire et en hexa.  

### option 2 :
exécuter dans un shell :  
./analizi  
et appuyer sur le bouton de la télécommande, ou attendre que le capteur envoie des données. Si le protocole est reconnu, on voit quelque chose qui ressemble à ça :  
 73 pulsations protocole : "xxx;p0001,bits=36,D0=689,D1=1923,DS=3890;ID:b1-b36"  
  données binaires : 011100110000000011011000111100100000  
  données hexa : 7300d8f20  


Il vous faut maintenant analyser les données binaires pour identifier la signification de chaque bit.  
Vous pouvez ensuite ajouter une ligne dans le fichier sentiloj.ini, chaque ligne est composée de trois éléments séparés par un point-virgule :  
* premier élément : nom du matériel. Attention, si c'est un interrupteur, il doit obligatoirement faire partie de la liste des éléments reconnus par domoticz.  
* deuxième élément : protocole. Recopier ce qui est affiché par rf_gpio ou analizo.  
	exemple : p0102,bits=36,D0=561,D1=1899,D2=3845,DS=9158  
		signifie : protocole p0102 (bit 0 = D0 D1 , bit 1 = D0 D2), 36 bits par trame, durée D0 = 561 µs, durée D1 = 1899 µs, durée D2 = 3845 µs, durée synchro DS=9158 µs  
		les parties protocoles, bits, D0 et D1 sont utilisés pour différencier les dispositifs.  
* troisième élément : description des données, chaque champ séparé des autres par une virgule  
	exemple pour une télécommande multi-canaux, on peut avoir : ID:b1-20,CMD:b21-21,SWITCH:b22-24  
		ID:b1-20 signifie que l'ID de la télécommande se trouve dans les bits 1 à 20 (bit 1 = premier bit transmis)  
		CMD:b21-21 signifie que la commande transmise (ON/OFF) se situe au bit 21.  
		SWITCH:b22-24 signifie que le numéro de la prise actionnée se trouve aux bits 22 à 24.  
	Pour être reconnu par domoticz, le nom du champ doit être dans la liste reconnue (voir sentiloj.txt). On peut toutefois mettre ce qu'on veut, simplement le champ sera ignoré par domoticz.  
	On peut concaténer plusieurs suites de bits, exemple : CMD:b17-17:b15-15:b16-16 va concaténer les bits 17 15 et 16 dans cet ordre.  
	On peut tester la valeur de certains bits, exemple : CST2:b43-48=1 va vérifier que les bits 43 a 48 contiennent la valeur 1 (hexadécimale) en réception, et va affecter ces bits en émission, CST2:b43-48!1 va vérifier que les bits 43 a 48 ne contiennent pas la valeur 1 (hexadécimale) en réception.  
	On peut déclarer des champs codés BCD (binaire codé décimal) : mettre B au lieu de b. exemple : TEMP:B12-15:B16-19:B20-23 déclare un champ température dont le premier chiffre se trouve aux bits 12-15, le deuxième aux bits 16 à 20 et le troisième aux bits 21 à 23.  
	Un champ finissant par «-inv» est un champ spécial qui prendra la valeur inverse (complément à un) de son champ homonyme à l'émission.  
	On peut affecter une valeur à un champ qui ne se trouve pas dans les données en utilisant «=». exemple : CMD=ON  
	On peut faire des calculs simples : une addition, une multiplication et une soustraction sont possibles (dans cet ordre exact), les constantes sont en hexadécimal.  
		Exemple pour convertir en dixièmes de °C une donnée qui est en dixièmes de °F+900:  
		TEMP:b17-28+-4c4*5/9  ( donc : donnée_finale = (donnée_brute -1220) * 5 / 9 )  

En réception toutes les lignes qui satisfont la condition vont générer une ligne, si vous voulez éviter les faux positifs, vous pouvez mettre en commentaire ou supprimer les lignes qui ne correspondent pas à vos matériels.  
En émission, seule la première ligne ayant le bon nom de matériel sera utilisée.  
	
		
Si le protocole n'est pas reconnu, vous pouvez utiliser analizi pour l'étudier en augmentant sa verbosité avec l'option -v, -vv ou -vvv. Mais rf_gpio ne pourra pas le reconnaitre sans développement supplémentaire.  


## PROTOCOLES SUPPORTÉS :

Seuls les protocoles ayant au moins les caractéristiques suivantes ont une chance d'être reconnus :  
* temps d'une pulsation toujours > 100 µs  
* pas plus de trois durées différentes pour représenter les données.  
* chaque bit est codé avec deux pulsations, toujours de la même façon.  
* chaque trame est entourée par deux signaux de synchronisation d'un temps >2500µs.  
* une trame ne contient pas plus de 200 pulsations.  

À l'émission, les codes tournants et les sommes de vérification ne sont pas gérés.  
