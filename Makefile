BIN=svr
SRC=*.cc
CXX=g++
CXXFLAGS=-std=c++11
PWD=$(shell pwd)

.PHONY:all
all:$(BIN) CGI

$(BIN):$(SRC)
	$(CXX) -o $@ $^ $(CXXFLAGS)

CGI:
	cd $(PWD)/cgi;\
	make;\
	cd -;


.PHONY:release
release:
	make all;\
	mkdir release;\
	cp $(BIN) release/;\
	cp -rf wwwroot release/wwwroot;\
	cp -rf cgi/mysql_cgi release/wwwroot;\
	cp -rf cgi/mysql_cgi release/wwwroot;\

.PHONY:clean
clean:
	rm -rf $(BIN);\
	rm -rf wwwroot/mysql_cgi;\
	rm -rf release;\
	cd $(PWD)/cgi;\
	make clean;
