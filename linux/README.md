= Host side of the pulse counter infrastructure. =

pulsecounter.c is modelled after BlueZ' gatttool. Because of "strange"
design of BlueZ tools, it is not sufficient to use the library that
gets installes with it. Complete source installation is necessary.

So, unpack, configure *and build* BlueZ source (do not `make install`).
In this directory, run `make BLUEZ=/path/to/top/of/bluez/source`. You
will get the `pulsecounter` binary.

Currently (2015-12-21) supplied database adapter uses MySQL(/MariaDB).
Confiruration file (default path `/etc/pulsecounter.db`) contains lines
of this format:

    key SPACE* [=:]* SPACE* value

Lines are analyzed up to the '#' character (comment separator).
There is no escaping.

Keys are:

    host	hostname, NULL if not specified (meaning local database).
    user	database user name, NULL if not specified
    password	database password, NULL if not specified
    database	database name, "watermeter" if not specified.


