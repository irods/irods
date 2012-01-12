-- Depending on your ICAT DBMS type, run this SQL statement using 
---    the MySQL client mysql,
---    the Oracle client sqlplus,
---    or the PostgreSQL client psql.
---
--- The 2.3 to 2.4 patch dropped and recreated the R_RULE_MAIN table,
--- but did not recreate the index (which was removed when the table
--- was dropped).  If the index already exists, you will get an error
--- message to that effect, which is harmless.  This index will improve
--- performance slightly.

create unique index idx_rule_main1 on R_RULE_MAIN (rule_id);
