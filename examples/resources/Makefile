GCC = g++ 
INC=-I/usr/include/irods/ -I/usr/include/irods/boost -I/usr/include/irods/jansson/src
all:
	${GCC} ${INC} -Dlinux_platform -fPIC -shared -o libexamplefilesystem.so libexamplefilesystem.cpp /usr/lib/libirods_client_plugins.a

clean:
	@rm -f libexamplefilesystem.so
