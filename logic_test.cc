#include "sg_logic.hpp"
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "sg_objects.hpp"
#include "obj_eval.hpp"
#include <assert.h>

#define NEVER_REACH EXPECT_TRUE(false)

using ::testing::Return;
using ::testing::ReturnRef;
using ::testing::ByRef;
using ::testing::StrictMock;

class mock_session{
public:
	typedef const std::string op;
	MOCK_METHOD2(notify,void(op& name,const msgpack::object&));
	MOCK_METHOD2(notify,void(op& name,const key&));
	MOCK_METHOD3(notify,void(op& name,const key&,const value&));
	MOCK_METHOD4(notify,void(op& name,const key&,int,host));
	MOCK_METHOD5(notify,void(op& name,const key&,const key&,const host&, const membership_vector&));
};

class mock_server{
public:
	MOCK_METHOD1(get_session,mock_session&(const msgpack::rpc::address&));
	typedef mock_session session;
};

class mock_request{
public:
	MOCK_METHOD1(result, void(const msgpack::object& res));
	/*
	MOCK_METHOD2(result, void(const msgpack::object& res,
			msgpack::rpc::auto_zone z));
	MOCK_METHOD2(result, void(const msgpack::object& res,
			msgpack::rpc::shared_zone z));
	*/
	MOCK_METHOD0(params, const msgpack::object&());
};

// ordinary lib
TEST(obj, host2addr){
	using namespace msgpack::rpc;
	host h("102.21.32.1",43);
	ip_address a = h.get_address();
	host i = host::get_host(a);
	EXPECT_TRUE(h == i);
	EXPECT_TRUE(std::string("102.21.32.1") == i.hostname);
	EXPECT_EQ(i.port, 43);
}

// skipgraph logic
TEST(set, firstkey){
	shared_data::instance().init();
	
	eval::object<key,value> obj("k1","v1");
	const msgpack::object param = obj.get();

	StrictMock<mock_server> sv;
	StrictMock<mock_session> sn;
	StrictMock<mock_request> req;
	EXPECT_CALL(req, params())
		.WillOnce(ReturnRef(param));
	
	// call
	logic::set(&req, &sv);
	// check
	shared_data::ref_storage st(shared_data::instance().storage);
	EXPECT_TRUE(st->find("k1") != st->end());
}

/*
TEST(set, secondkey){
	msg::set op("k2","v2");
	msgpack::zone z;
	msgpack::object obj;
	op.msgpack_object(&obj,&z);
	StrictMock<mock_server> sv;
	mock_session sn;
	
	// call
	logic::set(&obj, &sv);
	// check
	shared_data::ref_storage st(shared_data::instance().storage);
	EXPECT_TRUE(st->find("k1") != st->end());
}

TEST(search, not_found){
	msg::search op("notexist", 8, host("127.0.0.1",9080));
	msgpack::zone z;
	msgpack::object obj;
	op.msgpack_object(&obj,&z);

	// expect
	mock_server sv;
	mock_session sn;
	EXPECT_CALL(sn, notify(std::string("notfound"), key("notexist")));
	EXPECT_CALL(sv, get_sessiopppn(msgpack::rpc::ip_address("127.0.0.1",9080)))
		.Times(1)
		.WillOnce(ReturnRef(sn));
	
	// calld
	logic::search(&obj, &sv);
}

TEST(search, will_find){
	msg::search op("k3", 8, host("127.0.0.1",9080));
	msgpack::zone z;
	msgpack::object obj;
	op.msgpack_object(&obj,&z);

	// expect
	mock_session sn;
	mock_server sv;
	EXPECT_CALL(sn, notify(std::string("found"), key("k3"), value("set")));
	EXPECT_CALL(sv, get_session(msgpack::rpc::ip_address("127.0.0.1",9080)))
		.Times(1)
		.WillOnce(ReturnRef(sn));
	
	msg::set set_op("k3","set");
	msgpack::object setter;
	set_op.msgpack_object(&setter,&z);
	logic::set(&setter,&sv);
	
	// call
	logic::search(&obj, &sv);
}
TEST(link, all_level_equal){
	shared_data::instance().init();
	mock_server sv;
	{ // set "a"
		msg::set set_op("a","v");
		msgpack::object setter;
		msgpack::zone z;
		set_op.msgpack_object(&setter,&z);
		logic::set(setter,&sv);
	}
	{ // set "b"
		msg::set set_op("b","v");
		msgpack::object setter;
		msgpack::zone z;
		set_op.msgpack_object(&setter,&z);
		logic::set(setter,&sv);
	}
	{ // expect seted
		shared_data::ref_storage st(shared_data::instance().storage);
		EXPECT_TRUE(st->find("a") != st->end());
		EXPECT_TRUE(st->find("b") != st->end());
		sg_node& a_node = st->find("a")->second;
		sg_node& b_node = st->find("b")->second;
		shared_data::ref_ng_map rn(shared_data::instance().ngmap);
		EXPECT_TRUE(rn->find("a") != rn->end());
		EXPECT_TRUE(rn->find("b") != rn->end());
		boost::shared_ptr<neighbor> neighbor = rn->find("a")->second.lock();
		assert(!!neighbor);
	}
	
	
}

*/
