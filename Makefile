all:
	g++ configmanager.cpp main.cpp sqlwrapper.cpp database.cpp networkmessage.cpp -std=c++11 -ggdb -o bin/realotsquerymanager -I/usr/local/include/ -L/usr/compat/linux/lib/obsolete/ -llua -lpthread -L/usr/lib64/mysql/ -lmysqlclient
