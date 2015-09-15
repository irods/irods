GCC = g++
INC=-I/usr/include/irods/ -I/usr/include/irods/boost/

all: msvc_test hello curl_get

msvc_test:
	${GCC} ${INC} -fPIC -shared -Wno-deprecated -o libirods_msvc_test.so msvc_test.cpp

hello:
	${GCC} ${INC} -fPIC -shared -o libirods_hello.so hello.cpp -Wno-deprecated

curl_get:
	${GCC} ${INC} -DRODS_SERVER -fPIC -shared -g -Wno-deprecated -o libirods_curl_get.so curl_get.cpp `curl-config --libs`

clean:
	@rm -f ./*.so
