
insert into r_user_group (group_user_id, user_id) select user_id, user_id from r_user_main where user_type_name != 'rodsgroup';
