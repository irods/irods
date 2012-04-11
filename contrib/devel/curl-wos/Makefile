CC=gcc
CFLAGS=-g
CURL_FLAGS=-lcurl

all:	curl-wos 

curl-wos:	curl-wos.c curl-wos.h
	 $(CC) $(CFLAGS) -o $@ $< $(CURL_FLAGS)

clean:
	rm -f curl-wos
