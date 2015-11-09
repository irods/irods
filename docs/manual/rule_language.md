# iRODS Rule Language

The iRODS Rule Language is a language provided by iRODS to define policies and
actions in the system. The iRODS Rule Language is tightly integrated with
other components of iRODS. Many frequently used policies and actions can be
configured easily by writing simple rules, yet the language is flexible enough
to allow complex policies or actions to be defined.

Everything is a rule in the iRODS Rule Language. A typical rule looks like:

~~~
acPostProcForPut { 
  on($objPath like "*.txt") { 
    msiDataObjCopy($objPath,"$objPath.copy"); 
  } 
}
~~~

In this rule, the rule name `acPostProcForPut` is an
event hook defined in iRODS. iRODS automatically applies this rule when
certain events are triggered. The `on(...)` clause is a rule condition. The
`{...}` block following the rule condition is a sequence of actions that is
executed if the rule condition is true when the rule is applied. And the
customary hello world rule looks like: 

~~~
HelloWorld { 
  writeLine("stdout", "Hello, world!"); 
}
~~~

In the following sections, we go over some features of the rule engine.

## Comments

The rule engine parses characters between the `#` token and the end of
line as comments. Therefore, a comment does not have to occupy its own line.
For example,

~~~c
* A=1; # comments 
~~~

Although the parser is able to parse comments starting with `##`, it is not recommended to start comments with `##`, as `##` is also used in the one line rule syntax as the actions connector. It is recommended to start comments with `#`.

## Directives

Directives are used to provide the rule engine with compile time intructions.
The `@include` directive allows including a different rule base file into the
current rule base file, similar to `#include` in C. For example, if we have a
rule base file "definitions.re", then we can include it with the following
directive `@include "definitions"`

## Boolean and Numeric

### Boolean Literals

Boolean literals include `true` and `false`.

### Boolean Operators

Boolean operators include 

~~~c
!  # not
&& # and
|| # or
%% # or used in the ## syntax
~~~

For example:

~~~c
true && true
false && true
true || false
false || false
true %% false
false %% false
! true
~~~

### Numeric Literals

Numeric literals include integral literals and double literals. An integral
literal does not have a decimal while a double literal does. For example, 

~~~c
1 # integer 
1.0 # double
~~~

In the iRODS Rule Language, an integer can be converted to a double. The
reverse is not always true. A double can be converted to an integer only if
the fractional part is zero. The rule engine provides two
functions that can be used to truncate the fractional part of a double:
`floor()` and `ceiling()`.

Integers and doubles can be converted to booleans using the `bool()` function.
`bool()` converts `1` to `true` and `0` to `false`.

### Arithmetic Operators

Arithmetic operators include, ordered by precedence:

~~~c
-  # Negation
^  # Power
*  # Multiplication 
/  # Division 
%  # Modulo
-  # Subtraction 
+  # Addition
>  # Greater than
<  # Less than
>= # Greater than or equal
<= # Less than or equal
~~~

### Arithmetic Functions

Arithmetic functions include:

~~~c
exp(<num>)
log(<num>)
abs(<num>)
floor(<num>)
ceiling(<num>)
average(<num>,<num>,...)
max(<num>,<num>,...)
min(<num>,<num>,...)
~~~

For example:
~~~c
exp(10)
log(10)
abs(-10)
floor(1.2)
ceiling(1.2)
average(1,2,3)
max(1,2,3)
min(1,2,3)
~~~

## Strings

### String Literals

One of the main changes to the rule language is quotes and escapes in strings.
The rule engine requires by default that every string literal is quoted.
The quotes can be either matching single quotes `'This is a string.'` or double
quotes `"This is a string."`

If a programmer needs to quote strings containing single (double) quotes using single (double) quotes, then the quotes in the
strings should be escaped using a backslash `"\"`, just as in the C Programming
Language. For example, 

~~~c
writeLine("stdout", "\"\"");
# output "" 
~~~

Single quotes inside double quotes are viewed as regular characters, and vice versa. They can be either escaped or not escaped. For example, 

~~~c
writeLine("stdout", "'");
# output ' 
~~~

~~~c
writeLine("stdout", "\'");
# output ' 
~~~

The rule engine also supports various escaped characters: 
~~~c
\n, \r, \t, \', \", \$, \*
~~~

An asterisk should always be escaped if it is a regular
character and is followed by letters.

### Converting Values of Other Types to Strings

The `str()` function converts a value of type BOOLEAN, INTEGER, DOUBLE,
DATETIME, or STRING to string. For example

~~~c
writeLine("stdout", str(123));
# output 123
~~~

In addition

~~~c
timestrf(*time, *format)
~~~

converts a datetime stored in `*time` to a string, according to the `*format`
parameter.

~~~c
timestr(*time)
~~~

converts a datetime stored in `*time` to a string, according to the default
format of `%b %d %Y %H:%M:%S`.  An example would be `Jun 01 2015 16:12:13`.

