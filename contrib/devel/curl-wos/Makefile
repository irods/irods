# This code is for issue 721, really it is...
CC=gcc
CPPFLAGS=
CFLAGS=-g
JSON_FLAGS=-ljson
CURL_FLAGS=-lcurl
OBJECTS	:=	curl-wos.o curlWosFunctions.o
SRCS=curl-wos.c curlWosFunctions.c

all:	curl-wos

curl-wos: $(OBJECTS)
	 $(CC) $(CFLAGS) $(CPPFLAGS) -o $@ $(SRCS) $(CURL_FLAGS) $(JSON_FLAGS)

doc:	curl-wos.c curl-wos.h
	doxygen Doxyfile

clean:
	rm -f curl-wos *.o
	rm -rf docs
