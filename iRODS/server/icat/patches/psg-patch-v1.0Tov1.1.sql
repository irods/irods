-- run these SQL, using postgresql client psql 

alter table R_RULE_EXEC drop column rule_name;
alter table R_RULE_EXEC add column rule_name varchar(2700);

delete from R_TOKN_MAIN  where token_id = 1700;
insert into R_TOKN_MAIN values ('action_type',1800,'generic','','','','','1170000000','1170000000');
