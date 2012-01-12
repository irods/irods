/*

Copyright (c), The Regents of the University of California            
For more information please refer to files in the COPYRIGHT directory 

This prolog script was used in developing the ICAT GeneralQuery SQL
generation logic (icatGeneralQuerySetup.c).

*/

%attrInfo(

fklink(0,r_coll_main,r_data_main,'r_coll_main.coll_id = r_data_main.coll_id').
fklink(1,r_rsrc_group,r_rsrc_main,'r_rsrc_group.rsrc_id = r_rsrc_main.rsrc_id').
fklink(2,r_rsrc_main,r_rsrc_metamap,'r_rsrc_main.rsrc_id = r_rsrc_metamap.object_id').
fklink(3,r_coll_main,r_coll_metamap,'r_coll_main.coll_id = r_coll_metamap.object_id').
fklink(4,r_data_main,r_data_metamap,'r_data_main.data_id = r_data_metamap.object_id').
fklink(5,r_rule_main,r_rule_metamap,'r_rule_main.rule_id = r_rule_metamap.object_id').
fklink(6,r_met2_main,r_met2_metamap,'r_met2_main.meta_id = r_met2_metamap.object_id').
fklink(7,r_rsrc_main,r_rsrc_access,'r_rsrc_main.rsrc_id = r_rsrc_access..object_id').
fklink(8,r_coll_main,r_coll_access,'r_coll_main.coll_id = r_coll_access.object_id').
fklink(9,r_data_main,r_data_access,'r_data_main.data_id = r_data_access.object_id').
fklink(10,r_rule_main,r_rule_access,'r_rule_main.rule_id = r_rule_access.object_id').
fklink(11,r_met2_main,r_met2_access,'r_met2_main.meta_id = r_met2_access.object_id').
fklink(12,r_rsrc_main,r_rsrc_deny_access,'r_rsrc_main.rsrc_id = r_rsrc_deny_access.object_id').
fklink(13,r_coll_main,r_coll_deny_access,'r_coll_main.coll_id = r_coll_deny_access.object_id').
fklink(14,r_data_main,r_data_deny_access,'r_data_main.data_id = r_data_deny_access.object_id').
fklink(15,r_rule_main,r_rule_deny_access,'r_rule_main.rule_id = r_rule_deny_access.object_id').
fklink(16,r_met2_main,r_met2_deny_access,'r_met2_main.meta_id = r_met2_deny_access.object_id').
fklink(17,r_rsrc_main,r_rsrc_audit,'r_rsrc_main.rsrc_id = r_rsrc_audit.object_id').
fklink(18,r_coll_main,r_coll_audit,'r_coll_main.coll_id = r_coll_audit.object_id').
fklink(19,r_data_main,r_data_audit,'r_data_main.data_id = r_data_audit.object_id').
fklink(20,r_rule_main,r_rule_audit,'r_rule_main.rule_id = r_rule_audit.object_id').
fklink(21,r_met2_main,r_met2_audit,'r_met2_main.meta_id = r_met2_audit.object_id').

fklink(22,r_rsrc_metamap,r_rsrc_meta_main, 'r_rsrc_metamap.object_id = r_rsrc_meta_main').
fklink(23,r_coll_metamap,r_coll_meta_main, 'r_coll_metamap.object_id = r_coll_meta_main').
fklink(24,r_data_metamap,r_data_meta_main, 'r_data_metamap.object_id = r_data_meta_main').
fklink(25,r_rule_metamap,r_rule_meta_main, 'r_rule_metamap.object_id = r_rule_meta_main').
fklink(26,r_met2_metamap,r_met2_meta_main, 'r_met2_metamap.object_id = r_met2_meta_main').
fklink(27,r_rsrc_access, r_rsrc_tokn_accs, 'r_rsrc_access.access_typ_id = r_rsrc_tokn_accs.token_id').
fklink(28,r_rsrc_access, r_rsrc_user_group,'r_rsrc_access.user_id = r_rsrc_user_group.group_user_id').
fklink(29,r_coll_access, r_coll_tokn_accs, 'r_coll_access.access_typ_id = r_coll_tokn_accs.token_id').
fklink(30,r_coll_access, r_coll_user_group,'r_coll_access.user_id = r_coll_user_group.group_user_id').
fklink(31,r_data_access, r_data_tokn_accs, 'r_data_access.access_typ_id = r_data_tokn_accs.token_id').
fklink(32,r_data_access, r_data_user_group,'r_data_access.user_id = r_data_user_group.group_user_id').
fklink(33,r_rule_access, r_rule_tokn_accs, 'r_rule_access.access_typ_id = r_rule_tokn_accs.token_id').
fklink(34,r_rule_access, r_rule_user_group,'r_rule_access.user_id = r_rule_user_group.group_user_id').
fklink(35,r_met2_access, r_met2_tokn_accs, 'r_met2_access.access_typ_id = r_met2_tokn_accs.token_id').
fklink(36,r_met2_access, r_met2_user_group,'r_met2_access.user_id = r_met2_user_group.group_user_id').
fklink(37,r_rsrc_deny_access,r_rsrc_tokn_deny_accs,'r_rsrc_deny_access.access_typ_id = r_rsrc_tokn_deny_accs.token_id').
fklink(38,r_rsrc_deny_access,r_rsrc_user_group,    'r_rsrc_deny_access.user_id = r_rsrc_user_group.group_user_id').
fklink(39,r_coll_deny_access,r_coll_tokn_deny_accs,'r_coll_deny_access.access_typ_id = r_coll_tokn_deny_accs.token_id').
fklink(40,r_coll_deny_access,r_coll_user_group,    'r_coll_deny_access.user_id = r_coll_user_group.group_user_id').
fklink(41,r_data_deny_access,r_data_tokn_deny_accs,'r_data_deny_access.access_typ_id = r_data_tokn_deny_accs.token_id').
fklink(42,r_data_deny_access,r_data_user_group,    'r_data_deny_access.user_id = r_data_user_group.group_user_id').
fklink(43,r_rule_deny_access,r_rule_tokn_deny_accs,'r_rule_deny_access.access_typ_id = r_rule_tokn_deny_accs.token_id').
fklink(44,r_rule_deny_access,r_rule_user_group,    'r_rule_deny_access.user_id = r_rule_user_group.group_user_id').
fklink(45,r_met2_deny_access,r_met2_tokn_deny_accs,'r_met2_deny_access.access_typ_id = r_met2_tokn_deny_accs.token_id').
fklink(46,r_met2_deny_access,r_met2_user_group,    'r_met2_deny_access.user_id = r_met2_user_group.group_user_id').
fklink(47,r_rsrc_audit,r_rsrc_tokn_audit,'r_rsrc_audit.action_id = r_rsrc_tokn_audit.token_id').
fklink(48,r_rsrc_audit,r_rsrc_user_group,'r_rsrc_audit.user_id = r_rsrc_user_group.group_user_id').
fklink(49,r_coll_audit,r_coll_tokn_audit,'r_coll_audit.action_id = r_coll_tokn_audit.token_id').
fklink(50,r_coll_audit,r_coll_user_group,'r_coll_audit.user_id = r_coll_user_group.group_user_id').
fklink(51,r_data_audit,r_data_tokn_audit,'r_data_audit.action_id = r_data_tokn_audit.token_id').
fklink(52,r_data_audit,r_data_user_group,'r_data_audit.user_id = r_data_user_group.group_user_id').
fklink(53,r_rule_audit,r_rule_tokn_audit,'r_rule_audit.action_id = r_rule_tokn_audit.token_id').
fklink(54,r_rule_audit,r_rule_user_group,'r_rule_audit.user_id = r_rule_user_group.group_user_id').
fklink(55,r_met2_audit,r_met2_tokn_audit,'r_met2_audit.action_id = r_met2_tokn_audit.token_id').
fklink(56,r_met2_audit,r_met2_user_group,'r_met2_audit.user_id = r_met2_user_group.group_user_id').

fklink(57,r_rsrc_user_group,r_user_main,'r_rsrc_user_group.user_id = r_user_main.user_id').
fklink(58,r_coll_user_group,r_user_main,'r_coll_user_group.user_id = r_user_main.user_id').
fklink(59,r_data_user_group,r_user_main,'r_data_user_group.user_id = r_user_main.user_id').
fklink(60,r_rule_user_group,r_user_main,'r_rule_user_group.user_id = r_user_main.user_id').
fklink(61,r_met2_user_group,r_user_main,'r_met2_user_group.user_id = r_user_main.user_id').

fklink(62,r_user_main,r_user_password,'r_user_main.user_id = r_user_password.user_id').
fklink(63,r_user_main,r_user_session_key,'r_user_main.user_id = r_user_session_key.user_id').




table(r_user_password,r_user_password,0).
table(r_user_session_key,r_user_session_key,0).
table(r_tokn_main,r_tokn_main,0).
table(r_rsrc_group,r_rsrc_group,0).
table(r_zone_main,r_zone_main,1).
table(r_rsrc_main,r_rsrc_main,1).
table(r_coll_main,r_coll_main,1).
table(r_data_main,r_data_main,1).
table(r_met2_main,r_met2_main,1).
table(r_rule_main,r_rule_main,1).
table(r_user_main,r_user_main,1).

table(r_rsrc_access,'r_objt_access r_rsrc_access',2).
table(r_coll_access,'r_objt_access r_coll_access',2).
table(r_data_access,'r_objt_access r_data_access',2).
table(r_met2_access,'r_objt_access r_met2_access',2).
table(r_rule_access,'r_objt_access r_rule_access',2).
table(r_rsrc_audit,'r_objt_audit r_rsrc_audit',2).
table(r_coll_audit,'r_objt_audit r_coll_audit',2).
table(r_data_audit,'r_objt_audit r_data_audit',2).
table(r_met2_audit,'r_objt_audit r_met2_audit',2).
table(r_rule_audit,'r_objt_audit r_rule_audit',2).
table(r_rsrc_deny_access,'r_objt_deny_access r_rsrc_deny_access',2).
table(r_coll_deny_access,'r_objt_deny_access r_coll_deny_access',2).
table(r_data_deny_access,'r_objt_deny_access r_data_deny_access',2).
table(r_met2_deny_access,'r_objt_deny_access r_met2_deny_access',2).
table(r_rule_deny_access,'r_objt_deny_access r_rule_deny_access',2).
table(r_rsrc_metamap,'r_objt_metamap r_rsrc_metamap',2).
table(r_coll_metamap,'r_objt_metamap r_coll_metamap',2).
table(r_data_metamap,'r_objt_metamap r_data_metamap',2).
table(r_met2_metamap,'r_objt_metamap r_met2_metamap',2).
table(r_rule_metamap,'r_objt_metamap r_rule_metamap',2).

table(r_rsrc_user_group,'r_user_group r_rsrc_user_group',3).
table(r_coll_user_group,'r_user_group r_coll_user_group',3).
table(r_data_user_group,'r_user_group r_data_user_group',3).
table(r_met2_user_group,'r_user_group r_met2_user_group',3).
table(r_rule_user_group,'r_user_group r_rule_user_group',3).

table(r_rsrc_tokn_accs,'r_tokn_main r_rsrc_tokn_accs',4).
table(r_coll_tokn_accs,'r_tokn_main r_coll_tokn_accs',4).
table(r_data_tokn_accs,'r_tokn_main r_data_tokn_accs',4).
table(r_rule_tokn_accs,'r_tokn_main r_rule_tokn_accs',4).
table(r_met2_tokn_accs,'r_tokn_main r_met2_tokn_accs',4).
table(r_rsrc_tokn_deny_accs,'r_tokn_main r_rsrc_tokn_deny_accs',4).
table(r_coll_tokn_deny_accs,'r_tokn_main r_coll_tokn_deny_accs',4).
table(r_data_tokn_deny_accs,'r_tokn_main r_data_tokn_deny_accs',4).
table(r_rule_tokn_deny_accs,'r_tokn_main r_rule_tokn_deny_accs',4).
table(r_met2_tokn_deny_accs,'r_tokn_main r_met2_tokn_deny_accs',4).
table(r_rsrc_tokn_audit,'r_tokn_main r_rsrc_tokn_audit',4).
table(r_coll_tokn_audit,'r_tokn_main r_coll_tokn_audit',4).
table(r_data_tokn_audit,'r_tokn_main r_data_tokn_audit',4).
table(r_rule_tokn_audit,'r_tokn_main r_rule_tokn_audit',4).
table(r_met2_tokn_audit,'r_tokn_main r_met2_tokn_audit',4).
table(r_rsrc_meta_main,'r_meta_main r_rsrc_meta_main',4).
table(r_coll_meta_main,'r_meta_main r_coll_meta_main',4).
table(r_data_meta_main,'r_meta_main r_data_meta_main',4).
table(r_rule_meta_main,'r_meta_main r_rule_meta_main',4).
table(r_met2_meta_main,'r_meta_main r_met2_meta_main',4).



% findJoins([r_user_password,r_user_main,r_user_session_key],P,Q).
% findJoins([r_user_main,r_data_main,r_data_tokn_accs],P,Q).
% findJoins([r_user_main,r_coll_main,r_data_main,r_data_tokn_accs],P,Q).
% findJoins([r_coll_main,r_data_main],P,Q).
% getSql(['r_user_main.user_name', 'r_data_main.data_name','r_data_main.path_name'], [s,s,s], 
% ['r_user_main.user_name','r_coll_main.coll_name','r_data_main.data_name', 
% 'r_data_tokn_accs.access'], 
% ['= \'sekar@sdsc\'','= \'/home/sekar.sdsc\'','= \'foo.dat\'','= \'write\''],[],S).

%getSql(['r_data_main.data_name','r_coll_main.coll_name'], [s,s], 
%['r_coll_main.coll_name'],['= \'sekar@sdsc\''],[],S).

%--------------------------------------------------------------------------

%getSql(SelectAttributesSet, SelectionCriteriaSet, ComparisonAttributesSet, 
%	CompareStringSet, OrderByStringSet, Sql).

getSql(SelectAttributesSet, SelectionCriteriaSet, CompareAttributesSet, 
       CompareStringSet, OrderByStringSet, Sql) :-
	  getTableFromAttr(SelectAttributesSet,SelectTableSet),
	  getTableFromAttr(CompareAttributesSet, CompareTableSet),
	  flatten([SelectTableSet, CompareTableSet], ListOfTables),
	  findJoins(ListOfTables, ListOfJoins, ListOfAliases),
	  makeSelectString(SelectAttributesSet, SelectionCriteriaSet, SelString, GroupByString),
	  makeCompareString(CompareAttributesSet, CompareStringSet, CompString),
	  makeCommaString(OrderByStringSet, OrderByString),
	  stitchSql(SelString, CompString, GroupByString, OrderByString,
		    ListOfAliases, ListOfJoins, Sql).



findJoins(ListOfTables,ListOfJoins,ListOfAliases) :- qTables(ListOfTables,L),
	  fJMain(L,[],ListOfJoins,[],ListOfNewTables,[],ListOfPaths,[],ListOfAliases2),
	  list_to_set(ListOfAliases2,ListOfAliases).

fJMain([],P,P,Q,Q,R,R,M,M) :- !.
fJMain([A],[],[],[],[A],[],[],[],[T]) :- table(A,T,N1),!.
fJMain([A|L],P,P1,Q,Q1,R,R1,M,M1) :- fJ([A],P,P2,Q,Q2,R,R2,M,M2,L),
	  fJMain(L,P2,P1,Q2,Q1,R2,R1,M2,M1).
fJMain([A|L],P,P1,Q,Q1,R,R1,M,M1) :- table(A,T,N1),fJMain(L,P,P1,[A|Q],Q1,R,R1,[A|M],M1),!.

fJ([],P,P,Q,Q,R,R,M,M,L) :- !.
fJ([A|S],P,P1,Q,Q1,R,R1,M,M1,L) :- table(A,A1,4),member(A,Q),fJ(S,P,P1,Q,Q1,R,R1,M,M1,L).
fJ([A|S],P,P1,Q,Q1,R,R1,M,M1,L) :- table(A,A1,4),!, fail.
fJ([A|S],P,P1,Q,Q1,R,R1,M,M1,L) :- fklink(N,A,B,C), table(B,B1,4), !, 
    table(A,T,N1), isKnown4(B,S,Q,S2,Q2,M,M2,L), addTable(T,M2,T2), remMember(A,S2,S3),
    fJ(S3,[C|P],P1,[A|Q2],Q1,[N|R],R1,T2,M1,L).
fJ([A|S],P,P1,Q,Q1,R,R1,M,M1,L) :- fklink(N,A,B,C), \+member(N,R), 
    table(A,T,N1), isKnownOrElse(B,S,Q,S2), addTable(T,M,T2), remMember(A,S2,S3),   
    fJ(S3,[C|P],P1,[A|Q],Q1,[N|R],R1,T2,M1,L).
fJ([A|S],P,P1,Q,Q1,R,R1,M,M1,L) :- fklink(N,B,A,C), \+member(N,R), 
    table(A,T,N1), isKnownOrElse(B,S,Q,S2), addTable(T,M,T2),  remMember(A,S2,S3),  
    fJ(S3,[C|P],P1,[A|Q],Q1,[N|R],R1,T2,M1,L).



addTable(T,M,M) :- member(T,M),!.
addTable(T,M,[T|M]).

isKnownOrElse(B,S,Q,S) :- member(B,Q),!.
isKnownOrElse(B,S,Q,[B|S]) :- ! .

%isKnown4(B,S,Q,S,Q,M,M) :- member(B,Q),!.
%isKnown4(B,S,Q,S1,[B|Q],M,[T|M]) :- member(B,S),remMember(B,S,S1),table(B,T,N1).


isKnown4(B,S,Q,S,Q,M,M,L) :- member(B,Q),!.
isKnown4(B,S,Q,S1,[B|Q],M,[T|M],L) :- member(B,S),remMember(B,S,S1),table(B,T,N1).
isKnown4(B,S,Q,S,[B|Q],M,[T|M],L) :- member(B,L),!,table(B,T,N1).


remMember(B,[],[]).
remMember(B,[B|S],T) :- !,remMember(B,S,T) .
remMember(B,[A|S],[A|T]) :- remMember(B,S,T).

qTables(S,T) :- qT(S,S1,T1,1), qT(S1,S2,T2,2), 
    qT(S2,S3,T3,3),qT(S3,S4,T4,4),flatten([T1|[T2|[T3|[T4|S4]]]],T).

qT([],[],[],N).
qT([A|S],S1,[A|T], N) :- table(A,A1,N),!, qT(S,S1,T,N).
qT([A|S],[A|S1],T,N) :- qT(S,S1,T,N).


makeSelectString([A],[s],A,'') :- !.
makeSelectString([A],[B],W,A) :- swritef(W,'%w(%w)',[B,A]),!.
makeSelectString([A|S],[s|T],W,Y) :- !,makeSelectString(S,T,W1,Y),
	  swritef(W,'%w, %w',[A,W1]).
makeSelectString([A|S],[B|T],W,Y) :- !,makeSelectString(S,T,W1,Y1),
	  swritef(W,'%w(%w), %w',[B,A,W1]), swritef(Y,'%w, %w',[A,Y1]).

makeCompareString([],[],'') :- !.
makeCompareString([A],[B],W) :- !, swritef(W,'%w %w', [A,B]).
makeCompareString([A|S],[B|T],W) :- makeCompareString(S,T,W1),
	  swritef(W, '%w %w AND %w',[A,B,W1]).

makeCommaString([],'') :- !.
makeCommaString([A],A) :- !.
makeCommaString([A|S],W) :- makeCommaString(S,W1),
	  swritef(W,'%w, %w',[A,W1]).

makeAndString([],'') :- !.
makeAndString([A],A) :- !.
makeAndString([A|S],W) :- makeAndString(S,W1),
	  swritef(W,'%w AND %w',[A,W1]).

stitchSql(SelString, CompString, GroupByString, OrderByString,
		    ListOfAliases, ListOfJoins, Sql) :-
	  getGBS(GroupByString, G), getOBS(OrderByString, O),
	  makeCommaString(ListOfAliases,StringOfAliases),
          makeAndString(ListOfJoins, StringOfJoins),
	  getWhere(CompString, StringOfJoins, W),
	  swritef(Sql, 'SELECT %w FROM %w %w %w %w',
		  [SelString,StringOfAliases, W, G,O]).


getGBS(S,'') :- string_length(S,0),!.
getGBS(S,W) :- swritef(W, 'GROUP BY %w',[S]).

getOBS(S,'') :- string_length(S,0),!.
getGOS(S,W) :- swritef(W, 'ORDER BY %w',[S]).

getWhere(S,T,'') :- string_length(S,0),string_length(T,0),!.
getWhere(S,T,W) :- string_length(S,0),!, swritef(W, 'WHERE %w' , [T]).
getWhere(S,T,W) :- string_length(T,0),!, swritef(W, 'WHERE %w' , [S]).
getWhere(S,T,W) :- swritef(W, 'WHERE %w AND %w' , [S,T]).

getTableFromAttr([],[]).
getTableFromAttr([A|S],[B|T]) :- getTabName(A,B), getTableFromAttr(S,T).

getTabName(A,B) :- sub_string(A,S,L,C,'.'),sub_string(A,0,S,D,B1),name(B1,E),name(B,E).



