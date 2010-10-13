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
	MOCK_METHOD2(notify,void(op&,const msgpack::object&));
	MOCK_METHOD2(notify,void(op&,const key&));
	MOCK_METHOD3(notify,void(op&,const key&,const value&));
	MOCK_METHOD4(notify,void(op&,const key&,int,host));
	MOCK_METHOD5(notify,void(op&,const key&,const key&,const host&, const membership_vector&));
	MOCK_METHOD5(notify,void(op&,const key&,int,const key&, const host&));
};

class mock_server{
public:
	MOCK_METHOD1(get_session,mock_session&(const msgpack::rpc::address&));
	typedef mock_session session;
};

class mock_request{
public:
	MOCK_METHOD0(zone, msgpack::rpc::auto_zone&() );
	MOCK_METHOD1(result, void(const msgpack::object& res));
	MOCK_METHOD2(result, void(const msgpack::object& res,
			msgpack::rpc::shared_zone z));
	MOCK_METHOD1(result, void(bool));

	MOCK_METHOD0(params, const msgpack::object&());
};

#define STORAGE_DUMP {shared_data::instance().storage_dump();}

/* ---------------
 * ordinary lib
 * ---------------
 */
TEST(obj, host2addr){
	using namespace msgpack::rpc;
	host h("102.21.32.1",43);
	ip_address a = h.get_address();
	host i = host::get_host(a);
	EXPECT_TRUE(h == i);
	EXPECT_TRUE(std::string("102.21.32.1") == i.hostname);
	EXPECT_EQ(i.port, 43);
}

/* ---------------
 * skipgraph logic
 * ---------------
 */

// alias
const host& myhost = shared_data::instance().get_host();

TEST(set, firstkey){
	shared_data::instance().init();
	
	eval::object<key,value> obj("k1","v1");
	StrictMock<mock_server> sv;
	StrictMock<mock_request> req;
	EXPECT_CALL(req, params())
		.WillOnce(ReturnRef(obj.get()));
	EXPECT_CALL(req, result(true));
	
	// call
	logic::set(&req, &sv);
	// check
	{
		shared_data::ref_storage st(shared_data::instance().storage);
		EXPECT_TRUE(st->find("k1") != st->end());
	}
	EXPECT_FALSE(logic::detail::get_nearest_node("k0") == NULL);
	EXPECT_FALSE(logic::detail::get_nearest_node("k2") == NULL);
}

const host mockhost("hogeIP",11212); //host
TEST(treat, to_firstkey){
	/*
		k1  
		k1  
		k1  [k2]<=new!
	 */
	
	// request's input/output
	// treat operation
	{
		shared_data::ref_storage st(shared_data::instance().storage);
		st->find("k1")->second.set_vector(membership_vector(1));
	}
	eval::object<key,key,host,membership_vector> obj("k2","k1",mockhost, membership_vector(3));
	StrictMock<mock_request> req;
	EXPECT_CALL(req, params())
		.WillOnce(ReturnRef(obj.get()));

}
/*
TEST(set, secondkey){
	
		k1  [k2] k2 is other node's key
		k1  [k2] 
		k1  [k2]  k3<=new!
 
	// request's input/output
	eval::object<key,value> obj("k3","v3");
	StrictMock<mock_request> req;
	EXPECT_CALL(req, params())
		.WillOnce(ReturnRef(obj.get()));
	EXPECT_CALL(req, result(true));

	// set [k2] as other node's key
	
	
	const host& h = shared_data::instance().get_host();
	
	StrictMock<mock_session> sn;	
	for(int i=0; i< maxlevel; i++){
		EXPECT_CALL(sn, notify(std::string("link"), key("k2"), i, "k1", h));
		EXPECT_CALL(sn, notify(std::string("link"), key("k2"), i, "k3", h));
	}

	
	StrictMock<mock_server> sv;
	
	// call
	logic::set(&req, &sv);
	// check
	{
		shared_data::ref_storage st(shared_data::instance().storage);
		EXPECT_TRUE(st->find("k1") != st->end());
		EXPECT_TRUE(st->find("k3") != st->end());
	}
}
*/
/*
TEST(set, thirdkey){


	eval::object<key,value> obj("k2","v2");
	

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