The format string uses the same directives as [the standard C library](http://en.cppreference.com/w/c/chrono/strftime).

### Converting Strings to Values of Other Types

String can be converted to values of type BOOLEAN, INTEGER, DOUBLE, DATETIME,
or STRING. For example

~~~c
int("123")
double("123")
bool("true")
~~~

In addition

~~~c
datetimef(*str, *format)
~~~

converts a string stored in `*str` to a datetime, according to the `*format` parameter

~~~c
datetime(*str)
~~~

converts a string stored in `*str` to a datetime, according to the default
format (`%b %d %Y %H:%M:%S`, e.g. `Jun 01 2015 16:12:13`). It can also be used to convert an integer or a double to a datetime.

The following are examples of string datetime conversion

~~~c
datetime(*str)
datetimef(*str, "%Y %m %d %H:%M:%S")
timestr(*time)
timestrf(*time, "%Y %m %d %H:%M:%S")
~~~

### String Functions

The rule engine supports the infix string concatenation operator `"++"`

~~~c
writeLine("stdout", "This "++" is "++" a string.");
# output This is a string.
~~~

**Infix wildcard expression matching operator**: `like`

~~~c
writeLine("stdout", "This is a string." like "This is*");
# Output: true
~~~

**Infix regular expression matching operator**: `like regex`

~~~c
writeLine("stdout", "This is a string." like regex "This.*string[.]");
# Output: true
~~~

**Substring**: `substr()`

~~~c
writeLine("stdout", substr("This is a string.", 5, 9));
# or
writeLine("stdout", substr("This is a string.", 5, 5+4));
# Output: is a
~~~

**Length**: `strlen()`

~~~c
writeLine("stdout", strlen("This is a string."));
# Output: 17
~~~

**Split**: `split()`

~~~c
writeLine("stdout", split("This is a string.", " "));
# Output: [This,is,a,string.]
~~~

**Trim left**: `triml(*str, *del)`, which trims from `*str` the leftmost `*del`, inclusive.

~~~c
writeLine("stdout", triml("This is a string.", "i"));
# Output: s is a string.
~~~

**Trim right**: `trimr(*str, *del)`, which trims from `*str` the rightmost `*del`, inclusive.

~~~c
writeLine("stdout", trimr("This is a string.", "r"));
# Output: This is a st
~~~

### Variable Expansion

In a quoted string, an asterisk followed immediately by a variable name
(without whitespaces) makes an expansion of the variable. For example, 

~~~c
"This is *x."
~~~

is equivalent to 

~~~c
"This is "++str(*x)++"."
~~~

### Rules for Quoting Action Arguments

A parameter to a microservice is of type string if the expected type is
`MS_STR_T` or `STRING`. When a microservice expects a parameter of type string and
the argument is a string constant, the argument has to be quoted. For example,

~~~c
writeLine("stdout", "This is a string.");
~~~

When a microservice expects a parameter of type string and the argument is not of type string, a type error
may be thrown. For example,

~~~c
*x = 123; 
strlen(*x); 
~~~

This error can be fixed by either using the "str" function 

~~~c
strlen(str(*x));
~~~

or putting `*x` into quotes 

~~~c
strlen("*x");
~~~

Action names and keywords such as `for`, `while`, `assign` are not arguments. Therefore, they do not have to be quoted.

### Wildcard and Regular Expression

The rule engine supports both the wildcard matching operator `like` and a
regular expression matching operator `like regex`. (It is an operator, not
two separate keywords.)

The rule engine supports the `*` wildcard. For example

~~~c
"abcd" like "ab*"
~~~

In case of ambiguity with variable expansion, the `*` has to be escaped. For example

~~~c
"abcd" like "a\\*d"
~~~

because `"a*d"` is interpreted as `"a"++str(*d)+""`

When wildcard is not expressive enough, regular expression matching operator
can be used. For example

~~~c
"abcd" like regex "a.c."
~~~

A regular expression matches the whole string. It follows the syntax of the
POSIX API.

### Quoting Code

Sometime when you want to pass a string representation of code or regular
expressions into an action, it is very tedious to escape every special
character in the string. For example

~~~c
writeLine(\"stdout\", \*A)
~~~

or

~~~c
*A like regex "a\*c\\\\\\[\\]" # matches the regular expression a*c\\\[\]
~~~

For ease of reading, you can use two backticks ("\`\`") instead of the regular quotes. The rule
engine does not further look for variables, etc. in strings between two "\`\`"s.
The examples can now be written as:

~~~c
``writeLine("stdout", *A)``
~~~
and
~~~c
*A like regex ``a*c\\\[\]``
~~~

## Dot Expression

The dot oeprator provides a simple syntax for creating and accessing key
values pair.

To write to a key value pair, use the dot operator on the left hand side:

~~~c
*A.key = "val"
~~~

If the key is not a syntactically valid identifier, quotes can be used, escape
rules for strings also apply:

~~~c
*A."not an identifier" = "val"
~~~

If the variable `*A` is undefined, a new key value pair data structure will be
created.

To read from a key value pair, use the dot operator as binary infix operation
in any expression.

Currently key value pairs only support the string type for values.

The str() function is extended to support converting a key value pair data
structure to an options format:

~~~c
*A.a=A;
*A.b=B;
*A.c=C; 
str(*A); # a=A++++b=B++++c=C
~~~

## Constant

A constant can be defined as a function that returns a constant. A constant
defintion has the following syntax:

~~~c
<constant name> = <constant value>
~~~

where the constant value can be on of the following:

* an integer
* a double
* a string (with no variable expansion in it)
* a boolean

A constant name can be used in a pattern and is replaced by its value (whereas
a nonconstant is treated as a constructor). For example,

With

~~~c
CONSTANT = 1
~~~

the following expression

~~~c
match CONSTANT with
   CONSTANT => "CONSTANT"
   *_ => "NOT CONSTANT"
~~~

returns `CONSTANT`. With a nonconstant function definition such as

~~~c
CONSTANT = time()
~~~

it returns `NOT CONSTANT`.

## Variable

The rule engine expands variables on demand, like in C. For example, suppose we have
the following rule 

~~~c
ifExec(*A==1,assign(*A,0),assign(*A,1),nop,nop) 
~~~

Expanding `*A` (say, to 1) before the rule is executed would result in something like

~~~c
ifExec(1==1,assign(1,0),assign(1,1),nop,nop) 
~~~

As a result, extra code has to be written for system microservices to avoid this. The value of `*A` is retrieved from a runtime environment only when the rule engine tries to evaluate it.

## Function

### Function Definition

The rule engine allows defining functions. Functions can be thought of as
microservices written in the rule language. The syntax of a function
definition is 

~~~c
<name>(<param>, ..., <param>) = <expr> 
~~~

For example

~~~c
square(*n) = *n * *n 
~~~

Function names should be unique (no function-function or function-rule
name conflict). Functions can be defined in a mutually exclusive manner. For
example

~~~c
odd(*n) = if *n==0 then false else even(*n-1)
even(*n) = if *n==1 then true else odd(*n-1)
~~~

Here we cannot use `&&` or `||` because they do not short
circuit like in C or Java.

### Calling a Function

To use a function, call it as if it was a microservice.

## Rule

### Rule Definition

The syntax of a rule with a nontrivial rule condition is as follows:

~~~c
<name>(<param>, ..., <param>) { 
  on(<expr>) { <actions> } 
}
~~~

If the rule condition is trivial or unnecessary, the rule can be written in
the simpler form: 

~~~c
<name>(<param>, ..., <param>) { <actions> }
~~~

Multiple rules with the same rule name and parameters list can be combined in
a more concise syntax where each set of actions is enumerated for each set of
conditions: 

~~~c
<name>(<param>, ..., <param>) { 
  on(<expr>) { <actions> } ...
  on(<expr>) { <actions> } 
}
~~~

### Function Name and Rule Name

Function and rule names have to be valid identifiers. Identifiers start with
letters followed by letters or digits. For example,

~~~c
ThisIsAValidFunctionNameOrRuleName
~~~

There should not be whitespaces in a
function name or a rule name. For example

~~~c
This Is Not A Valid Function Name or Rule Name
~~~

### Rule Condition

Rule conditions should be expressions of type `boolean`. The rule is executed only when the rule condition evaluates to true. Which means that there are three failure conditions:

  1. Rule condition evaluates to false.
  2. Some actions in rule condition fails which causes the evaluation of the whole rule condition to fail.
  3. Rule condition evaluates to a value whose type is not boolean.

For example, if we want to run a rule when the microservice "msi" succeeds, we
can write the rule as

~~~c
rule { 
  on (msi >= 0) { ... } 
} 
~~~

Conversely, if we want to run a rule when the
microservice fails, we need to write the rule as 

~~~c
rule { 
  on (errorcode(msi) < 0) { ... } 
}
~~~

The errormsg microservice captures the error message, allows
further processing of the error message, and avoiding the default logging of
the error message

~~~c
rule { 
  on (errormsg(msi, *msg) < 0 ) { ... } 
} 
~~~

By failure condition 3, the following rule condition always fails because msi returns an integer value 

~~~c
on(msi) { ... }
~~~

### Generating and Capturing Errors

In a rule, we can also prevent the rule from failing when a microservice fails

~~~c
errorcode(msi)
~~~

The errormsg microservice captures the error message, allows
further processing of the error message, and avoiding the default logging of
the error message

~~~c
errormsg(msi, *msg)
~~~

In a rule, the fail and failmsg microservices can be used to generate errors 

~~~c
fail(*errorcode)
~~~

generates an error with an error code

~~~c
failmsg(*errorcode, *errormsg)
~~~

generates an error with an error code and an error message. For example

~~~c
fail(-1)
failmsg(-1, "this is a user generated error message")
~~~

The msiExit microservice is similar to failmsg

~~~c
msiExit("-1", "msi")
~~~

### Calling a Rule

To use a rule, call it as if it was a microservice.

## Data Types and Pattern Matching

### System Data Types

#### Lists

A list can be created using the `list()` microservice. For example

~~~c
list("This","is","a","list")
~~~

The elements of a list should have the same type. Elements of a list can be
retrieved using the "elem" microservice. The index starts from 0. For example,

~~~c
elem(list("This","is","a","list"),1)
~~~

evaluates to `"is"`. 

If the index is out of range it fails with error code -1. 

The `setelem()` takes in three parameters, a list, an index, and a value, 
and returns a new list that is identical with the list given by the first 
parameter except that the element at the index given by the second parameter 
is replace by the value given by the third parameter.

~~~c
setelem(list("This","is","a","list"),1,"isn't")
~~~

evaluates to

~~~c
list("This","isn't","a","list").
~~~

If the index is out of range it fails with an error code. The `size` 
microservice takes in on parameter, a list, and returns the size of the list. 

For example

~~~c
size(list("This","is","a","list"))
~~~

evaluates to `4`.

The `hd()` microservice returns the first element of a list and
the `tl()` microservice returns the rest of the list. 

If the list is empty then it fails with an error code. 

~~~c
hd(list("This","is","a","list"))
~~~

evaluates to "This" and 

~~~c
tl(list("This","is","a","list"))
~~~

evaluates to 

~~~c
list("is","a","list")
~~~

The `cons()` microservice returns a list by combining an
element with another list. For example

~~~c
cons("This",list("is","a","list"))
~~~

evaluates to

~~~c
list("This","is","a","list").
~~~

#### Tuples

The rule engine supports the built-in data type tuple. 

~~~c
( <component>, ..., <component> ) 
~~~

Different components may have different types.

#### Interactions with Packing Instructions

Complex lists such as lists of lists can be constructed locally, but mapping
from complex list structures to packing instructions are not yet supported.
The supported lists types that can be packed are integer lists and string
lists. When remote execute or delay execution is called while there is a
complex list in the current runtime environment, an error will be generated.
For example, in the following rule

~~~c
test {
   *A = list(list(1,2),list(3,4));
   *B = elem(*A, 1);
   delay("<PLUSET>1m</PLUSET>") {
       writeLine("stdout", *B);
   }
}
~~~

Even though `*A` is not used in the delay execution block, the rule will 
still generate an error. One solution to this is to create a rule with only necessary values. 

~~~c
test {
   *A = list(list(1,2),list(3,4));
   *B = elem(*A, 1);
   test(*B);
}
test(*B) {
   delay("<PLUSET>1m</PLUSET>") {
       writeLine("stdout", *B);
   }
}
~~~

### Inductive Data Type

The rule engine allows defining inductive data types. An inductive data
type is a data type for values that can be defined inductively, i.e. more
complex values can be constructed from simpler values using constructors. The
general syntax for inductive data type definition is

~~~c
data <name> [ ( <type parameter list> ) ] =
    |  : <data constructor type>
    ...
    | <data constructor name> : <data constructor type>
~~~

For example, a data type that represents the natural numbers can be defined as

~~~c
data nat =
    | zero : nat
    | succ : nat -> nat
~~~
  
Here the type name defined is "nat." The type parameter list is empty. If the type parameter list is empty, we may omit it. There are two data constructors. The first constructor "zero" has type "nat," which means that "zero" is a nullary constructor of nat. We use "zero" to represent "0". The second constructor "succ" has type "nat -> nat" which means that "succ" is unary constructor of nat. We use "succ" to represent the successor. With these two constructors we can represent all natural numbers: `zero, succ(zero), succ(succ(zero)), ...` As another example, we can define a data type that represents binary trees of natural numbers

~~~c
data tree =
    | empty : tree
    | node : nat * tree * tree -> tree
~~~

The `empty` constructor constructs an empty tree, and the `node` constructor
takes in a `nat` value, a left subtree, and a right subtree and constructs a
tree whose root node value is the "nat" value. The next example shows how to
define a polymorphic data type. Suppose that we want to generalize our binary
tree data type to those trees whose value type is not "nat." We give the type
tree a type parameter `X`

~~~c
data tree(X) =
    | empty : tree(X)
    | node : X * tree(X) * tree(X) -> tree(X)
~~~
  
With a type parameter, `tree` is not a type, but a unary type constructor. A
type constructor constructs types from other types. For example, the data type
of binary trees of natural numbers is `tree(nat)`. By default, the rule engine
parses all types with that starts with uppercase letter as type variables.

Just as data constructors, type constructor can also take multiple parameters.
For example

~~~c
data pair(X, Y) =
    | pair : X * Y -> pair(X, Y)
~~~
  
Given the data type definition of "pair", we can construct a pair using "pair"
the data constructor. For example,

~~~c
*A = pair(1, 2);
~~~

### Pattern Matching

#### Pattern Matching In Assignment

Patterns are similar to expressions. For example

~~~c
pair(*X, *Y)
~~~

There are a few restrictions. First, only data constructors and free variables may appear in
patterns. Second, each variable only occurs once in a pattern (sometimes
called linearity). To retrieve the components of `*A`, we can use patterns on
the left hand side of an assignment. For example

~~~c
pair(*X, *Y) = *A;
~~~

When this action is executed, `*X` will be assigned to 1 and `*Y` will be assigned to `2`.
Patterns can be combined with let expressions. For example

~~~c
fib(*n) = if *n==0 then pair(-1, 0)
    else if *n==1 then (0, 1)
    else let pair(*a, *b) = fib(*n - 1) in
        pair(*b, *a + *b)
~~~

#### Pseudo Data Constructors

For other types, the rule engine allows the programmer to define pseudo
data constructors on them for pattern matching purposes. Pseudo data
constructors are like data constructors but can only be used in patterns.
Pseudo data constructor definitions are like function definitions except that
a pseudo data constructor definition starts with a tilde and must return a
tuple. The general syntax is 

~~~c
<name>(<param>, ..., <param>) = <expr>
~~~

A pseudo data constructor can be thought of an inverse function that maps the values in
the codomain to values in the domain. For example, we can define the following
pseudo data constructor

~~~c
lowerdigits(*n) = let *a = *n % 10 in ((*n - *a) / 10 % 10, *a)
~~~

The assignment

~~~c
lowerdigits(*a, *b) = 256;
~~~

results in `*a` assigned `5` and `*b` assigned `6`.

## Control Structures

### Actions

The iRODS rule engine has a unique concept of recovery action. Every action in
a rule definition may have a recovery action. The recovery action is executed
when the action fails. This allows iRODS rules to rollback some side effects
and restore most of the system state to a previous point. An action recovery
block has the form

~~~c
{
   A1 ::: R1
   A2 ::: R2
   ...
   An ::: Rn
}
~~~

The basic semantics is that if `Ax` fails then `Rx, R(x-1), ... R1` will be executed. The programmer
can use this mechanism to restore the system state to the point before this action recovery block is executed.

The rule engine make the distinction between expressions and actions. An
expression does not have a recovery action. An action always has a recovery
action. If a recovery action is not specified for an action, the rule engine
use `nop` as the default recovery action.

Examples of expressions include the rule condition, and the conditional
expressions in the `if`, `while`, and `for` actions.

There is no intrinsic difference between an action and an expression. An
expression becomes an action when it occurs at an action position in an action
recovery block. An action recovery block, in turn, is an expression.

The principle is that an expression should only be used not as an action if it
is side-effect free. In the current version, this property is not checked by
the rule engine. The programmer has to make sure that it holds for the rule
base.

From this perspective, the only difference between functions and rules is
nondeterminism.

### if

The rule engine has a few useful extensions to the `if` keyword that makes
programming in the rule language more convenient.

In addition to the traditional way of using `if`, which
will be referred to as the `logical if` where you use if as an action which
either succeeds or fails with an error code, the rule engine supports
an additional way of using if, which will be referred to as the `functional if`. The
`functional if` may return a value of any type if it succeeds. The two
different usages have different syntax. The `logical if` has the expected syntax of

~~~c
if <expr> then { <actions> } else { <actions> }
~~~

while the "functional if" has the following syntax

~~~c
if <expr> then <expr> else <expr>
~~~

For example, the following are "functional if"s

~~~c
if true then 1 else 0 
if *A==1 then true else false 
~~~

To compare, if written in the "logical if" form, the
second example would be

~~~c
if (*A==1) then { true; } else { false; }
~~~

To make the syntax of "logical if" more concise, the rule engine allows
the following abbreviation (where the greyed out part can be abbreviated): 

~~~c
if (...) then { ... } else { ... } 
if (...) then { ... } else { if (...) then {...} else {...} }
~~~

Multiple abbreviations can be combined for
example:

~~~c
if (*X==1) { *A = "Mon"; } 
else if (*X==2) {*A = "Tue"; } 
else if (*X==3) {*A = "Wed"; } 
...
~~~

### foreach

The rule engine allows defining a different variable name for the iterator
variables in the foreach action. For example

~~~c
foreach(*E in *C) {
  writeLine("stdout", *E); 
} 
~~~

This is equivalent to

~~~c
foreach(*C) {
  writeLine("stdout", *C); 
}
~~~

This feature allows the collection to be a complex expression. For
example

~~~c
foreach(*E in list("This", "is", "a", "list")) { 
  writeLine("stdout", *E); 
} 
~~~

This is equivalent to

~~~c
*C = list("This", "is", "a", "list");
foreach(*C) {
   writeLine("stdout", *C);
}
~~~

### The `let` Expression

As function definitions are based on expressions rather than action sequences,
we cannot put an assignment directly inside an expression. For example, the
following is not a valid function definition 

~~~c
quad(*n) = *t = *n * *n; *t * *t
~~~

To solve this problem, the let expression provides scoped values in an
expression. The general syntax for the let expression is 

~~~c
let <assignment> in <expr> 
~~~

For example

~~~c
quad(*n) = let *t = *n * *n in *t * *t 
~~~

The variable on the
left hand side of the assignment in the let expression is a let-bound
variable. The scope of such a variable is within the let expression. A let
bound variable should not be reassigned inside the let expression.

### The `match` Expression

If a data type has more than one data structure, then the "match" expression
is useful

~~~c
match <expr> with
   | <pattern> => <expr>
   ...
   | <pattern> => <expr>
~~~
  
For example, given the `nat` data type we defined earlier, we can define the
following function using the `match` expression

~~~c
add(*x, *y) =
   match *x with
       | zero => *y
       | succ(*z) => succ(add(*z, *y))
~~~

For another example, given the "tree" data type we defined earlier, we can
define the following function

~~~c
size(*t) =
   match *t with
       | empty => 0
       | node(*v, *l, *r) => 1 + size(*l) + size(*r)
~~~

## Recovery Chain For Control Structures

### Sequence

Actions:

~~~c
A1##A2##...##An
~~~

Recovery:

~~~c
R1##R2##...##Rn
~~~

Rulegen syntax: 

~~~c
A1:::R1
A2:::R2
...
An:::Rn
~~~

If Ax fails, then `Rx, ..., R1` are executed

### Branch

Action: 

~~~c
if(cond, A11##A12##...##A1n, A21##A22##...##A2n,
         R11##R12##...##R1n, R21##R22##...##R2n)
~~~

Recovery:

~~~c
R
~~~

Rulegen syntax:

~~~c
if(cond) then {
    A11:::R11
    A12:::R12
    ...
    A1n:::R1n
} else {
    A21:::R21
    A22:::R22
    ...
    A2n:::R2n
}:::R
~~~

If Axy fails, then `Rxy, ..., Rx1, R` are executed. If `cond` fails, then `R` is
executed.

### Loop

#### `while`

Action: 

~~~c
while(cond, A1##A2##...##An
            R1##R2##...##Rn)
~~~

Recovery:

~~~c
R
~~~

Rulegen syntax: 

~~~
while(cond) {
    A1:::R1
    A2:::R2
    ...
    An:::Rn
}:::R
~~~

If `Ax` fails, then `Rx, ..., R1, R` are executed. If `cond` fails, then `R` is
executed. Here `R` should deal with the loop invariant. The recovery chain in
the loop restores the loop invariant, and the `R` restores the machine status
from the loop invariant to before the loop is executed.

#### `foreach`

Action: 

~~~c
foreach(coll, A1##A2##...##An 
              R1##R2##...##Rn) 
~~~

Recovery: 

~~~c
R
~~~

Rulegen syntax: 

~~~c
foreach(coll) {
    A1:::R1
    A2:::R2
    ...
    An:::Rn
}:::R
~~~

If `Ax` fails, then `Rx, ..., R1, R` are executed.

#### "for"

Action:

~~~c
for(init, cond, incr, A1##A2##...##An
                      R1##R2##...##Rn)
~~~

Recovery:

~~~c
R
~~~

Rulegen syntax:

~~~c
for(init; cond; incr) {
    A1:::R1
    A2:::R2
    ...
    An:::Rn
}:::R
~~~

If `Ax` fails, then `Rx, ..., R1, R` are executed. If `init`, `cond`, or `incr` fails, then `R` is executed.

## Types

### Introduction

Types are useful for capturing errors before rules are executed. At the same
time, a restrictive type system may also rule out meaningful expressions. As
the rule language is a highly dynamic language, the main goal of introducing a
type system is the following:

* To enable discovering some errors statically.
* To help remove some repetitive type checking and conversion code in microservices by viewing types as contracts of what kinds of values are passed between the rule engine and microservices.

The type system is designed so that the rule language is dynamically typed
when no type information is given, while providing certain static guarantees
when some type information is given. The key is combining static typing with
dynamic typing, so that we only need to check the statically typed part of a
program statically and leave the rest of the program to dynamic typing. The
idea is based on Coercion, Soft Typing, and Gradual Typing.

The rule engine uses the central idea of coercion insertion. The rule engine
has a fixed coercion relation among
its primitive types. Any coercion has to be based on this coercion relation.
The coercion relation does not have to be fixed but has to satisfy certain
properties. This way, we can potentially adjust the coercion relation without
breaking other parts of the type system.

The rule engine distinguishes between two groups of microservices. System
provided microservices such as string operators are called internal micro
services. The rest are called external microservices. Most internal micro
services are statically typed. They come with type information which the type
check can make use of to check for errors statically. Currently, all external
microservices are dynamically typed.

The rule engine supports a simple form of polymorphism, based on the
Hindley-Milner (HM) polymorphism. The idea of HM is that any function type can
be polymorphic, but all type variables are universally quantified at the top
level. As the rule language does not have higher-order functions, many
simplifications can be made. To support certain internal microservices, the
type system allows type variables to be bounded, but only to a set of base
types.

The type system implemented in the rule engine is an extension to the
existing iRODS types by viewing the existing iRODS types as opaque types.

### Types

The function parameter and return types can be

~~~c
<btype> ::= boolean
          | integer
          | double
          | string
          | time
          | path
~~~

~~~c
<stype> ::= <tvar>                   identifiers starting with uppercase letters
          | iRODS types              back quoted string
          | <btype>
          | ?                        dynamic type
          | <stype> * … * <stype>    tuple types 
          | c[(<stype>, …, <stype>)] inductive data types
~~~

A function type is

~~~c
  <ftype> ::= <quanti>, …, <quanti>, <ptype> * <ptype> * … * <ptype> [*|+|?] -> <stype>
~~~

where

* the `<stype>` on the right is the return type
* the optional `*`, `+`, or `?` indicates varargs
* the `<ptype>` are parameter types of the form

~~~c
  <ptype> ::= [(input|output)*|dynamic|actions|expression] [f] <stype>
~~~

where the optional

* `input` indicates that this is an io parameter
* `output` indicates that this is an output parameter
* `dynamic` indicates that this is an io parameter or output parameter determined dynamically
* `actions` indicates that this is a sequence of actions
* `expression` indicates that this is an expression
* `f` indicates that a coercion can be inserted at compile time based on the coercion relation

the `<quanti>` are quantifiers of the form

~~~c
  <quanti> ::= forall <tvar> [in {<btype> … <btype>}]
~~~

where

* `<tvar>` is a type variable
* the optional set of types provides a bound for the type variable.

### Typing Constraint

Type constraints are used in the rule engine to encode typing requirements
that need to be checked at compile time or at runtime. The type constraints
are solved against a type coercion relation, a model of whether one type can
be coerced to another type and how their values should be converted. The type
coercion relation can be considered as a directed graph, with types as its
vertices, and type coercion functions as its edges.

The current coercion relation, denoted by `->` below, consists of the
following rules.

~~~c
SCIntegerDouble: integer -> double
SCDynamicLeft: dynamic -> y
SCDynamicRight: x -> dynamic
~~~

All coercion functions in the coercion relation are total.

### Types by Examples

For example binary arithmetic operators such as addition and subtraction are
given type: 

~~~c
forall X in {integer double}, f X * f X -> X
~~~

This indicates that the operator takes in two parameters of the same type and returns a value of
the same type as its parameters. The parameter type is bounded by {integer
double}, which means that the microservice applies to only integers or
doubles, but the "f" indicates that if anything can be coerced to these types,
they can also be accepted with a runtime conversion inserted. Examples:

(a) double + double => X = double 

~~~c
1.0+1.0 
~~~

(b) int + double => X = double 

~~~c
1+1.0
~~~

(c) integer + integer => X = {integer double} 

~~~c
1+1
~~~

(d) unknown + double => X = double
Assuming that *A is a fresh variable

~~~c
*A+1.0
~~~

The type checker generate a constraint that the type of `*A` can be coerced to double.

(e) unknown + unknown => X = {integer double}

Assuming that `*A` and `*B` are fresh variables

~~~c
*A+*B
~~~

The type checker generate a constraint that the type of `*A` can be coerced to either integer or double.

Some typing constraints can be solved within certain context. For example, if
we put (e) in to the following context

~~~c
*B = 1.0;
*B = *A + *B;
~~~

then we can eliminate the possibility that `*B` is an integer, thereby narrowing the type variable `X` to double.

Some typing constraints can be proved unsolvable. For example,

~~~c
*B = *A + *B;
*B == "";
~~~

by the second action we know that `*B` has to have type string. In this case the rule engine reports a type error.

However, if some typing constraints are not solvable, they are left to be
solved at runtime.

### Variable Typing

As in C, all variables in the rule language have a fixed type that can not be updated through an assignment.
For example, the following does not work:

~~~c
testTyping1 {
    *A = 1;
    *A = "str";
}
~~~

Once a variable `*A` is assigned a value `X` the type of the variable is given by a typing constraint

~~~c
type of X can be coerced to type of *A
~~~

For brevity, we sometimes denote the "can be coerced to" relation by `<=`. For example,

~~~c
type of X <= type of *A
~~~

The reason why the type of `*A` is not directly assigned to the type of `*X` is to allow the following usage

~~~c
testTyping2 {
    *A = 1; # integer <= type of *A
    *A = 2.0; # double <= type of *A
}
~~~

Otherwise, the programmer would have to write

~~~c
testTyping3 {
    *A = 1.0;
    *A = 2.0;
}
~~~

to make the rule pass the type checker.

As a more complex example, the following generates a type error:

~~~c
testTyping4 {
    *A = 1; # integer <= type of *A
    if(*A == "str") { # type error
    }
}
~~~

If the value of a variable is dynamically typed, then a coercion is inserted. The following example works, with a runtime coercion:

~~~c
testTyping2 {
   msi(*A);
   if(*A == "str") { # insert coercion type of *A <= string
   }
}
~~~

### Type Declaration

In the rule engine, you can declare the type of a rule function or a
microservice. If the type of an action is declared, then the rule engine will do
more static type checking. For example, although

~~~c
concat(*a, *b) = *a ++ *b
add(*a, *b) = concat(*a, *b)
~~~

does not generate a static type error, `add(0, 1)` will generate a dynamic 
type error. This can be solved (generate static type errors instead of dynamic 
type errors) by declaring the types of the functions

~~~c
concat : string * string -> string
concat(*a, *b) = *a ++ *b
add : integer * integer -> integer
add(*a, *b) = concat(*a, *b)
~~~

## Microservices

### Automatic Evaluation of Arguments

The rule engine automatically evaluates expressions within arguments of 
actions, which is useful when a program needs to pass the result of an 
expression in as an argument to an action. For example, to pass the result of an expression "1+2" as an argument to 
microservice `msi`, the programmer can write:

~~~c
msi(1+2);
~~~

and the rule engine will evaluate the expression

~~~c
1+2
~~~

and pass the result in.

### The Return Value of User Defined Microservices

The rule engine views the return value of user 
defined microservices as an integer "errorcode." If the return value of a 
microservice is less than zero, the rule engine interprets it as a failure, 
rather than an integer value; and if the return value is greater than zero, the rule engine
interprets it as an integer. Therefore the following expression

~~~c
msi >= 0
~~~

either evaluates to true or fail, since when `msi` returns a negative 
integer, the rule engine interprets the value as a failure.
In some applications, there is need for capturing all possible return values as 
regular integers. The "errorcode" microservice can be used to achieve this.
In the previous example, we can modify the code to

~~~c
errorcode(msi) >= 0
~~~

This expression will not fail on negative return values from msi.

## Improvements to the Rule Engine

### Error Messages

The rule engine generates error messages that provide information on
parser errors, type errors, and runtime errors. Most of the error messages
contain error locations in the source code, whether dynamically generated or
given in the core.re file or an irule input file.

There error messages can be found in the server log.

### Indexing

To improve the performance of rule execution, the rule engine provides two 
level indexing on applicable rules. The first level of indexing is based on the 
rule name. The second level of indexing is based on rule conditions. The rule 
condition indexing can be demonstrate by the following example:

~~~c
testRule(*A) {
    on (*A == "a") { ... }
    on (*A == "b") { ... }
}
~~~

In this example, we have two rules with the same rule name, but different rule 
conditions. The first level of indexing does not improve the performance in a 
rule application like

~~~c
testRule("a")
~~~

However, the second level indexing does. The second level indexing works on rules with similar rule conditions. In particular, the rule conditions have to be of the form

~~~c
<expression> == <string>
~~~

All rules have to have the same number of parameters, but they may have different parameter names. The expression has to be the same for all rules modulo variable renaming, and the strings have to be different for different rules.

The rule engine indexes the rules by the string. When a rule is called, the rule engine evaluates the expression once and looks up the rule using the second level indexing.

## Improvements to irule

### Multiple Rules in irule Input

The rule engine supports multiple rules in the .r file when it is used as
input to the irule command. The first rule will be evaluated and all the rules
included in the .r will be available during the execution of the first rule
before the irule command returns. Only the first rule will be available for
delayed execution or remote execution.


## Language Integrated General Query

Language Integrated General Query (LIGQ) provides native syntax support for GenQueries in the rule language and integrates automatic management of thr continuation index and the input and output data structures into the foreach loop. A query expression starts with the key word SELECT and looks exactly the same as a normal GenQuery:

~~~c
SELECT META_DATA_ATTR_NAME WHERE DATA_NAME = 'data_name'
~~~

At runtime this query is evaluated to an object of type genQueryInp_t * genQueryOut_t. This object can be assigned to a variable:

~~~c
*A = SELECT META_DATA_ATTR_NAME WHERE DATA_NAME = 'data_name';
~~~

and iterated in a foreach loop:

~~~c
foreach(*Row in *A) {
    ...
}
~~~

Or we can skip the assignment:

~~~c
foreach(*Row in SELECT META_DATA_ATTR_NAME WHERE DATA_NAME = 'data_name' AND COLL_NAME = 'coll_name') {
    ...
}
~~~

where *Row is a keyValPair_t object that contains the current row in the result set. The break statement can be used to exit the foreach loop.

LIGQ supports the following gen query syntax:

* `count`, `sum`, `order_desc`, `order_asc`
* `=`, `<>`, `>`, `<`, `>=`, `<=`, `in`, `between`, `like`, `not like`
* `||` and `&&`

LIGQ also provides support for using `==` and `!=` as equality predicates, in order to be consistent with the rule engine syntax.

The left hand side of comparison operators is always a column name, but the right hand side of a comparison operator is always one (or more in the case of `between`) normal Rule Engine expression(s) which is(are) evaluated by the rule engine first. Therefore, we can use any rule engine expression on the right hand side of a comparison operator. If the right hand side operand is not a simple single quoted string or number, then the LIGQ query cannot be executed from the iquery command.

One potential confusion is that the `like` and `not like` operators in the gen query syntax differ from those in the Rule Language. The right hand side operand is first evaluated by the rule engine to a string which in turn is interpreted by the qen query subsystem. Therefore, LIGQ queries do not use the same syntax for wildcards as the rule engine (unless the wildcards are in a nested rule engine expression). While the rule engine uses `*` for wildcards, qen query uses the standard SQL syntax for wildcards.

## Path Literals

A path literal starts with a slash:

~~~c
/tempZone/home/rods
~~~

A path literal is just like a string, you can use variable expansion, escape characters, etc:

~~~c
/*Zone/home/*User/\\.txt
~~~

In addition to the characters that must be escaped in a string, the following characters must also be escaped in a path literal:

~~~c
','  comma
';'  semicolon
')'  right parenthesis
' '  space
~~~

A path literal can be assigned to a variable:

~~~c
*H = /tempZone/home/rods
~~~

New path literals can be constructed from paths but it must start with "/", the rule engine automatically removes redundant leading slashes in a path:

~~~c
*F = /*H/foo.txt
~~~

Path literals can be used in various places. If a path literal points to a collection, it can be used in a foreach loop to loop over all data objects under that collection.

~~~c
foreach(*D in *H) {
  ...
}
~~~

A path literal can also be used in collection and data object related microservice calls:

~~~c
msiCollCreate(/*H/newColl, "", *Status);
msiRmColl(/*H/newColl, "", *Status);
~~~

## Contributors

This page content was originally written by Hao Xu and then converted by [Samuel Lampa](https://github.com/samuell/irods-cheatsheets/blob/master/irods-rule-lang-full-guide.md).
