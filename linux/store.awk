#!/usr/bin/awk

# (zcat /var/log/syslog.*.gz;cat /var/log/syslog.1 /var/log/syslog) \
# | grep 'pulsecounter: store:' > store.log
# then "awk -f store.awk store.log"

# Jan  1 11:49:00 pccross pulsecounter: store: "2,156"
# 1    2 3        4       5             6      7

BEGIN {
  m["Jan"] = 1;
  m["Feb"] = 2;
  m["Mar"] = 3;
  m["Apr"] = 4;
  m["May"] = 5;
  m["Jun"] = 6;
  m["Jul"] = 7;
  m["Aug"] = 8;
  m["Sep"] = 9;
  m["Oct"] = 10;
  m["Nov"] = 11;
  m["Dec"] = 12;
  table[1] = "coldcnt";
  table[2] = "hotcnt";
}

{
  if ($1 ~ /Jan/) yr = 2016; else yr = 2015;
  split($3, t, ":");
  tstr = sprintf ("%04d %02d %02d %02d %02d %02d", yr, m[$1], $2, t[1], t[2], t[3]);
  print tstr, $7;
  tsstr = strftime("%Y-%m-%d %H:%M:%S", mktime(tstr), 1)
  match($7, /"([12]),([0-9]+)"/, v)
  print "insert into " table[v[1]] " values ('" tsstr "', " v[2] ");"
}
