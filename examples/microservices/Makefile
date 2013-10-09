GCC = g++ 
INC=-I/usr/include/eirods/

all: msvc_test hello curl_get

msvc_test:
	${GCC} ${INC} -fPIC -shared -Wno-deprecated -o libeirods_msvc_test.so eirods_msvc_test.cpp /usr/lib/libeirods.a
	
hello:
	${GCC} ${INC} -fPIC -shared -o libeirods_hello.so eirods_hello.cpp -Wno-deprecated /usr/lib/libeirods.a
	
curl_get:
	${GCC} ${INC} -DRODS_SERVER -fPIC -shared -g -Wno-deprecated -o libeirods_curl_get.so eirods_curl_get.cpp `curl-config --libs` /usr/lib/libeirods.a
	
