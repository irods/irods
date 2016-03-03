ALTER TABLE R_DATA_MAIN ADD RESC_ID BIGINT;
ALTER TABLE R_RESC_MAIN ADD RESC_PARENT_CONTEXT TEXT;
UPDATE R_SPECIFIC_QUERY SET sqlstr='WITH coll AS (SELECT coll_id, coll_name FROM r_coll_main WHERE R_COLL_MAIN.coll_name = ? OR R_COLL_MAIN.coll_name LIKE ?) SELECT DISTINCT d.data_id, (SELECT coll_name FROM coll WHERE coll.coll_id = d.coll_id) coll_name, d.data_name, d.data_repl_num, d.resc_name, d.data_path, d.resc_id FROM R_DATA_MAIN d WHERE d.coll_id = ANY(ARRAY(SELECT coll_id FROM coll)) ORDER BY coll_name, d.data_name, d.data_repl_num' where alias='DataObjInCollReCur';

update r_data_main as rdm set resc_id = am.resc_id from ( select dm.data_id, dm.data_repl_num, rm.resc_id from r_resc_main as rm, ( select data_id, data_repl_num, regexp_replace(resc_hier, '^.*;', '') from r_data_main ) as dm where dm.regexp_replace = rm.resc_name ) as am where rdm.data_id=am.data_id and rdm.data_repl_num=am.data_repl_num;

update r_resc_main as rdm set resc_parent = am.resc_id from ( select resc_name, resc_id from r_resc_main ) as am where am.resc_name = rdm.resc_parent;

