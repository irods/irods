-- run these SQL, using Oracle client sqlplus 

alter table R_RULE_EXEC modify rule_name varchar(2700);
delete from R_TOKN_MAIN  where token_id = 1700;
insert into R_TOKN_MAIN values ('action_type',1800,'generic','','','','','1170000000','1170000000');

exit;
