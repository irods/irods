#include <stdio.h>


int main (int argc, char** argv) {

	FILE* fp;
	int	i;
	char x[1024];
	int limit=25000;

	if (argc > 1) {
	   int j;
	   j = atoi(argv[1]);
	   if (j > 0) limit=j;
	}

	fp = fopen ("bigfile", "w");

	for (i=0; i<1024; i++)
		x[i]=(char) i;

	for (i=0; i<limit; i++)
		fwrite(x, sizeof(x[0]), sizeof(x)/sizeof(x[0]), fp);

	fclose (fp);

}
