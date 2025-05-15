select * from myfriends where a=30000;
create index indexA ON myfriends(a);
select * from myfriends where a=30000;
DROP index indexA ON myfriends;