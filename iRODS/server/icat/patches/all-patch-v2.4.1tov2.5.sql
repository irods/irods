-- Depending on your ICAT DBMS type, run this SQL statement using 
--    the MySQL client mysql,
--    the Oracle client sqlplus,
--    or the PostgreSQL client psql.

-- New type and classes for Database Resources (DBR)
insert into R_TOKN_MAIN values ('resc_type',405,'database','','','','','1288631300','1288631300');

insert into R_TOKN_MAIN values ('resc_class',504,'postgresql','','','','','1288631300','1288631300');
insert into R_TOKN_MAIN values ('resc_class',505,'mysql','','','','','1288631300','1288631300');
insert into R_TOKN_MAIN values ('resc_class',506,'oracle','','','','','1288631300','1288631300');

-- New data_type for Database Objects (used with DBRs)
insert into R_TOKN_MAIN values ('data_type',1695,'database object','text','','','','1288631300','1288631300');

-- New table to hold specific-queries (SQL-based queries)
create table R_SPECIFIC_QUERY
(
   alias varchar(1000),
   sqlStr varchar(2700),
   create_ts varchar(32)
);

-- Add a couple built-in specific queries (to see the specific queries).
insert into R_SPECIFIC_QUERY (alias, sqlStr, create_ts) values ('ls', 'select alias,sqlStr from R_SPECIFIC_QUERY', '01292940000');

insert into R_SPECIFIC_QUERY (alias, sqlStr, create_ts) values ('lsl', 'select alias,sqlStr from R_SPECIFIC_QUERY where sqlStr like ?', '01292940000');

create index idx_specific_query1 on R_SPECIFIC_QUERY (sqlStr);
create index idx_specific_query2 on R_SPECIFIC_QUERY (alias);
