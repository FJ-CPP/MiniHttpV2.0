BIN=test_cgi mysql_cgi

.PHONY:all
all:$(BIN)

test_cgi:test_cgi.cc
	g++ -o $@ $^ -std=c++11

mysql_cgi:mysql_cgi.cc
	g++ -o $@ $^ -std=c++11 -w -Imysql_connector/include -Lmysql_connector/lib -lmysqlclient -lpthread -ldl -static 

.PHONY:clean
clean:
	rm -f test_cgi mysql_cgi
