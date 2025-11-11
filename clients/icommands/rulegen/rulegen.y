%{
#include "rulegen.h"

#define	YYTRACE
#define	YYMAX_READ 0
#define	INTSIZE	long
extern char *yytext;
extern void *stitch(int typ, void *arg1, void *arg2, void *arg3, void *arg4); 
extern int stack_top;
extern FILE *outf;
extern char cutstr[10];
extern char nopstr[10];
char *stack[100];
char *pop_stack(char *stack[]);
int push_stack(char *stack[], char *item);
char *get_stack(char *stack[]);

%}

%union {
        int i;
        long l;
        struct symbol *s;
        struct node *n;
} 
%type <s> cond_expr selection_statement statement statement_list 
%type <s> compound_statement ass_expr iteration_statement identifier
%type <s> ass_expr_list  logical_expr relational_expr
%type <s> rule_list rule action_def action_name arg_list arg_val
%type <s> action_statement first_statement  microserve 
%type <s> execution_statement or_list_statement_list 
%type <s> inputs outputs inp_expr inp_expr_list out_expr out_expr_list

%token <l> LIT
%token <i> CHAR_LIT
%token <s>  STR_LIT NUM_LIT Q_STR_LIT

%token <i> EQ_OP NE_OP AND_OP OR_OP LE_OP GE_OP ACRAC_SEP LIKE NOT 

%token <i> ',' '*' '/' '%' '+' '-' '<' '>' '&' '^' '|' '.' '(' '[' '!' '~' '='
%token <i> '?' ':' '{' ';'
%token <s> PAREXP BRAC STLIST IF ELSE THEN WHILE FOR ASSIGN ASLIST TRUE FALSE
%token <s> ELSEIFELSEIF IFELSEIF DELAY REMOTE PARALLEL ONEOF SOMEOF FOREACH
%token <s> RLLIST RULE ACDEF ARGVAL AC_REAC REL_EXP EMPTYSTMT MICSER ON
%token <s> ONORLIST ORON OR ORONORLIST ORORLIST IFTHEN IFTHENELSE
%token <s> INPUT OUTPUT INPASS INPASSLIST OUTPASS OUTPASSLIST
%start program
%%

program				/* 1 */
	: /* NULL */			{ }
	| program rule_list inputs outputs     	{ print_final($2,$3,$4); }
	;

inputs
        : INPUT inp_expr_list          { $$=$2; }
        ;
outputs
        : OUTPUT out_expr_list          { $$=$2; }
        ;

rule_list  
        : rule                       { $$=stitch(RLLIST,$1,NULL,NULL,NULL); }
        | rule rule_list             { $$=stitch(RLLIST,$1,$2,NULL,NULL); }
        ;
rule    
        : action_def '{' first_statement '}' { $$=stitch(RULE,$1,$3,NULL,NULL); }
        | action_def '{' first_statement statement_list '}' { $$=stitch(RULE,$1,$3,$4,NULL); }
        ;

action_def
        : action_name                  {$$=stitch(ACDEF, $1,NULL,NULL,NULL); }
        | action_name '(' arg_list ')'  {$$=stitch(ACDEF, $1,$3,NULL,NULL); }
        ;

microserve
        : action_name                  {$$=stitch(MICSER, $1,NULL,NULL,NULL); }
        | action_name '(' arg_list ')'  {$$=stitch(MICSER, $1,$3,NULL,NULL); }
        ;

action_name
        : identifier                    
        ;

arg_list
        : arg_val                       { $$=stitch (ARGVAL, $1,NULL,NULL,NULL); }
        | arg_val ',' arg_list           { $$=stitch (ARGVAL, $1,$3,NULL,NULL); }
        ;
arg_val 
        : STR_LIT                      { $$=stitch (STR_LIT,yytext,NULL,NULL,NULL); }
        | Q_STR_LIT                    { $$=stitch (Q_STR_LIT,yytext,NULL,NULL,NULL); }
	| NUM_LIT                      { $$=stitch (NUM_LIT,yytext,NULL,NULL,NULL); }
        ;

first_statement 
        :  selection_statement         { $$=stitch(STLIST,$1,NULL,NULL,NULL); }
        |                              { $$=stitch (EMPTYSTMT,NULL,NULL,NULL,NULL); }
        ; 
