GCC = g++ 
INC=-I/usr/include/eirods/
all:
	${GCC} ${INC} -Dlinux_platform -fPIC -shared -o libexamplefilesystem.so libexamplefilesystem.cpp /usr/lib/libeirods.a
	${GCC} ${INC} -Dlinux_platform -fPIC -shared -o libpassthru.so          libpassthru.cpp          /usr/lib/libeirods.a
