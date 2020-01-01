LDLIBS=-lwiringPi -lrt
CFLAGS=-g
LDFLAGS=-g

but: analizi usend rf_gpio sendipulsoj

../LisMoi.md : ../LeguMin.md
	trans -b eo: :fr -i ../LeguMin.md >trans.fr.tmp

../README.md : ../LeguMin.md
	trans -b eo: :en -i ../LeguMin.md >trans.en.tmp

usend : usend.o protokoloj.o sentiloj.o
	gcc $(LDFLAGS) -o $@ $^ $(LDLIBS)

protokoloj.o : protokoloj.h rf_gpio.h sentiloj.h protokoloj.c
	gcc $(CFLAGS) -c $*.c

sentiloj.o : rf_gpio.h protokoloj.h sentiloj.h sentiloj.c
	gcc $(CFLAGS) -c $*.c
	
rf_gpio : rf_gpio.o servilo.o protokoloj.o sentiloj.o
	gcc -g -o $@ $^ -lwiringPi -lpthread


dist:	but
	strip rf_gpio analizi
	cp -p sentiloj.ini rf_gpio analizi rf_gpio.sh ..
	make clean

clean:
	rm *.o analizi sendipulsoj usend rf_gpio toto* trans* nohup.out 2>/dev/null;echo

