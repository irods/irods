-- add new resource hierarchy columns and index
alter table r_resc_main add column resc_children varchar(1000);
alter table r_resc_main add column resc_context varchar(1000);
alter table r_resc_main add column resc_parent varchar(1000);
alter table r_data_main add column resc_hier varchar(1000);
alter table r_resc_main add column resc_objcount integer DEFAULT 0;

-- update leaf node object counts
update r_resc_main as r SET resc_objcount = ( select count(*) from r_data_main as d where d.resc_name=r.resc_name );

-- reset resc_group for consistency
update r_data_main SET resc_group_name=resc_name;

-- populate the resc_hier field
update r_data_main SET resc_hier=resc_name;

-- define and populate new configuration table
create table R_GRID_CONFIGURATION
 (
   namespace varchar(2700),
   option_name varchar(2700),
   option_value varchar(2700)
 );
insert into r_grid_configuration values ( 'database', 'schema_version', '1' );

