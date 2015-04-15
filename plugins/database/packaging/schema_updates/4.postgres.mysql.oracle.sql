create index idx_quota_main1 on R_QUOTA_MAIN (user_id);
delete from r_tokn_main where token_name = 'domainadmin';
delete from r_tokn_main where token_name = 'rodscurators';
delete from r_tokn_main where token_name = 'storageadmin';
