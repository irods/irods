ALTER TABLE R_DATA_MAIN ADD RESC_ID integer;
ALTER TABLE R_RESC_MAIN ADD RESC_PARENT_CONTEXT CLOB;
UPDATE R_SPECIFIC_QUERY SET sqlstr='WITH coll AS (SELECT coll_id, coll_name FROM r_coll_main WHERE R_COLL_MAIN.coll_name = ? OR R_COLL_MAIN.coll_name LIKE ?) SELECT DISTINCT d.data_id, (SELECT coll_name FROM coll WHERE coll.coll_id = d.coll_id) coll_name, d.data_name, d.data_repl_num, d.resc_name, d.data_path, d.resc_id FROM R_DATA_MAIN d WHERE d.coll_id = ANY(ARRAY(SELECT coll_id FROM coll)) ORDER BY coll_name, d.data_name, d.data_repl_num' where alias='DataObjInCollReCur';

update r_data_main rdm set resc_id = ( select resc_id from ( select dm.data_id, dm.data_repl_num, rm.resc_id from R_RESC_MAIN rm, ( select data_id, data_repl_num, resc_hier from R_DATA_MAIN ) dm where regexp_replace(dm.resc_hier, '^.*;', '') = rm.resc_name ) am where rdm.data_id = am.data_id and rdm.data_repl_num = am.data_repl_num );

update R_RESC_MAIN rdm set resc_parent = ( select resc_id from ( select resc_name, resc_id from R_RESC_MAIN ) am where am.resc_name = rdm.resc_parent ); 


