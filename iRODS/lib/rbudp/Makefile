RBUDP_OBJ = obj/QUANTAnet_rbudpBase_c.o obj/QUANTAnet_rbudpSender_c.o	\
obj/QUANTAnet_rbudpReceiver_c.o

LDFLAGS = $(RBUDP_OBJ)
transbin = bin/recvfile bin/sendfile

all: $(RBUDP_OBJ)

test: $(RBUDP_OBJ) $(transbin)
 
obj/QUANTAnet_rbudpBase_c.o: src/QUANTAnet_rbudpBase_c.c include/QUANTAnet_rbudpBase_c.h
	gcc -g -Wall -c -Iinclude -o obj/QUANTAnet_rbudpBase_c.o src/QUANTAnet_rbudpBase_c.c
obj/QUANTAnet_rbudpSender_c.o: src/QUANTAnet_rbudpSender_c.c include/QUANTAnet_rbudpSender_c.h
	gcc -g -Wall -c -Iinclude -o obj/QUANTAnet_rbudpSender_c.o src/QUANTAnet_rbudpSender_c.c
obj/QUANTAnet_rbudpReceiver_c.o: src/QUANTAnet_rbudpReceiver_c.c include/QUANTAnet_rbudpReceiver_c.h
	gcc -g -Wall -c -Iinclude -o obj/QUANTAnet_rbudpReceiver_c.o src/QUANTAnet_rbudpReceiver_c.c
obj/recvfile.o: src/recvfile.c $(RBUDP_OBJ)
	gcc -g -Wall -c -Iinclude -o obj/recvfile.o src/recvfile.c
obj/sendfile.o: src/sendfile.c $(RBUDP_OBJ)
	gcc -g -Wall -c -Iinclude -o obj/sendfile.o src/sendfile.c

bin/recvfile: obj/recvfile.o
	gcc -o $@ $^ $(LDFLAGS)
bin/sendfile: obj/sendfile.o
	gcc -o $@ $^ $(LDFLAGS)

clean:
	rm -f $(RBUDP_OBJ) $(transbin) obj/recvfile.o obj/sendfile.o


