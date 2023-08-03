show ivorysql.sort_desc_nulls_last;

create table tbl_sort (id int, t date);

insert into tbl_sort values(1, '2023-08-01'::date + 1);
insert into tbl_sort values(2, '2023-08-01'::date + 2);
insert into tbl_sort values(3, '2023-08-01'::date + 3);
insert into tbl_sort values(4, '2023-08-01'::date + 4);
insert into tbl_sort values(5, NULL);
insert into tbl_sort values(6, NULL);

select * from tbl_sort order by t;

select * from tbl_sort order by t desc; -- nulls first (default)

select * from tbl_sort order by t desc nulls last;

set ivorysql.sort_desc_nulls_last to true;

show ivorysql.sort_desc_nulls_last;

select * from tbl_sort order by t desc;	-- nulls last (set by guc)

select * from tbl_sort order by t desc nulls last;

select * from tbl_sort order by t desc nulls first;
