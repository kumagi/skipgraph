#include "sg_logic.hpp"
#include <gtest/gtest.h>
#include <assert.h>

TEST(reflection,die){
	reflection ref_table;
	ref_table.regist("die",&msg::die);
	msgpack::object obj;
	ref_table.call("die",obj);
	assert(false); // never reach here
}

struct expect_equal_object{
	expect_equal_object(const msgpack::object& obj_):obj(obj_){}
	void operator()(const msgpack::object& obj)const{
		assert(false);
	}
	msgpack::object obj;
private:
	expect_equal_object();
};

TEST(reflection,test2){
	reflection ref_table;
	const msgpack::object obj;
	ref_table.regist("hoge", expect_equal_object(obj));
}
