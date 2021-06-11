SYSTEMC_DIR=/usr/local/systemc-2.3.3
LIB_DIR=-L$(SYSTEMC_DIR)/lib-linux64
INC_DIR=-I$(SYSTEMC_DIR)/include
CXXFLAG=-Wl,-rpath,$(SYSTEMC_DIR)/lib-linux64
LIB=-lsystemc

APP=8051_iss
CPP_LIST:=$(shell find *.cpp)
OBJ_LIST:=$(CPP_LIST:.cpp=.o)

$(APP): $(OBJ_LIST)
	g++ -o $@ $^ $(LIB_DIR) $(INC_DIR) $(LIB) $(CXXFLAG)

%.o:%.cpp
	g++ -c -o $@ $^ $(LIB_DIR) $(INC_DIR) $(LIB) $(CXXFLAG)

clean:
	rm -rf $(APP)
	rm -rf $(OBJ_LIST)