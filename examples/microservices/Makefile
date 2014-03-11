GCC = g++ 
INC=-I/usr/include/irods/

all: msvc_test hello curl_get

msvc_test:
	${GCC} ${INC} -fPIC -shared -Wno-deprecated -o libirods_msvc_test.so irods_msvc_test.cpp /usr/lib/libirods.a
	
hello:
	${GCC} ${INC} -fPIC -shared -o libirods_hello.so irods_hello.cpp -Wno-deprecated /usr/lib/libirods.a
	
curl_get:
	${GCC} ${INC} -DRODS_SERVER -fPIC -shared -g -Wno-deprecated -o libirods_curl_get.so irods_curl_get.cpp `curl-config --libs` /usr/lib/libirods.a
	
clean:
	@rm -f libirods_msvc_test.so
