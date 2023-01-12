konfiture-cxx
======

konfiture is configuration markup language. it looks like XML but much easier to write and read.

The syntax is quite simple. There are two types of entities: nodes and attributes.

Each node can have many attributes and many children nodes. 


Simple config

```
// single-line comment
host = 127.0.0.1
port = 8080

server-name = "Server name"
```

also you can use simple version for key-value nodes
```
host 127.0.0.1
port 8080

server-name "Server name"
````

Multiple nodes and attributes

```
/* this is example configuration file */
table name=User {
    column name=id type=integer required=true primaryKey=true autoIncrement=true
    column name=username type=varchar size=255 required=true {
        validates regex="/^[a-z0-9]+$/"
    }
}

table name=Files {
    column name=id type=integer required=true primaryKey=true autoIncrement=true
    column name=filename type=varchar size=255 required=true
    column name=filesize type=longint required=true
}
```

You can also use namespaces 
```
ui::menu {
    ui::menu name=File {
        ui::menu_entry name="New" text="&New" {
            on-click call=""
        }
    }
}
```

there are no requirements on namespace syntax (foo.bar, foo::bar and foo:bar are valid)

```
$ sudo apt-get install build-essential cmake flex bison
$ cmake CMakeLists.txt
$ make
$ ./konfiture-lint < examples/scheme.conf

parsing ok
  node table "name"="User"
    node column "name"="id" "type"="integer" "required"="true" "primaryKey"="true" "autoIncrement"="true"
    node column "name"="username" "type"="varchar" "size"="255" "required"="true"
  node table "name"="Files"
    node column "name"="id" "type"="integer" "required"="true" "primaryKey"="true" "autoIncrement"="true"
    node column "name"="filename" "type"="varchar" "size"="255" "required"="true"
    node column "name"="filesize" "type"="longint" "required"="true"

```

or if you are a Mac user
```
$ brew install bison
$ brew install flex
$ PATH="/usr/local/Cellar/flex/2.5.39/bin:$PATH"
$ PATH="/usr/local/Cellar/bison/3.0.4/bin:$PATH"
$ cmake CMakeLists.txt
$ make
```
