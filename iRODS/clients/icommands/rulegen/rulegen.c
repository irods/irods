#include <stdlib.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#define YYDEBUG 1

extern char *optarg;

static FILE *fin;
extern FILE *yyin;
extern void yyparse();
extern int yydebug;

int stack_top =  0;
char *stack[100];
FILE *outf;
char cutstr[10];
char nopstr[10];
char *pop_stack(char *stack[])
{
   if (stack_top == 0)
     return(NULL);
   stack_top--;
   if (yydebug == 1) 
     printf("SSS:Pop %i:%s\n",stack_top,stack[stack_top]);
   return(stack[stack_top]);
}

int push_stack(char *stack[], char *item)
{
  if (stack_top == 100)
    return(-1);
  stack[stack_top] = strdup(item);
  stack_top++;
  if (yydebug == 1)  
    printf("SSS:Push %i:%s\n",stack_top,item);
  return(stack_top);
}

char *get_stack(char *stack[])
{
   if (stack_top == 0)
     return(NULL);
  if (yydebug == 1)  
    printf("SSS:Get %i:%s\n",stack_top,stack[stack_top - 1]);
  return(stack[stack_top - 1]);
}

usage( char *com)
{
    printf ("Usage: %s [-s] <infile>\n",com);

}
main (int argc, char **argv)
{
    int c;
    char outFile[250];
  

  if (argc < 2) {
    printf ("Usage: %s [-sch] [-o <outFile>] <infile>\n",argv[0]);
    printf ("  -s : silent mode. no debugging statements printed.\n");
    printf ("  -c : disable cut so that later rule definitions will be tried.\n");
    printf ("  -h : prints this help.\n");
    exit(1);
  }
#ifdef YYDEBUG
  yydebug = 1;
#endif 
  outFile[0] = '\0';
  strcpy(cutstr,"cut##");
  strcpy(nopstr,"nop##");

  while ((c=getopt(argc, argv,"scho:")) != EOF) {
        switch (c) {
            case 's':
                yydebug = 0;
                break;
            case 'c':
	        strcpy(cutstr,"");
		strcpy(nopstr,"");
                break;
            case 'o':
	        strcpy(outFile,optarg);
		
                break;
            case 'h':
            default:
                usage (argv[0]);
                exit (1);
        }
    }




  if ((fin = fopen (argv[argc -1],"rt")) == NULL) {
    printf ("Cant open '%s' for input\n",argv[1]);
    exit(1);
  }
  else 
    yyin = fin;

  if (strlen(outFile) > 0) {
    if ((outf = fopen (outFile,"w")) == NULL) {
      printf ("Cant open '%s' for output\n",outFile);
      exit(1);
    }
  }
  else {
    outf = stdout;
  }
  yyparse ();
  fclose(fin);
  fclose(outf);
  exit (0);
}
int yywrap()
{
	return 1;
}
void yyerror(char *fmt, ...)
{
	/* extern int yylineno; */
	extern int column, line_num;
	extern char error_line_buffer[256];
	va_list va;

	va_start(va, fmt);
	fprintf (stdout, "%s\n", error_line_buffer);
	fprintf (stdout, "\n%*s", column, "^");
	vfprintf(stdout, fmt, va);
	fprintf (stdout, ":Line %d\n", line_num);

}

void yycomment( char *s)
{

}
