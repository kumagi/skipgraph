CXX=g++
OPTS=-O0 -fexceptions -g
LD=-lboost_program_options -lmsgpack -lmpio -lmsgpack-rpc
TEST_LD= -lpthread $(LD)
GTEST_INC= -I$(GTEST_DIR)/include -I$(GTEST_DIR)
GTEST_DIR=/opt/google/gtest-1.5.0
WARNS= -W -Wall -Wextra -Wformat=2 -Wstrict-aliasing=4 -Wcast-qual -Wcast-align \
	-Wwrite-strings -Wfloat-equal -Wpointer-arith -Wswitch-enum
NOTIFY=&& notify-send Test success! -i ~/themes/ok_icon.png || notify-send Test failed... -i ~/themes/ng_icon.png
SRCS=$(HEADS) $(BODYS)

target:skipgraph
target:logic_test

skipgraph: skipgraph.o tcp_wrap.o sg_logic.o
	$(CXX) $^ -o $@ $(LD) $(OPT) $(WARNS)
logic_test: sg_logic.o logic_test.o gtest_main.a
	$(CXX) $^ -o $@ $(GTEST_INC) $(TEST_LD) $(OPT) $(WARNS)
	./logic_test $(NOTIFY)

logic_test.o:logic_test.cc
	$(CXX) -c $^ -o $@ $(OPT) $(WARNS)
sg_logic.o: sg_logic.cc sg_logic.hpp
	$(CXX) -c sg_logic.cc -o $@ $(OPT) $(WARNS)
skipgraph.o:skipgraph.cc skipgraph.h
	$(CXX) -c skipgraph.cc -o $@ $(OPT) $(WARNS)
tcp_wrap.o:tcp_wrap.cpp
	$(CXX) -c $^ -o $@ $(OPT) $(WARNS)

#%.o: %.c
#	$(CXX) -c $(OPTS) $(WARNS) $< -o $@

# gtest
gtest_main.o:
	$(CXX) $(GTEST_INC) -c $(OPTS) $(GTEST_DIR)/src/gtest_main.cc -o $@
gtest-all.o:
	$(CXX) $(GTEST_INC) -c $(OPTS) $(GTEST_DIR)/src/gtest-all.cc -o $@
gtest_main.a:gtest-all.o gtest_main.o
	ar -r $@ $^

clean:
	rm -f *.o
	rm -f *~
