-- run these SQL statements, using the postgresql client psql 
--
-- We could also do an 'alter table R_USER_MAIN drop column
-- user_distin_name;' but we do not so that if one switches back to an
-- older version of iRODS servers, it will still function.


alter table R_RESC_MAIN add column resc_status varchar(32);

create table R_USER_AUTH
(
   user_id             bigint not null,
   user_auth_name      varchar(1000),
   create_ts varchar(32)
);

insert into R_USER_AUTH ( user_id, user_auth_name ) select user_id, user_distin_name from r_user_main where user_distin_name <> '';

insert into R_TOKN_MAIN values ('data_type',1694,'tar bundle','','','','','1250100000','1250100000');
insert into R_TOKN_MAIN values ('resc_type',403,'s3','','','','','1250100000','1250100000');
insert into R_TOKN_MAIN values ('resc_type',404,'MSS universal driver','','','','','1250100000','1250100000');

insert into R_RESC_MAIN (resc_id, resc_name, zone_name, resc_type_name, resc_class_name,  resc_net, resc_def_path, free_space, free_space_ts, resc_info, r_comment, resc_status, create_ts, modify_ts) values (9100, 'bundleResc', 'tempZone', 'unix file system', 'bundle', 'localhost', '/tmp', '', '', '', '', '', '1250100000','1250100000');
