select to_seconds(timestamp) as time, value+adj as value from
  (select c.timestamp timestamp, c.value value, 
		(select sum(value) from coldadj a
			where a.timestamp <= c.timestamp
		) adj from coldcnt c order by timestamp
  ) t;
select to_seconds(timestamp) as time, value+adj as value from
  (select c.timestamp timestamp, c.value value, 
		(select sum(value) from hotadj a
			where a.timestamp <= c.timestamp
		) adj from hotcnt c order by timestamp
  ) t;