compound_statement		/* 39 */
	: '{' '}'			{ $$=stitch(BRAC,NULL,NULL,NULL,NULL); }
	| '{' statement_list '}'	{ $$=stitch(BRAC,$2,NULL,NULL,NULL); }
	;

statement_list			/* 40 */
	: statement
				{ $$=stitch(STLIST,$1,NULL,NULL,NULL); }
	| statement_list statement
				{ $$=stitch(STLIST,$1,$2,NULL,NULL); }
	;

statement			/* 36 */
	: selection_statement
	| iteration_statement
        | compound_statement
        | action_statement ';' {}
        | ass_expr ';' {}
        | execution_statement
	;

selection_statement		/* 41 */
	: ON '(' cond_expr ')' statement    { $$=stitch(ON,$3,$5,NULL,NULL); }
	| ON '(' cond_expr ')' statement or_list_statement_list 
					   { $$=stitch(ONORLIST,$3,$5,$6,NULL); }
	;

iteration_statement		/* 42 */
	: WHILE '(' cond_expr ')' statement
					{ $$=stitch(WHILE,$3,$5,NULL,NULL); }
	| FOR '(' ass_expr_list ';' cond_expr ';'  ass_expr_list ')' statement
					{ $$=stitch(FOR,$3,$5,$7,$9); }
	| IF '(' cond_expr ')' THEN statement 
					{ $$=stitch(IFTHEN,$3,$6,NULL,NULL); }
	| IF '(' cond_expr ')' THEN statement ELSE statement
					{ $$=stitch(IFTHENELSE,$3,$6,$8,NULL); }
	;
or_list_statement_list 
        : ORON '(' cond_expr ')' statement    { $$=stitch(ORON,$3,$5,NULL,NULL); }
	| OR  statement                       { $$=stitch(OR,$2,NULL,NULL,NULL); }
	| ORON '(' cond_expr ')' statement or_list_statement_list 
					   { $$=stitch(ORONORLIST,$3,$5,$6,NULL); }
	| OR statement or_list_statement_list 
					   { $$=stitch(ORORLIST,$2,$3,NULL,NULL); }
	;

action_statement
        : microserve ACRAC_SEP microserve   { $$=stitch(AC_REAC,$1,$3,NULL,NULL); }
        | microserve                        { $$=stitch(AC_REAC,$1,NULL,NULL,NULL); }
        ;

execution_statement
        : DELAY '(' cond_expr ')' statement
					{ $$=stitch(DELAY,$3,$5,NULL,NULL); }
        | REMOTE '(' identifier ',' cond_expr ')' statement
					{ $$=stitch(REMOTE,$3,$5,$7,NULL); }
        | PARALLEL '(' cond_expr ')' statement
					{ $$=stitch(PARALLEL,$3,$5,NULL,NULL); }
        | ONEOF    statement
					{ $$=stitch(ONEOF,$2,NULL,NULL,NULL); }
        | SOMEOF  '(' identifier ')'   statement
					{ $$=stitch(SOMEOF,$3,$5,NULL,NULL); }
        | FOREACH  '(' identifier ')'   statement
					{ $$=stitch(FOREACH,$3,$5,NULL,NULL); }
        ;
 /******************************** Expressions *************************/
inp_expr			/* 45 */
	: identifier  '=' cond_expr        { $$=stitch(INPASS,$1,$3,NULL,NULL); }
        ;
inp_expr_list
	: inp_expr
        | inp_expr ',' inp_expr_list
				{ $$=stitch(INPASSLIST,$1,$3,NULL,NULL); }
	;

out_expr			/* 45 */
	: arg_val
out_expr_list
	: out_expr
        | out_expr ',' out_expr_list
				{ $$=stitch(OUTPASSLIST,$1,$3,NULL,NULL); }
	;

ass_expr			/* 45 */
	: identifier  '=' cond_expr        { $$=stitch(ASSIGN,$1,$3,NULL,NULL); }

ass_expr_list
	: ass_expr
        | ass_expr ',' ass_expr_list
				{ $$=stitch(ASLIST,$1,$3,NULL,NULL); }
	;

