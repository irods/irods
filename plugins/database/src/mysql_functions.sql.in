drop function if exists R_ObjectId_nextval;
delimiter %%
create function R_ObjectId_nextval()
returns bigint
begin
   insert into R_ObjectId_seq_tbl values (NULL) ;
   set @R_ObjectId_val=LAST_INSERT_ID() ;
   delete from R_ObjectId_seq_tbl where nextval = @R_ObjectId_val ;
   return @R_ObjectId_val ;
end
%%
delimiter ;

drop function if exists R_ObjectId_currval;
create function R_ObjectId_currval() returns bigint deterministic return @R_ObjectId_val;
