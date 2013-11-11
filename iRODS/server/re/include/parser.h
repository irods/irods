/* For copyright information please refer to files in the COPYRIGHT directory
 */


#ifndef PARSER_H
#define PARSER_H

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


#ifndef DEBUG
#include "objInfo.h"
#include "reHelpers1.h"
#endif

#include "reconstants.h"
#include "restructs.h"
#include "utils.h"
#include "region.h"
#include "hashtable.h"


typedef struct op {
    char* string;
    int arity;
    int prec;
} Op;

#define num_ops 31
extern Op new_ops[];

typedef struct pointer {
	FILE *fp; /* file */
	char buf[POINTER_BUF_SIZE]; /* buffer */
	unsigned int len; /* len of string in buf */
	unsigned int p; /* pointer to next char in buf */
	unsigned long fpos; /* position of the beginning of the buffer in file */
    unsigned int strp; /* pointer to next char in strbuf */
    char *strbuf; /* string buffer */
    unsigned int strlen;
    int isFile;
    char *base; /* f + filename without extension, or s + source */
} Pointer;

#define pushRule(rs, r) ((rs)->rules[(rs)->len++] = (r))


typedef struct {
    Node *nodeStack[1024];
    int nodeStackTop;
    int stackTopStack[1024];
    int stackTopStackTop;
    int error;
    int prec;
    int backwardCompatible;
    Node *errnode;
    Label errloc;
    char errmsgbuf[ERR_MSG_LEN];
    Hashtable *symtable;
    rError_t *errmsg;
    Region *region;
    Token tokenQueue[1024];
    int tqp;
    int tqtop;
    int tqbot;
} ParserContext;

#define PUSH(n) (context->nodeStack[(context->nodeStackTop)++] = n)
#define POP (context->nodeStack[--(context->nodeStackTop)])
#define NEXT_TOKEN_BASIC NEXT_TOKEN(0)
#define NEXT_TOKEN_EXT NEXT_TOKEN(1)
#define NEXT_TOKEN(ext) \
{ \
    FPOS; \
    token = nextTokenRuleGen(e, context, rulegen, ext); \
    if(token->type==N_ERROR) { \
        context->error=1; \
        if(pos.exprloc > context->errloc.exprloc) context->errloc = pos; \
        break;\
    } \
}
#define TOKEN_TYPE(t) (token->type == (t))
#define TOKEN_TEXT(str) (strcmp(token->text, (str))==0)
#define PUSHBACK pushback(e, token, context)
#define FPOS (getFPos(&pos, e, context))
#define UPDATE_ERR_LOC if(FPOS->exprloc > context->errloc.exprloc) {context->errloc = *FPOS;}
#define CASCADE(x) \
{\
        Node *_ncascade = (x); \
        if(_ncascade == NULL || _ncascade->nodeType == N_ERROR) { \
                UPDATE_ERR_LOC; \
                context->error = 1; \
                break;\
        } else { \
                PUSH(_ncascade); \
        } \
}
#define BUILD_NODE(type,cons,loc,deg, consume) \
        if(context->error==0){Node *var = newNode((type), (cons), (loc), context->region); \
            if(deg!=0) { \
                Node **subs = setDegree(var, (deg), context->region); \
                int counter; \
                for(counter = 1;counter <= (deg);counter ++) {\
                    subs[(deg)-counter] = context->nodeStack[context->nodeStackTop-counter];\
                } \
            } \
            context->nodeStackTop -= (consume); \
            CASCADE(var);}

#define BUILD_APP_NODE(cons,loc,deg) \
		BUILD_NODE(N_TUPLE, TUPLE, loc, deg, deg); \
		BUILD_NODE(TK_TEXT, cons, loc, 0, 0); \
		SWAP; \
		BUILD_NODE(N_APPLICATION, APPLICATION, loc, 2, 2); \

