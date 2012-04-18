# This code is for issue 721
CC=gcc
CFLAGS=-g
JSON_FLAGS=-ljson
CURL_FLAGS=-lcurl

all:	curl-wos

curl-wos:	curl-wos.c curl-wos.h
	 $(CC) $(CFLAGS) -o $@ $< $(CURL_FLAGS) $(JSON_FLAGS)

doc:	curl-wos.c curl-wos.h
	doxygen Doxyfile

clean:
	rm -f curl-wos
	rm -rf docs
