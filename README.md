# Dependences
- libmysqlclient-dev
- libevent-dev


# Database
- import table from dumpfile
```sh
$ cd db/
$ mysql -u username --password=your_password database_name < CREATE_TABLE.sql

```


# Build
```sh
$ make server
```

