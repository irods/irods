
include ../../iRODS/config/platform.mk
include ../../iRODS/config/config.mk

GCC = g++ -DRODS_SERVER -DZIP_EXEC_PATH=\"$(ZIP_EXEC_PATH)\" -DUNZIP_EXEC_PATH=\"$(UNZIP_EXEC_PATH)\"
INC=-I ../../iRODS/lib/core/include/ -I ../../iRODS/lib/api/include/ -I../../iRODS/lib/md5/include -I../../iRODS/server/core/include/ -I../../iRODS/server/icat/include/ -I../../iRODS/server/re/include/ -I../../iRODS/server/drivers/include/ -I../../external/libarchive-3.0.4/libarchive/
all:
	${GCC} ${INC} -Dlinux_platform -fPIC -shared -o libunixfilesystem.so  libunixfilesystem.cpp 
	${GCC} ${INC} -Dlinux_platform -fPIC -shared -o libpassthru.so  libpassthru.cpp 
	${GCC} ${INC} -Dlinux_platform -fPIC -shared -o libnonblocking.so     libnonblocking.cpp 
	${GCC} ${INC} -Dlinux_platform -fPIC -lboost_filesystem  -shared -o libstructfile.so libstructfile.cpp \
        ../../external/libarchive*/./.libs/libarchive.a ../../external/bzip2*/libbz2.a ../../external/libzip*/./lib/.libs/libzip.a ../../external/zlib*/libz.a  \
        ../../external/boost*/stage/lib/libboost_filesystem.a ../../external/boost*/stage/lib/libboost_system.a

