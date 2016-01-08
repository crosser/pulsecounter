drop table if exists adjustment;
drop table if exists counter;
create table adjustment (timestamp datetime, value int);
create table counter    (timestamp datetime, value int);

insert into adjustment values ('2016-01-07 00:00:00', 10);
insert into counter    values ('2016-01-07 00:00:01', 5);
insert into counter    values ('2016-01-07 00:00:02', 10);
insert into counter    values ('2016-01-07 00:00:03', 15);
insert into adjustment values ('2016-01-07 00:00:04', 30);
insert into counter    values ('2016-01-07 00:00:05', 0);
insert into counter    values ('2016-01-07 00:00:06', 5);
insert into counter    values ('2016-01-07 00:00:07', 10);

select timestamp, value+adj as value from
  (select c.timestamp timestamp, c.value value, 
                (select value from adjustment a
                        where a.timestamp <= c.timestamp
                        order by timestamp desc limit 1) adj from counter c
  ) t;

