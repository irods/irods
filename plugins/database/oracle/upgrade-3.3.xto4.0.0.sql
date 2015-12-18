-- add new resource hierarchy columns and index
alter table R_RESC_MAIN add resc_children varchar(1000);
alter table R_RESC_MAIN add resc_context varchar(1000);
alter table R_RESC_MAIN add resc_parent varchar(1000);
alter table R_DATA_MAIN add resc_hier varchar(1000);
alter table R_RESC_MAIN add resc_objcount integer DEFAULT 0;

-- update leaf node object counts
update R_RESC_MAIN r SET resc_objcount = ( select count(*) from R_DATA_MAIN d where d.resc_name=r.resc_name );

-- populate the resc_hier field
update R_DATA_MAIN SET resc_hier=resc_name;

-- define and populate new configuration table
create table R_GRID_CONFIGURATION
 (
   namespace varchar(2700),
   option_name varchar(2700),
   option_value varchar(2700)
 );
insert into R_GRID_CONFIGURATION values ( 'database', 'schema_version', '1' );

