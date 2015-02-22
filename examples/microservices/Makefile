GCC = g++
INC=-I/usr/include/irods/ -I/usr/include/irods/boost/

all: msvc_test hello curl_get

msvc_test:
	${GCC} ${INC} -fPIC -shared -Wno-deprecated -o libirods_msvc_test.so msvc_test.cpp /usr/lib/libirods_client.a

hello:
	${GCC} ${INC} -fPIC -shared -o libirods_hello.so hello.cpp -Wno-deprecated /usr/lib/libirods_client.a

curl_get:
	${GCC} ${INC} -DRODS_SERVER -fPIC -shared -g -Wno-deprecated -o libirods_curl_get.so curl_get.cpp `curl-config --libs` /usr/lib/libirods_client.a

clean:
	@rm -f ./*.so
