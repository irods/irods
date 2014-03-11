GCC = g++ 
INC=-I/usr/include/irods/
all:
	${GCC} ${INC} -Dlinux_platform -fPIC -shared -o libexamplefilesystem.so libexamplefilesystem.cpp /usr/lib/libirods.a

clean:
	@rm -f libexamplefilesystem.so
