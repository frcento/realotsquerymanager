all:
	g++ main.cpp sqlwrapper.cpp database.cpp networkmessage.cpp configmanager.cpp -ggdb -o querymanager -I/usr/local/include/ -L/usr/compat/linux/lib/obsolete/ -llua5.1 -lpthread -L/usr/lib/mysql/ -lmysqlclient
