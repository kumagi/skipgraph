CXX=g++
OPTS=-O0 -fexceptions -g
LD=-lboost_program_options -lmsgpack -lmpio 
PATH_MSGPACK_RPC=/home/kumagi/work/msgpack-rpc/cpp/src/msgpack/rpc
TEST_LD= -lpthread $(LD)
GTEST_INC= -I$(GTEST_DIR)/include -I$(GTEST_DIR)
GTEST_DIR=/opt/google/gtest-1.5.0
GMOCK_DIR=/opt/google/gmock-1.5.0
WARNS= -W -Wall -Wextra -Wformat=2 -Wstrict-aliasing=4 -Wcast-qual -Wcast-align \
	-Wwrite-strings -Wfloat-equal -Wpointer-arith -Wswitch-enum
NOTIFY=&& notify-send Test success! -i ~/themes/ok_icon.png || notify-send Test failed... -i ~/themes/ng_icon.png
SRCS=$(HEADS) $(BODYS)
MSGPACK_RPC_OBJS=$(PATH_MSGPACK_RPC)/*.o

target:skipgraph
target:logic_test
target:testclient

skipgraph: skipgraph.o tcp_wrap.o sg_logic.o
	$(CXX) $^ -o $@ $(LD) $(OPTS) $(WARNS)  $(PATH_MSGPACK_RPC)/*.o -I$(PATH_MSGPACK_RPC)/ 
logic_test: sg_logic.o logic_test.o gtest_main.a libgmock.a
	$(CXX) $^ -o $@ $(GTEST_INC) $(TEST_LD) $(OPTS) $(WARNS) $(PATH_MSGPACK_RPC)/*.o -I$(PATH_MSGPACK_RPC)/ 
	./logic_test $(NOTIFY)
testclient: testclient.cpp
	$(CXX) $^ -o $@ $(LD) $(OPTS) $(WARNS)  $(PATH_MSGPACK_RPC)/*.o -I$(PATH_MSGPACK_RPC)/ 


server:server.cpp
	$(CXX) $^ -o $@ $(LD) $(OPTS) $(WARNS) $(MSGPACK_RPC_OBJS) -I$(PATH_MSGPACK_RPC)/
server2:server2.cpp
	$(CXX) $^ -o $@ $(LD) $(OPTS) $(WARNS) $(MSGPACK_RPC_OBJS) $(PATH_MSGPACK_RPC)/*.o 

sg_logic.o: sg_logic.cc sg_logic.hpp
	$(CXX) -c sg_logic.cc -o $@ $(OPTS) $(WARNS)
skipgraph.o:skipgraph.cc skipgraph.h tcp_wrap.h sg_logic.hpp debug_macro.h sg_objects.hpp
	$(CXX) -c skipgraph.cc -o $@ $(OPTS) $(WARNS) -I$(PATH_MSGPACK_RPC)
logic_test.o: logic_test.cc sg_logic.hpp sg_objects.hpp
	$(CXX) -c logic_test.cc $(OPTS) $(WARNS)
tcp_wrap.o:tcp_wrap.cpp
	$(CXX) -c $^ -o $@ $(OPTS) $(WARNS)

#%.o: %.c
#	$(CXX) -c $(OPTS) $(WARNS) $< -o $@
# gtest
gtest_main.o:
	$(CXX) $(GTEST_INC) -c $(OPTS) $(GTEST_DIR)/src/gtest_main.cc -o $@
gtest-all.o:
	$(CXX) $(GTEST_INC) -c $(OPTS) $(GTEST_DIR)/src/gtest-all.cc -o $@
gtest_main.a:gtest-all.o gtest_main.o
	ar -r $@ $^

libgmock.a:
	g++ ${GTEST_INC} -I${GTEST_DIR} -I${GMOCK_DIR}/include -I${GMOCK_DIR} -c ${GTEST_DIR}/src/gtest-all.cc
	g++ ${GTEST_INC} -I${GTEST_DIR} -I${GMOCK_DIR}/include -I${GMOCK_DIR} -c ${GMOCK_DIR}/src/gmock-all.cc
	ar -rv libgmock.a gtest-all.o gmock-all.o

clean:
	rm -f *.o
	rm -f *~