cond_expr		/* 47 */
	: logical_expr
        | '(' logical_expr ')' { $$=stitch(PAREXP, $2,NULL,NULL,NULL); }
        | cond_expr AND_OP cond_expr    { $$=stitch($2,$1,$3,NULL,NULL); }
        | cond_expr OR_OP cond_expr    { $$=stitch($2,$1,$3,NULL,NULL); }
        | cond_expr '+'  cond_expr    { $$=stitch($2,$1,$3,NULL,NULL); }
        | cond_expr '-'  cond_expr    { $$=stitch($2,$1,$3,NULL,NULL); }
	;

logical_expr			/* 54 */
        : TRUE                  {$$=stitch (TRUE,NULL,NULL,NULL,NULL); }
        | FALSE                 {$$=stitch (FALSE,NULL,NULL,NULL,NULL); }
        | relational_expr
	| logical_expr EQ_OP logical_expr
				{ $$=stitch (REL_EXP,$1,"==",$3,NULL); }
	| logical_expr NE_OP logical_expr
				{ $$=stitch (REL_EXP,$1,"!=",$3,NULL); }
	| logical_expr '<' logical_expr
				{ $$=stitch (REL_EXP,$1,"<",$3,NULL); }
	| logical_expr '>' logical_expr
				{ $$=stitch (REL_EXP,$1,">",$3,NULL); }
	| logical_expr LE_OP logical_expr
				{ $$=stitch (REL_EXP,$1,"<=",$3,NULL); }
	| logical_expr GE_OP logical_expr
				{ $$=stitch (REL_EXP,$1,">=",$3,NULL); }
        | logical_expr LIKE logical_expr
                               { $$=stitch (REL_EXP,$1,"like",$3,NULL); }
        | logical_expr NOT LIKE logical_expr
                               { $$=stitch (REL_EXP,$1,"not like",$4,NULL); }
        ;
relational_expr
        :  STR_LIT              { $$=stitch (STR_LIT,yytext,NULL,NULL,NULL); }
	|  NUM_LIT              { $$=stitch (NUM_LIT,yytext,NULL,NULL,NULL); }
	|  Q_STR_LIT              { $$=stitch (Q_STR_LIT,yytext,NULL,NULL,NULL); }
        ;
identifier
        :  STR_LIT              { $$=stitch (STR_LIT,yytext,NULL,NULL,NULL); }
        |  Q_STR_LIT              { $$=stitch (Q_STR_LIT,yytext,NULL,NULL,NULL); }
	|  NUM_LIT              { $$=stitch (NUM_LIT,yytext,NULL,NULL,NULL); }
        ;
%%


