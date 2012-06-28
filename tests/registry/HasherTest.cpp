#include "Hasher.h"
#include "MD5Strategy.h"
#include "SHA256Strategy.h"

#include <string>
#include <iostream>
#include <fstream>

using namespace std;
using namespace eirods;

void
generateHash(
    const char* filename)
{
    // Create hasher
    Hasher hasher;
    MD5Strategy* md5Strategy = new MD5Strategy();
    hasher.addStrategy(md5Strategy);
    SHA256Strategy* sha256Strategy = new SHA256Strategy();
    hasher.addStrategy(sha256Strategy);
    
    // Read the file and hash it
    ifstream input(filename, ios_base::in | ios_base::binary);
    if(input.is_open()) {
	char buffer[1024];
	hasher.init();
	while(!input.eof()) {
	    input.read(buffer, 1023);
	    int numRead = input.gcount();
	    hasher.update(buffer, numRead);
	}
	input.close();
	string messageDigest;
	hasher.digest("MD5", messageDigest);
	cout << messageDigest << " ";
	hasher.digest("SHA256", messageDigest);
	cout << messageDigest << endl;
    } else {
	cerr << "ERROR: Unable to open file: " << filename << endl;
    }
}

int
main(
    int argc,
    char* argv[])
{
    int result = 0;
    if(argc != 2) {
	cerr << "ERROR: wrong number of arguments: " << argc << endl;
	cerr << "ERROR: filename must be specified" << endl;
	result = 1;
    } else {
	generateHash(argv[1]);
    }
    return result;
}
