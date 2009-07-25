SRC=proccache.c
TRG=proccache

CFLAGS=-Wall -pedantic -I /usr/local/include -I./ `pkg-config --cflags glib-2.0`
LDFLAGS=`pkg-config --libs glib-2.0`

all: $(SRC)
	$(CC) $(CFLAGS) -o $(TRG) $(SRC) $(LDFLAGS) 

install: $(TRG)
	cp $(TRG) /usr/local/bin

clean:
	rm -rf $(TRG) $(TRG).core