void *stitch(int typ, void *inarg1, void  *inarg2, void  *inarg3, void  *inarg4)
{

  int i,j,k;
  char tmpStr[10000];
  char *s, *t;
  char *u, *v;
  char *arg1, *arg2, *arg3, *arg4;

  arg1 = (char *) inarg1;
  arg2 = (char *) inarg2;
  arg3 = (char *) inarg3;
  arg4 = (char *) inarg4;

  switch (typ) {
  case BRAC:
    return(arg1);
    break;
  case STLIST:
    if (arg2 == NULL)
      return(arg1);
    else {
      if ((t = strstr(arg1,":::")) != NULL) {
	*t = '\0';
	if ((s = strstr(arg2,":::")) != NULL) {
	  *s = '\0';
	  sprintf(tmpStr,"%s##%s%s%s##%s", 
		  arg1, arg2, ":::", (char *) t + strlen(":::"), 
		  (char *) s + strlen(":::"));
	}
	else {
	  sprintf(tmpStr,"%s##%s%s%s", 
		  arg1, arg2, ":::", (char *) t + strlen(":::"));
	}
      }
      else {
	sprintf(tmpStr,"%s##%s", arg1, arg2);
      }
    }
    break;
  case ASSIGN:
    sprintf(tmpStr,"assign(%s,%s):::nop", arg1, arg2);
    break;
  case INPASS:
    stripEndQuotes(arg2);
    sprintf(tmpStr,"%s=%s", arg1, arg2);
    break;
  case INPASSLIST:
    sprintf(tmpStr,"%s%%%s", arg1, arg2);
    break;
  case OUTPASSLIST:
    sprintf(tmpStr,"%s%%%s", arg1, arg2);
    break;
  case PAREXP:
    sprintf(tmpStr,"(%s)", arg1);
    break;
  case ORON:
    sprintf(tmpStr,"%s|%s%s",arg1,cutstr,arg2);
    break;
  case OR:
    sprintf(tmpStr,"|%s%s",cutstr,arg1);
    break;
  case ORONORLIST:
    sprintf(tmpStr,"%s|%s%s;;;%s",arg1,cutstr,arg2,arg3);
    break;
  case ORORLIST:
    sprintf(tmpStr,"|%s%s;;;%s",cutstr,arg1,arg2);
    break;
  case ON:
    sprintf(tmpStr,"%s|%s%s",  arg1, cutstr,arg2);
    break;
  case ONORLIST:
    sprintf(tmpStr,"%s|%s%s;;;%s",arg1,cutstr,arg2,arg3);
    break;
  case WHILE:
    if ((t = strstr(arg2,":::")) != NULL) {
      *t = '\0';
      sprintf(tmpStr,"whileExec(%s,%s,%s):::nop", arg1, arg2,(char *) t + strlen(":::"));
      *t = ':';
    }
    else {
      sprintf(tmpStr,"whileExec(%s,%s,%s):::nop", arg1, arg2,"''");
    }
    break;
  case IFTHEN:
    if ((t = strstr(arg2,":::")) != NULL) {
      *t = '\0';
      sprintf(tmpStr,"ifExec(%s,%s,%s,nop,nop):::nop", arg1,arg2, (char *) t + strlen(":::")); 
      *t = ':';
    }
    else {
      sprintf(tmpStr,"ifExec(%s,%s,nop,nop,nop):::nop", arg1,arg2);
    }
    break;
  case IFTHENELSE:
    if ((t = strstr(arg2,":::")) != NULL) {
      *t = '\0';
      if ((s = strstr(arg3,":::")) != NULL) {
	*s = '\0';
	sprintf(tmpStr,"ifExec(%s,%s,%s,%s,%s):::nop", arg1,arg2,(char *) t + strlen(":::"),
		arg3, (char *) s + strlen(":::"));
	*s = ':';
      }
      else {
	sprintf(tmpStr,"ifExec(%s,%s,%s,%s,nop):::nop", arg1,arg2, (char *) t + strlen(":::"), arg3); 
      }
      *t = ':';
    }
    else {
      if ((s = strstr(arg3,":::")) != NULL) {
	*s = '\0';
	printf(tmpStr,"ifExec(%s,%s,nop,%s,%s):::nop", arg1,arg2, arg3, (char *) s + strlen(":::"));
	*s = ':';
      }
      else {
	sprintf(tmpStr,"ifExec(%s,%s,nop,%s,nop):::nop", arg1,arg2,arg3);
      }
    }
    break;
  case DELAY:
    if ((t = strstr(arg2,":::")) != NULL) {
      *t = '\0';
      sprintf(tmpStr,"delayExec(%s,%s,%s):::nop", arg1, arg2,(char *) t + strlen(":::"));
      *t = ':';
    }
    else {
      sprintf(tmpStr,"delayExec(%s,%s,%s):::nop", arg1, arg2,"''");
    }
    break;
  case REMOTE:
    if ((t = strstr(arg3,":::")) != NULL) {
      *t = '\0';
      sprintf(tmpStr,"remoteExec(%s,%s,%s,%s):::nop", arg1, arg2, arg3,(char *) t + strlen(":::"));
      *t = ':';
    }
    else {
      sprintf(tmpStr,"remoteExec(%s,%s,%s,%s):::nop", arg1, arg2, arg3,"''");
    }
    break;
  case PARALLEL:
    if ((t = strstr(arg2,":::")) != NULL) {
      *t = '\0';
      sprintf(tmpStr,"parallelExec(%s,%s,%s):::nop", arg1, arg2,(char *) t + strlen(":::"));
      *t = ':';
    }
    else {
      sprintf(tmpStr,"parallelExec(%s,%s,%s):::nop", arg1, arg2,"''");
    }
    break;
  case SOMEOF:
    if ((t = strstr(arg2,":::")) != NULL) {
      *t = '\0';
      sprintf(tmpStr,"someOfExec(%s,%s,%s):::nop", arg1, arg2,(char *) t + strlen(":::"));
      *t = ':';
    }
    else {
      sprintf(tmpStr,"someOfExec(%s,%s,%s):::nop", arg1, arg2,"''");
    }
    break;
  case ONEOF:
    if ((t = strstr(arg1,":::")) != NULL) {
      *t = '\0';
      sprintf(tmpStr,"oneOfExec(%s,%s):::nop", arg1,(char *) t + strlen(":::"));
      *t = ':';
    }
    else {
      sprintf(tmpStr,"oneOfExec(%s,%s):::nop", arg1, "''");
    }
    break;
  case FOREACH:
    if ((t = strstr(arg2,":::")) != NULL) {
      *t = '\0';
      sprintf(tmpStr,"forEachExec(%s,%s,%s):::nop", arg1, arg2,(char *) t + strlen(":::"));
      *t = ':';
    }
    else {
      sprintf(tmpStr,"forEachExec(%s,%s,%s):::nop", arg1, arg2,"''");
    }
    break;
  case FOR:
    if ((u =  strstr(arg1,":::"))!= NULL) *u = '\0';
    if ((v =  strstr(arg3,":::"))!= NULL) *v = '\0';
    if ((t = strstr(arg4,":::")) != NULL) {
      *t = '\0';
      sprintf(tmpStr,"forExec(%s,%s,%s,%s,%s):::nop", 
	      arg1, arg2,arg3,arg4,(char *) t + strlen(":::"));
      *t = ':';
    }
    else {
      sprintf(tmpStr,"forExec(%s,%s,%s,%s,%s):::nop", 
	      arg1, arg2,arg3,arg4,"''");
    }
    if (u != NULL) *u = ':';
    if (v != NULL) *v = ':';
    break;
  case ASLIST:
    break;
  case RLLIST:
    if (arg2 == NULL)
      return(arg1);
    else 
      sprintf(tmpStr,"%s\n%s", arg1, arg2);
    break;
  case RULE:
    if (yydebug == 1)
      if (arg3 == NULL)
	printf("BBB:%s:%s:NOARG3\n",arg1, arg2);
      else
	printf("BBB:%s:%s:%s\n",arg1, arg2, arg3);
    u = arg2;
    strcpy(tmpStr,"");
    while (u != NULL) {
      v = strstr(u,";;;");
      if (v != NULL)
	*v = '\0';
      if (arg3 == NULL) {
	if (!strcmp(u," ")) 
	  return(arg1);
	else {
	  if ((t = strstr(u,":::")) != NULL) {
	    *t = '\0';
	    sprintf(tmpStr,"%s%s|%s|%s%s",tmpStr, arg1, u,nopstr, (char *) t + strlen(":::"));
	  }
	  else {
	    sprintf(tmpStr,"%s%s|%s",tmpStr, arg1,u); 
	  }
	}
      }
      else { 
	if (!strcmp(u," ")) {
	  if ((t = strstr(arg3,":::")) != NULL) {
	    *t = '\0';
	    sprintf(tmpStr,"%s%s||%s|%s%s",tmpStr, arg1, arg3,nopstr, (char *) t + strlen(":::"));
	  }
	  else 
	    sprintf(tmpStr,"%s%s||%s",tmpStr, arg1, arg3);
	}
	else {
	  if ((t = strstr(u,":::")) != NULL) {
	    *t = '\0';
	    if ((s = strstr(arg3,":::")) != NULL) {
	      *s = '\0';
	      sprintf(tmpStr,"%s%s|%s##%s|%s%s##%s ",tmpStr, 
		      arg1, u, arg3,  nopstr,  (char *) t + strlen(":::"), 
		      (char *) s + strlen(":::"));
	    }
	    else {
	      sprintf(tmpStr,"%s%s|%s##%s|%s%s",tmpStr, 
		      arg1, u, arg3,   nopstr, (char *) t + strlen(":::"));
	    } 
	  }
	  else {
	    if ((s = strstr(arg3,":::")) != NULL) {
	      *s = '\0';
	      sprintf(tmpStr,"%s%s|%s##%s|%s%s",tmpStr,
		      arg1, u, arg3,  nopstr,(char *) s + strlen(":::"));
	    }
	    else
	      sprintf(tmpStr,"%s%s|%s##%s",tmpStr, arg1, u, arg3);
	  }
	}
      }
      if (v == NULL)
	break;
      *v = ';';
      u = v + 3;
      strcat(tmpStr,";;;");
    }
    pop_stack(stack);
    break;
  case ACDEF:
    if (arg2 == NULL)
      sprintf(tmpStr,"%s", arg1);
    else 
      sprintf(tmpStr,"%s(%s)", arg1, arg2);
    if (push_stack(stack, tmpStr) < 0) {
          printf("Stack OverFlow for Rule Depth");
	  exit(1);
    }
    break;
  case MICSER:
    if (arg2 == NULL)
      return(arg1);
    else 
      sprintf(tmpStr,"%s(%s)", arg1, arg2);
    break;
  case ARGVAL:
    if (arg2 == NULL)
      return(arg1);
    else 
      sprintf(tmpStr,"%s,%s", arg1, arg2);
    break;
  case AC_REAC:
    if (arg2 == NULL)
      sprintf(tmpStr,"%s:::nop", arg1);
    else 
      sprintf(tmpStr,"%s:::%s", arg1, arg2);
    break;
  case REL_EXP:
    sprintf(tmpStr,"%s %s %s",  arg1, arg2, arg3);
    break;
  case TRUE:
    sprintf(tmpStr,"%d",1);
    break;
  case FALSE:
    sprintf(tmpStr,"%d",0);
    break;
  case STR_LIT:
    sprintf(tmpStr,"%s", arg1);
    break;
  case Q_STR_LIT:
    sprintf(tmpStr,"\"%s\"", arg1);
    stripEscFromQuotedStr(tmpStr);
    break;
  case NUM_LIT:
    sprintf(tmpStr,"%s", arg1);
    break;
  case EMPTYSTMT:
       strcpy(tmpStr," "); 
    /* strcpy(tmpStr,"cut");*/
    /* strcpy(tmpStr,"|");*/
    break;
  case AND_OP:
    sprintf(tmpStr,"%s %s %s", arg1, "&&", arg2);
    break;
  case OR_OP:
    sprintf(tmpStr,"%s %s %s", arg1, "!!", arg2);
    break;
  case '+':
    sprintf(tmpStr,"%s %s %s", arg1, "+", arg2);
    break;
  case '-':
    sprintf(tmpStr,"%s %s %s", arg1, "-", arg2);
    break;
  default:
    printf("Error in stictch typ: %i", typ);
    exit(1);
    break;
  }
  s = strdup(tmpStr);
  if (yydebug == 1)
    printf("AAA:%i: %s\n",typ,tmpStr);
  return(s);

}

int
print_final ( char *out, char* input, char *output)
{
  
  char *s, *t;

  s = out;
  while ((t = strstr(s, ";;;")) != NULL) {
    *t = '\0';
    fprintf(outf,"%s\n",s);
    *t = ';';
    s = t+3;
  }
 fprintf(outf,"%s\n",s);
 fprintf(outf,"%s\n%s\n", input, output);
 return(0);
}

int 
stripEndQuotes(char *s)
{
  char *t;

  if (*s == '\'' ||*s == '"' ) {
    memmove(s,s+1,strlen(s+1)+1);
    t = s+strlen(s)-1;
    if (*t == '\'' ||*t == '"' )
      *t = '\0';
  }
  /* made it so that end quotes are removed only if quoted initially */
  return (0);
}

int
stripEscFromQuotedStr(char *str)
{
 char *s, *t, *u;
 s = strdup(str + 1);
 t = s;
 u = str;
 while (*t != '\0') {
   if (*t == '\\' && (*(t+1) == '"' || *(t+1) == '\\')) {
     t++;
   }
   *u = *t;
   u++;
   t++;
 }
 u--;
 while (*u != '"')
   u--;
 *u = '\0';
}