#define PARSER_FUNC_PROTO(l) \
void CONCAT(nextRuleGen, l)(Pointer* e, ParserContext *context)
#define PARSER_FUNC_PROTO1(l, p) \
void CONCAT(nextRuleGen, l)(Pointer* e, ParserContext *context, p)
#define PARSER_FUNC_PROTO2(l, p, q) \
void CONCAT(nextRuleGen, l)(Pointer* e, ParserContext *context, p, q)
#define PARSER_BEGIN(l) \
    Label start; \
    Label pos; \
    Token *token; (void)token; \
    skipWhitespace(e); \
    getFPos(&start, (e), context); \
    do {

#define PARSER_FUNC_BEGIN(l) \
PARSER_FUNC_PROTO(l) { \
    PARSER_BEGIN(l)
#define PARSER_FUNC_BEGIN1(l, p) \
PARSER_FUNC_PROTO1(l, p) { \
    PARSER_BEGIN(l)
#define PARSER_FUNC_BEGIN2(l, p, q) \
PARSER_FUNC_PROTO2(l, p, q) { \
    PARSER_BEGIN(l)

#define PARSER_FUNC_END(l) \
    PARSER_END(l) \
}
#define PARSER_END(l) \
	} while(0);

#define SWAP \
{\
    Node *node = POP;\
    Node *node2 = POP;\
    PUSH(node);\
    PUSH(node2);\
}
#define UNZIP(n) \
{ \
    int i; \
    Node *node[1024]; \
    for(i=0;i<n;i++) { \
        node[i] = context->nodeStack[context->nodeStackTop - 2*(n-i) + 1]; \
        context->nodeStack[context->nodeStackTop-2*n+i] = context->nodeStack[context->nodeStackTop-2*n+2*i]; \
    } \
    for(i=0;i<n;i++) { \
        context->nodeStack[context->nodeStackTop-n+i] = node[i]; \
    } \
}

#define TTEXT(x) \
    NEXT_TOKEN(0); \
    if(!((TOKEN_TYPE(TK_TEXT)||TOKEN_TYPE(TK_OP)||TOKEN_TYPE(TK_MISC_OP)) && TOKEN_TEXT(x))) { \
        context->error = 1; \
        if(pos.exprloc > context->errloc.exprloc) context->errloc = pos; \
        break; \
    }
#define TTEXT2(x,y) \
    NEXT_TOKEN(0); \
    if(!((TOKEN_TYPE(TK_TEXT)||TOKEN_TYPE(TK_OP)||TOKEN_TYPE(TK_MISC_OP)) && (TOKEN_TEXT(x)||TOKEN_TEXT(y)))) { \
        context->error = 1; \
        if(pos.exprloc > context->errloc.exprloc) context->errloc = pos; \
        break; \
    }
#define TTEXT3(x,y,z) \
    NEXT_TOKEN(0); \
    if(!((TOKEN_TYPE(TK_TEXT)||TOKEN_TYPE(TK_OP)||TOKEN_TYPE(TK_MISC_OP)) && (TOKEN_TEXT(x)||TOKEN_TEXT(y)||TOKEN_TEXT(z)))) { \
        context->error = 1; \
        if(pos.exprloc > context->errloc.exprloc) context->errloc = pos; \
        break; \
    }
#define TTEXT_LOOKAHEAD(x) \
    TTEXT(x); \
    PUSHBACK;
#define TTYPE(x) \
    NEXT_TOKEN(x==TK_PATH); \
    if(!TOKEN_TYPE(x)) { \
        context->error = 1; \
        if(pos.exprloc > context->errloc.exprloc) context->errloc = pos; \
        break; \
    }
#define TTYPE_LOOKAHEAD(x) \
    TTYPE(x); \
    PUSHBACK;

#define NO_SYNTAX_ERROR (context->error == 0)
#define CHECK_ERROR \
if(context->error!=0) { \
    break; \
}

#define NT(x) \
CONCAT(nextRuleGen, x)(e, context); \
CHECK_ERROR;

#define NT1(x, p) \
CONCAT(nextRuleGen, x)(e, context, p); \
CHECK_ERROR;

#define NT2(x, p, q) \
CONCAT(nextRuleGen, x)(e, context, p, q); \
CHECK_ERROR;

#define CHOICE_BEGIN(l) \
if(context->error==0) { \
    Label CONCAT(l,Start); \
    int CONCAT(l,Finish) = 0; \
    int CONCAT(l,TokenQueueP) = context->tqp; \
    getFPos(&CONCAT(l,Start), e, context); \
    context->stackTopStack[context->stackTopStackTop++] = context->nodeStackTop;

#define CHOICE_END(l) \
    (context->stackTopStackTop)--; \
    if(!CONCAT(l,Finish)) { \
        UPDATE_ERR_LOC; \
        context->error = 1; \
        break;\
    } \
}

#define BRANCH_BEGIN(l) \
if(!CONCAT(l,Finish)) { \
    do { \
    	context->tqp = CONCAT(l,TokenQueueP); \
        context->nodeStackTop = context->stackTopStack[context->stackTopStackTop-1]; \
        context->error = 0;

#define BRANCH_END(l) \
        if(context->error == 0) \
            CONCAT(l,Finish) = 1; \
    } while(0);\
}

#define TRY(l) \
CHOICE_BEGIN(l) \
BRANCH_BEGIN(l)

#define OR(l) \
BRANCH_END(l) \
BRANCH_BEGIN(l)

#define FINALLY(l) \
BRANCH_END(l) \
{ \
    do {

#define END_TRY(l) \
BRANCH_END(l) \
CHOICE_END(l)

#define ABORT(x) \
if(x) { \
        context->error = 1; \
        break;\
}

#define OPTIONAL_BEGIN(l) \
CHOICE_BEGIN(l); \
BRANCH_BEGIN(l);

#define OPTIONAL_END(l) \
BRANCH_END(l); \
BRANCH_BEGIN(l); \
BRANCH_END(l); \
CHOICE_END(l);

#define LOOP_BEGIN(l) \
int CONCAT(done, l) = 0; \
while(!CONCAT(done, l) && NO_SYNTAX_ERROR) {

#if defined(solaris_platform)
#define LOOP_END(l) \
} \
if(!CONCAT(done, l)) { \
    break; \
}

#define DONE(l) \
CONCAT(done, l) = 1;

#else
#define LOOP_END(l) \
} \
CONCAT(exit, l): \
if(!CONCAT(done, l)) { \
    break; \
}

#define DONE(l) \
CONCAT(done, l) = 1; \
goto CONCAT(exit, l);
#endif

#define COUNTER(l) \
		CONCAT(l, Counter)

#define REPEAT_BEGIN(l) \
int COUNTER(l) = 0; \
LOOP_BEGIN(l) \
	TRY(l) \

#define REPEAT_END(l) \
		COUNTER(l)++; \
	OR(l) \
		DONE(l); \
	END_TRY(l) \
LOOP_END(l)

#define LIST_BEGIN(l) \
	int COUNTER(l) = 0; \
	LOOP_BEGIN(l) \

#define LIST_DELIM(l) \
	COUNTER(l)++; \
	TRY(l) \

#define LIST_END(l) \
	OR(l) \
		DONE(l); \
	END_TRY(l) \
LOOP_END(l)

/** utility functions */
ParserContext *newParserContext(rError_t *errmsg, Region *r);
void deleteParserContext(ParserContext *t);

Token *nextTokenRuleGen(Pointer* expr, ParserContext* pc, int rulegen, int ext);
int nextString(Pointer *e, char *value, int vars[]);
int nextString2(Pointer *e, char *value, int vars[]);
int eol(char ch);
int isOp(char *token);
int isUnaryOp(Token* token);
int getUnaryPrecedence(Token* token);
int isBinaryOp(Token *token);
int getBinaryPrecedence(Token* token);
void getCoor(Pointer *p, Label * errloc, int coor[2]);
int getLineRange(Pointer *p, int line, rodsLong_t range[2]);

/**
 * skip a token of type TK_TEXT, TK_OP, or TK_MISC_OP and text text, token will has type N_ERROR if the token does not match
 */
int skip(Pointer *expr, char *text, Token **token, ParserContext *pc, int rulegen);
void skipWhitespace(Pointer *expr);
char *findLineCont(char *expr);

int parseRuleSet(Pointer *e, RuleSet *ruleSet, Env *funcDesc, int *errloc, rError_t *errmsg, Region *r);
/**
 * Parse a rule, create a rule pack.
 * If error, either ret==NULL or ret->type=N_ERROR.
 */
Node *parseRuleRuleGen(Pointer *expr, int backwardCompatible, ParserContext *pc);
Node *parseTermRuleGen(Pointer *expr, int rulegn, ParserContext *pc);
Node *parseActionsRuleGen(Pointer *expr, int rulegn, int backwardCompatible, ParserContext *pc);
void pushback(Pointer *e, Token *token, ParserContext *pc);
void initPointer(Pointer *p, FILE* fp, char* ruleBaseName);
void initPointer2(Pointer *p, char* buf);
Pointer *newPointer(FILE* buf, char *ruleBaseName);
Pointer *newPointer2(char* buf);
void deletePointer(Pointer* buf);

void skipComments(Pointer *e);
int nextChar(Pointer *p);
int lookAhead(Pointer *p, unsigned int n);

char* trim(char* str);
void trimquotes(char *string);
int isLocalVariableNode(Node *node);
int isSessionVariableNode(Node *node);
int isVariableNode(Node *node);

void ruleNameToString(char **p, int *s, int indent, Node *rn);
void ruleToString(char *buf, int size, RuleDesc *rd);
void actionsToString(char **p, int *s, int indent, Node *na, Node *nr);
void indentToString(char **p, int *s, int indent);
void termToString(char **p, int *s, int indent, int prec, Node *n, int quote);
void patternToString(char **p, int *s, int indent, int prec, Node *n);
void typeToStringParser(char **p, int *s, int indent, int lifted, Node *n);
void functionApplicationToString(char *buf, int size, char *fn, Node **args, int n);

char *nextRuleSection(char *expr, char* section);
char *parseFunctionParameters(char *e, char *args[], int *argc);
char *functionParameters(char *expr, char* param);
ExprType *parseFuncTypeFromString(char *string, Region *r);
Node* parseTypingConstraintsFromString(char *string, Region *r);
ExprType *parseType(Pointer *e, int prec, Env *vtable, int lifted, Region *r);

Label *getFPos(Label *label, Pointer *p, ParserContext *context);
void clearBuffer(Pointer *p);
void seekInFile(Pointer *p, unsigned long x);
void nextChars(Pointer *p, int len);

void syncTokenQueue(Pointer *e, ParserContext *context);

int dupLine(Pointer *p, Label * start, int n, char *buf);
int dupString(Pointer *p, Label * start, int n, char *buf);

StringList *getVarNamesInExprNode(Node *expr, Region *r);
StringList *getVarNamesInExprNodeAux(Node *expr, StringList* varnames, Region *r);
int eqExprNodeSyntactic(Node *a, Node *b);
int eqExprNodeSyntacticVarMapping(Node *a, Node *b, Hashtable *varMapping /* from a to b */);

int nextStringBase(Pointer *e, char *value, char* delim, int consumeDelim, char escape, int cntOffset, int vars[]);
int nextStringBase2(Pointer *e, char *value, char* delim);
Node *convertStringToExpression(Token *token, char *base, Node **node, Region *r);
Node *nextActionBackwardCompatible(Pointer *e, Node **node, rError_t *errmsg, Region *r);
Node *parseActionArgumentBackwardCompatible(Pointer *e, Node **node, rError_t *errmsg, Region *r);
void nextActionArgumentStringBackwardCompatible(Pointer *e, Token *token);

char* typeName_Res(Res *s);
char* typeName_ExprType(ExprType *s);
char* typeName_NodeType(NodeType s);
char* typeName_Parser(NodeType s);
void printTree(Node *n, int indent);
void printIndent(int indent);

void generateErrMsgFromFile(char *msg, long errloc, char *ruleBaseName, char* ruleBasePath, char errbuf[ERR_MSG_LEN]);
void generateErrMsgFromSource(char *msg, long errloc, char *src, char errbuf[ERR_MSG_LEN]);
void generateErrMsgFromPointer(char *msg, Label *l, Pointer *e, char errbuf[ERR_MSG_LEN]);
char *generateErrMsg(char *msg, long errloc, char* ruleBaseName, char errbuf[ERR_MSG_LEN]);
void generateAndAddErrMsg(char *msg, Node *node, int errcode, rError_t *errmsg);

#endif
