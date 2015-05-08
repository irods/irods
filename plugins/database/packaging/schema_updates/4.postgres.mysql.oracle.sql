create index idx_quota_main1 on R_QUOTA_MAIN (user_id);
delete from R_TOKN_MAIN where token_name = 'domainadmin';
delete from R_TOKN_MAIN where token_name = 'rodscurators';
delete from R_TOKN_MAIN where token_name = 'storageadmin';
