
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "sg_logic.hpp"
#include "sg_objects.hpp"
#include "obj_eval.hpp"
#include <assert.h>

#define NEVER_REACH EXPECT_TRUE(false)

using ::testing::Return;
using ::testing::ReturnRef;
using ::testing::StrictMock;
using ::testing::AtLeast;
using ::testing::_;

class mock_session{
public:
	typedef const std::string op;

	// die
	// MOCK_METHOD2(notify,void(op&,const msgpack::object&));
	// not found
	MOCK_METHOD2(notify,void(op&,const key&));
	// set, found
	MOCK_METHOD3(notify,void(op&,const key&,const value&));
	// search
	MOCK_METHOD4(notify,void(op&,const key&,int,const host&));
	// treat
	MOCK_METHOD4(notify,void(op&,const key&,const host&, const membership_vector&));
	// link
	MOCK_METHOD5(notify,void(op&,const key&,int,const key&, const host&));
	// introduce
	MOCK_METHOD6(notify,void(op&,const key&,const key&, const host&, const membership_vector&, int));
	
	// die
	// MOCK_METHOD2(call,void(op&,const msgpack::object&));
	// not found
	MOCK_METHOD2(call,void(op&,const key&));
	// set, found
	MOCK_METHOD3(call,void(op&,const key&,const value&));
	// search
	MOCK_METHOD4(call,void(op&,const key&,int,const host&));
	// treat
	MOCK_METHOD4(call,void(op&,const key&,const host&, const membership_vector&));
	// link
	MOCK_METHOD5(call,void(op&,const key&,int,const key&, const host&));
	// introduce
	MOCK_METHOD6(call,void(op&,const key&,const key&, const host&, const membership_vector&, int));
	
};

class mock_server{
public:
	mock_session invalid_session;
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

class mock_host{
public:
	MOCK_METHOD0(get_address, msgpack::rpc::address());
	
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
TEST(_0, settings){
	shared_data::instance().init();
	shared_data::instance().myvector = membership_vector(0);
	shared_data::instance().set_host("127.0.0.1", 11211);
}
// alias
const host& localhost(shared_data::instance().get_host());
const host mockhost("133.21.1.1",11212); // k2
const host mockhost2("213.232.121.12",1443); // k2.5
const membership_vector mockvector1(2);
const membership_vector mockvector2(6);

TEST(_1, set_first_key){
// k1  
// k1
// k1
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
		shared_data::storage_t& st = shared_data::instance().storage.get_ref();		
		EXPECT_TRUE(st.contains("k1"));
		EXPECT_FALSE(st.contains("k0"));
		EXPECT_FALSE(st.contains("k2"));
	}
}

TEST(_2, treat_to_firstkey){
// k1  
// k1  [k2]
// k1  [k2]<=new!
	assert(get_direction("k1", "k2")==right);
	// request's input/output
	// treat operation
	eval::object<key,host,membership_vector>
		obj("k2",mockhost, mockvector1);
	
	StrictMock<mock_request> req;
	EXPECT_CALL(req, params())
		.WillOnce(ReturnRef(obj.get()));
	EXPECT_CALL(req, result(true));
	
	StrictMock<mock_session> ssn;
	EXPECT_CALL(ssn, call("link", "k2", 0, "k1", localhost));
	EXPECT_CALL(ssn, call("link", "k2", 1, "k1", localhost));

	StrictMock<mock_server> sv;
	EXPECT_CALL(sv, get_session(mockhost.get_address()))
		.WillRepeatedly(ReturnRef(ssn));
	
	// call
	logic::treat(&req, &sv);

	// check
	//STORAGE_DUMP;
	{
		shared_data::storage_t& st = shared_data::instance().storage.get_ref();		
		EXPECT_TRUE(st.get("k1")->second.neighbors()[right][0]);
		EXPECT_TRUE(st.get("k1")->second.neighbors()[right][1]);
		EXPECT_TRUE(st.get("k1")->second.neighbors()[right][0]->get_key() == "k2");
		EXPECT_TRUE(st.get("k1")->second.neighbors()[right][1]->get_key() == "k2");
		EXPECT_TRUE(st.get("k1")->second.neighbors()[right][0]->get_host() == mockhost);
		EXPECT_TRUE(st.get("k1")->second.neighbors()[right][1]->get_host() == mockhost);
		EXPECT_TRUE(!st.get("k1")->second.neighbors()[right][2]);
	}
}

TEST(_3, more_new_key){
// k1   k2 is other node's key
// k1  [k2] 
// k1  [k2]  k3<=new!

	
	// request's input/output
	eval::object<key,value> obj("k3","v3");
	StrictMock<mock_request> req;
	EXPECT_CALL(req, params())
		.WillOnce(ReturnRef(obj.get()));
	EXPECT_CALL(req, result(true));

	// set [k2] as other node's key
	const membership_vector& localvector = shared_data::instance().myvector;

	StrictMock<mock_session> sn;
	EXPECT_CALL(sn, call
		(std::string("treat"), "k3", localhost, localvector));
	StrictMock<mock_server> sv;
	EXPECT_CALL(sv, get_session(mockhost.get_address()))
		.WillRepeatedly(ReturnRef(sn));
	
	// call
	logic::set(&req, &sv);
	{
		shared_data::storage_t& st = shared_data::instance().storage.get_ref();		
		EXPECT_TRUE(st.get("k1") != st.end());
		EXPECT_TRUE(st.get("k3") != st.end());
	}
}

TEST(_4, link_to_newkey){
//		k1   k2 is other node's key
//		k1  [k2]->k3
//		k1  [k2]->k3
	// request's input/output
	eval::object<const key,int,const key,const host> obj1("k3",0,"k2",mockhost);
	eval::object<const key,int,const key,const host> obj2("k3",1,"k2",mockhost);
	StrictMock<mock_request> req1;
	EXPECT_CALL(req1, params())
		.WillOnce(ReturnRef(obj1.get()));
	StrictMock<mock_request> req2;
	EXPECT_CALL(req2, params())
		.WillOnce(ReturnRef(obj2.get()));
	
	mock_server sv;
	
	// call
	logic::link(&req1, &sv);
	logic::link(&req2, &sv);
	{
		shared_data::storage_t& st = shared_data::instance().storage.get_ref();
		EXPECT_TRUE(!st.get("k1")->second.neighbors()[left][0]);
		EXPECT_TRUE(!st.get("k1")->second.neighbors()[left][1]);
		EXPECT_TRUE(!st.get("k1")->second.neighbors()[left][2]);
		EXPECT_TRUE(st.get("k1")->second.neighbors()[right][0]->get_key() == "k2");
		EXPECT_TRUE(st.get("k1")->second.neighbors()[right][1]->get_key() == "k2");
		EXPECT_TRUE(st.get("k1")->second.neighbors()[right][0]->get_host() == mockhost);
		EXPECT_TRUE(st.get("k1")->second.neighbors()[right][1]->get_host() == mockhost);
		EXPECT_TRUE(!st.get("k1")->second.neighbors()[right][2]);
		
		EXPECT_TRUE(!st.get("k3")->second.neighbors()[right][0]);
		EXPECT_TRUE(!st.get("k3")->second.neighbors()[right][1]);
		EXPECT_TRUE(st.get("k3")->second.neighbors()[left][0]->get_key() == "k2");
		EXPECT_TRUE(st.get("k3")->second.neighbors()[left][1]->get_key() == "k2");
		EXPECT_TRUE(st.get("k3")->second.neighbors()[left][0]->get_host() == mockhost);
		EXPECT_TRUE(st.get("k3")->second.neighbors()[left][1]->get_host() == mockhost);
		EXPECT_TRUE(!st.get("k3")->second.neighbors()[left][2]);
	}
}

TEST(_5, more_key_in_the_middle){
// k1
// k1  [k2]  new  k3
// k1  [k2] k2.2  k3

	// request's input/output
	eval::object<key,value> obj("k2.2","v2.2");
	StrictMock<mock_request> req;
	EXPECT_CALL(req, params())
		.WillOnce(ReturnRef(obj.get()));
	EXPECT_CALL(req, result(true));
	
	const membership_vector& localvector = shared_data::instance().myvector;

	StrictMock<mock_session> sn;
	EXPECT_CALL(sn, call
		("introduce", "k2.2", "k2", localhost, localvector, 0));
	StrictMock<mock_server> sv;
	EXPECT_CALL(sv, get_session(mockhost.get_address()))
		.WillRepeatedly(ReturnRef(sn));
	// call
	logic::set(&req, &sv);
	{
		shared_data::storage_t& st = shared_data::instance().storage.get_ref();
		EXPECT_TRUE(!st.get("k3")->second.neighbors()[right][0]);
		EXPECT_TRUE(!st.get("k3")->second.neighbors()[right][1]);
		EXPECT_TRUE(st.get("k3")->second.neighbors()[left][0]->get_key() == "k2");
		EXPECT_TRUE(st.get("k3")->second.neighbors()[left][1]->get_key() == "k2");
		EXPECT_TRUE(st.get("k3")->second.neighbors()[left][0]->get_host() == mockhost);
		EXPECT_TRUE(st.get("k3")->second.neighbors()[left][1]->get_host() == mockhost);
		EXPECT_TRUE(!st.get("k3")->second.neighbors()[left][2]);
	}
}

TEST(_6, link_to_the_middle_key){
		// k1
		// k1  [k2]->k2.2 k3
		// k1  [k2]->k2.2 k3

	// request's input/output
	eval::object<const key,int,const key,const host> obj1("k2.2",0,"k2",mockhost);
	eval::object<const key,int,const key,const host> obj2("k2.2",1,"k2",mockhost);
	StrictMock<mock_request> req1;
	EXPECT_CALL(req1, params())
		.WillOnce(ReturnRef(obj1.get()));
	StrictMock<mock_request> req2;
	EXPECT_CALL(req2, params())
		.WillOnce(ReturnRef(obj2.get()));
	
	StrictMock<mock_server> sv;
	
	// call
	logic::link(&req1, &sv);
	logic::link(&req2, &sv);
	{
		shared_data::storage_t& st = shared_data::instance().storage.get_ref();
		EXPECT_TRUE(st.get("k2.2")->second.neighbors()[left][0]);
		EXPECT_TRUE(st.get("k2.2")->second.neighbors()[left][1]);
		EXPECT_TRUE(st.get("k2.2")->second.neighbors()[left][0]->get_key() == "k2");
		EXPECT_TRUE(st.get("k2.2")->second.neighbors()[left][1]->get_host() == mockhost);
		EXPECT_TRUE(!st.get("k2.2")->second.neighbors()[left][2]);
	}
}

TEST(_7, expect_introduce){
		// k1       k2.2  new!  k3
		// k1  [k2] k2.2 [k2.5] k3
		// k1  [k2] k2.2 [k2.5] k3
	
	EXPECT_TRUE(logic::detail::left_is_near("k3","k2.5","k2.2"));
	EXPECT_TRUE(get_direction("k3", "k2.5") == left);
	// request's input/output
	eval::object<const key,const host, const membership_vector>
		obj("k2.5",mockhost2,mockvector2);
	StrictMock<mock_request> req1;
	EXPECT_CALL(req1, params())
		.WillOnce(ReturnRef(obj.get()));
	EXPECT_CALL(req1, result(true));

	StrictMock<mock_server> sv;
	
	StrictMock<mock_session> sn1;
	EXPECT_CALL(sn1, call("link", "k2.5", 0, "k3", localhost));
	EXPECT_CALL(sn1, call("link", "k2.5", 1, "k3", localhost));
	EXPECT_CALL(sv, get_session(mockhost2.get_address()))
		.WillRepeatedly(ReturnRef(sn1));
	
	StrictMock<mock_session> sn2;
	EXPECT_CALL(sn2, call("introduce", "k2.5", "k2", mockhost2, mockvector2, 0));
	EXPECT_CALL(sv, get_session(mockhost.get_address()))
		.WillRepeatedly(ReturnRef(sn2));

	// call
	logic::treat(&req1, &sv);
	{
		shared_data::storage_t& st = shared_data::instance().storage.get_ref();
		EXPECT_TRUE(!st.get("k3")->second.neighbors()[right][0]);
		EXPECT_TRUE(!st.get("k3")->second.neighbors()[right][1]);
		EXPECT_TRUE(!st.get("k3")->second.neighbors()[right][2]);
		EXPECT_TRUE(st.get("k3")->second.neighbors()[left][0]->get_key() == "k2.5");
		EXPECT_TRUE(st.get("k3")->second.neighbors()[left][1]->get_key() == "k2.5");
		EXPECT_TRUE(!st.get("k3")->second.neighbors()[left][2]);

		EXPECT_TRUE(st.get("k2.2")->second.neighbors()[left][0]);
		EXPECT_TRUE(st.get("k2.2")->second.neighbors()[left][1]);
		EXPECT_TRUE(st.get("k2.2")->second.neighbors()[left][0]->get_key() == "k2");
		EXPECT_TRUE(st.get("k2.2")->second.neighbors()[left][1]->get_key() == "k2");
		EXPECT_TRUE(st.get("k2.2")->second.neighbors()[left][0]->get_host() == mockhost);
		EXPECT_TRUE(st.get("k2.2")->second.neighbors()[left][1]->get_host() == mockhost);
	}
}

TEST(_8, introduce_key){
		// k1       k2.2         k3
		// k1  [k2] k2.2<-[k2.5] k3
		// k1  [k2] k2.2<-[k2.5] k3
	// request's input/output
	eval::object<const key,const key,const host, const membership_vector,int>
		obj("k2.5","k2.2",mockhost2,mockvector2,0); // 1 bit match
	StrictMock<mock_request> req;
	EXPECT_CALL(req, params())
		.WillOnce(ReturnRef(obj.get()));
	EXPECT_CALL(req, result(true));

	StrictMock<mock_server> sv;
	StrictMock<mock_session> sn1;
	EXPECT_CALL(sn1, call("introduce", "k2.5", "k2", mockhost2, mockvector2, 2));
	EXPECT_CALL(sv, get_session(mockhost.get_address()))
		.WillRepeatedly(ReturnRef(sn1));
	StrictMock<mock_session> sn2;
	EXPECT_CALL(sn2, call("link", "k2.5", 0, "k2.2", localhost));
	EXPECT_CALL(sn2, call("link", "k2.5", 1, "k2.2", localhost));
	EXPECT_CALL(sv, get_session(mockhost2.get_address()))
		.WillRepeatedly(ReturnRef(sn2));
	
	// call
	logic::introduce(&req, &sv);
	{
		shared_data::storage_t& st = shared_data::instance().storage.get_ref();
		EXPECT_TRUE(!st.get("k3")->second.neighbors()[right][0]);
		EXPECT_TRUE(!st.get("k3")->second.neighbors()[right][1]);
		EXPECT_TRUE(!st.get("k3")->second.neighbors()[right][2]);
		EXPECT_TRUE(st.get("k3")->second.neighbors()[left][0]);
		EXPECT_TRUE(st.get("k3")->second.neighbors()[left][1]);
		EXPECT_TRUE(st.get("k3")->second.neighbors()[left][0]->get_key() == "k2.5");
		EXPECT_TRUE(st.get("k3")->second.neighbors()[left][1]->get_key() == "k2.5");
		EXPECT_TRUE(!st.get("k3")->second.neighbors()[left][2]);
		
		EXPECT_TRUE(st.get("k2.2")->second.neighbors()[right][0]);
		EXPECT_TRUE(st.get("k2.2")->second.neighbors()[right][1]);
		DEBUG_OUT("key %s \n", st.get("k2.2")->second.neighbors()[right][1]->get_key().c_str() );
		EXPECT_TRUE(st.get("k2.2")->second.neighbors()[right][0]->get_key() == "k2.5");
		EXPECT_TRUE(st.get("k2.2")->second.neighbors()[right][1]->get_key() == "k2.5");
		EXPECT_TRUE(st.get("k2.2")->second.neighbors()[right][0]->get_host() == mockhost2);
		EXPECT_TRUE(st.get("k2.2")->second.neighbors()[right][1]->get_host() == mockhost2);
	}
}

TEST(_9, add_more_key){
	//                 new
	// k1       k2.2  k2.3         k3
	// k1  [k2] k2.2  k2.3  [k2.5] k3
	// k1  [k2] k2.2  k2.3  [k2.5] k3
	
	// request's input/output
	eval::object<const key,const value>
		obj("k2.3","v2.3");
	StrictMock<mock_request> req;
	EXPECT_CALL(req, params())
		.WillOnce(ReturnRef(obj.get()));
	EXPECT_CALL(req, result(true));

	mock_server sv;
	mock_session sn1;
	EXPECT_CALL(sn1, call("introduce", "k2.3", "k2.5", 
			localhost, shared_data::instance().myvector, 0));
	EXPECT_CALL(sv, get_session(mockhost2.get_address()))
		.WillRepeatedly(ReturnRef(sn1));
	
	// call
	logic::set(&req, &sv);
	{
		shared_data::storage_t& st = shared_data::instance().storage.get_ref();
		
		EXPECT_TRUE(!st.get("k3")->second.neighbors()[right][0]);
		EXPECT_TRUE(!st.get("k3")->second.neighbors()[right][1]);
		EXPECT_TRUE(!st.get("k3")->second.neighbors()[right][2]);
		EXPECT_TRUE(st.get("k3")->second.neighbors()[left][0]);
		EXPECT_TRUE(st.get("k3")->second.neighbors()[left][1]);
		EXPECT_TRUE(st.get("k3")->second.neighbors()[left][0]->get_key() == "k2.5");
		EXPECT_TRUE(st.get("k3")->second.neighbors()[left][1]->get_key() == "k2.5");
		EXPECT_TRUE(!st.get("k3")->second.neighbors()[left][2]);
		
		EXPECT_TRUE(st.get("k2.2")->second.neighbors()[right][0]);
		EXPECT_TRUE(st.get("k2.2")->second.neighbors()[right][1]);
		EXPECT_TRUE(st.get("k2.2")->second.neighbors()[right][0]->get_key() == "k2.3");
		EXPECT_TRUE(st.get("k2.2")->second.neighbors()[right][1]->get_key() == "k2.3");
		EXPECT_TRUE(st.get("k2.2")->second.neighbors()[right][0]->get_host() == localhost);
		EXPECT_TRUE(st.get("k2.2")->second.neighbors()[right][1]->get_host() == localhost);
		
		EXPECT_TRUE(st.get("k2.3")->second.neighbors()[left][0]->get_key() == "k2.2");
		EXPECT_TRUE(st.get("k2.3")->second.neighbors()[left][1]->get_key() == "k2.2");
		EXPECT_TRUE(st.get("k2.3")->second.neighbors()[left][2]->get_key() == "k2.2");
		EXPECT_TRUE(st.get("k2.3")->second.neighbors()[left][0]->get_host() == localhost);
		EXPECT_TRUE(st.get("k2.3")->second.neighbors()[left][1]->get_host() == localhost);
		EXPECT_TRUE(st.get("k2.3")->second.neighbors()[left][2]->get_host() == localhost);
		EXPECT_TRUE(!st.get("k2.3")->second.neighbors()[right][0]);
		EXPECT_TRUE(!st.get("k2.3")->second.neighbors()[right][1]);
		EXPECT_TRUE(!st.get("k2.3")->second.neighbors()[right][2]);
	}
}

TEST(_10, link_to_left_key){
	//                     link
	// k1       k2.2  k2.3         k3
	// k1  [k2] k2.2  k2.3  [k2.5] k3
	// k1  [k2] k2.2  k2.3  [k2.5] k3
	// request's input/output
	eval::object<const key,int,const key, host>
		obj("k2.3", 0, "k2.5", mockhost2);
	StrictMock<mock_request> req;
	EXPECT_CALL(req, params())
		.WillOnce(ReturnRef(obj.get()));
	
	mock_server sv;
	
	// call
	logic::link(&req, &sv);
	{
		shared_data::storage_t& st = shared_data::instance().storage.get_ref();
		EXPECT_TRUE(!st.get("k3")->second.neighbors()[right][0]);
		EXPECT_TRUE(!st.get("k3")->second.neighbors()[right][1]);
		EXPECT_TRUE(!st.get("k3")->second.neighbors()[right][2]);
		EXPECT_TRUE(st.get("k3")->second.neighbors()[left][0]);
		EXPECT_TRUE(st.get("k3")->second.neighbors()[left][1]);
		EXPECT_TRUE(st.get("k3")->second.neighbors()[left][0]->get_key() == "k2.5");
		EXPECT_TRUE(st.get("k3")->second.neighbors()[left][1]->get_key() == "k2.5");
		EXPECT_TRUE(!st.get("k3")->second.neighbors()[left][2]);
		
		EXPECT_TRUE(st.get("k2.2")->second.neighbors()[right][0]);
		EXPECT_TRUE(st.get("k2.2")->second.neighbors()[right][1]);
		EXPECT_TRUE(st.get("k2.2")->second.neighbors()[right][0]->get_key() == "k2.3");
		EXPECT_TRUE(st.get("k2.2")->second.neighbors()[right][1]->get_key() == "k2.3");
		EXPECT_TRUE(st.get("k2.2")->second.neighbors()[right][0]->get_host() == localhost);
		EXPECT_TRUE(st.get("k2.2")->second.neighbors()[right][1]->get_host() == localhost);
		
		EXPECT_TRUE(st.get("k2.3")->second.neighbors()[left][0]->get_key() == "k2.2");
		EXPECT_TRUE(st.get("k2.3")->second.neighbors()[left][1]->get_key() == "k2.2");
		EXPECT_TRUE(st.get("k2.3")->second.neighbors()[left][2]->get_key() == "k2.2");
		EXPECT_TRUE(st.get("k2.3")->second.neighbors()[right][0]);
		EXPECT_TRUE(st.get("k2.3")->second.neighbors()[right][0]->get_key() == "k2.5");
		EXPECT_TRUE(st.get("k2.3")->second.neighbors()[right][0]->get_host() == mockhost2);
		EXPECT_TRUE(!st.get("k2.3")->second.neighbors()[right][1]);
		EXPECT_TRUE(!st.get("k2.3")->second.neighbors()[right][2]);
	}
}

TEST(_11, introduce_to_left_key){
//           introduce!
		// k1       k2.2  k2.3         k3
		// k1  [k2] k2.2  k2.3  [k2.5] k3
		// k1  [k2] k2.2  k2.3  [k2.5] k3

		// request's input/output
	EXPECT_TRUE(logic::detail::left_is_near("k2.2","k2.3","k2.5"));
	eval::object<const key,const key, host, membership_vector, int >
		obj("k2.2", "k2.3",
			localhost, shared_data::instance().myvector, 1);
	StrictMock<mock_request> req;
	EXPECT_CALL(req, params())
		.WillOnce(ReturnRef(obj.get()));
	EXPECT_CALL(req, result(true));
	
	mock_server sv;
	
	// call
	logic::introduce(&req, &sv);
	{
		shared_data::storage_t& st = shared_data::instance().storage.get_ref();

		EXPECT_TRUE(!st.get("k3")->second.neighbors()[right][0]);
		EXPECT_TRUE(!st.get("k3")->second.neighbors()[right][1]);
		EXPECT_TRUE(!st.get("k3")->second.neighbors()[right][2]);
		EXPECT_TRUE(st.get("k3")->second.neighbors()[left][0]);
		EXPECT_TRUE(st.get("k3")->second.neighbors()[left][1]);
		EXPECT_TRUE(st.get("k3")->second.neighbors()[left][0]->get_key() == "k2.5");
		EXPECT_TRUE(st.get("k3")->second.neighbors()[left][1]->get_key() == "k2.5");
		EXPECT_TRUE(!st.get("k3")->second.neighbors()[left][2]);
		
		EXPECT_TRUE(st.get("k2.2")->second.neighbors()[right][0]);
		EXPECT_TRUE(st.get("k2.2")->second.neighbors()[right][1]);
		EXPECT_TRUE(st.get("k2.2")->second.neighbors()[right][0]->get_key() == "k2.3");
		EXPECT_TRUE(st.get("k2.2")->second.neighbors()[right][1]->get_key() == "k2.3");
		EXPECT_TRUE(st.get("k2.2")->second.neighbors()[right][0]->get_host() == localhost);
		EXPECT_TRUE(st.get("k2.2")->second.neighbors()[right][1]->get_host() == localhost);
		
		EXPECT_TRUE(st.get("k2.3")->second.neighbors()[left][0]->get_key() == "k2.2");
		EXPECT_TRUE(st.get("k2.3")->second.neighbors()[left][1]->get_key() == "k2.2");
		EXPECT_TRUE(st.get("k2.3")->second.neighbors()[left][2]->get_key() == "k2.2");
		EXPECT_TRUE(st.get("k2.3")->second.neighbors()[right][0]);
		EXPECT_TRUE(st.get("k2.3")->second.neighbors()[right][0]->get_key() == "k2.5");
		EXPECT_TRUE(st.get("k2.3")->second.neighbors()[right][0]->get_host() == mockhost2);
		EXPECT_TRUE(!st.get("k2.3")->second.neighbors()[right][1]);
		EXPECT_TRUE(!st.get("k2.3")->second.neighbors()[right][2]);
	}
}

TEST(_12, introduce_to_right_key){
	//                    introduce!
	// k1       k2.2  k2.3         k3
	// k1  [k2] k2.2  k2.3  [k2.5] k3
	// k1  [k2] k2.2  k2.3  [k2.5] k3
	
	// request's input/output
	eval::object<const key,const key, host, membership_vector, int >
		obj("k2.3", "k3",
				localhost, shared_data::instance().myvector, 1);
	StrictMock<mock_request> req;
	EXPECT_CALL(req, params())
		.WillOnce(ReturnRef(obj.get()));
	EXPECT_CALL(req, result(true));
	
	mock_server sv;
	
	// call
	logic::introduce(&req, &sv);
	{
		shared_data::storage_t& st = shared_data::instance().storage.get_ref();
		EXPECT_TRUE(!st.get("k3")->second.neighbors()[right][0]);
		EXPECT_TRUE(!st.get("k3")->second.neighbors()[right][1]);
		EXPECT_TRUE(!st.get("k3")->second.neighbors()[right][2]);
		EXPECT_TRUE(st.get("k3")->second.neighbors()[left][0]);
		EXPECT_TRUE(st.get("k3")->second.neighbors()[left][1]);
		EXPECT_TRUE(st.get("k3")->second.neighbors()[left][0]->get_key() == "k2.5");
		EXPECT_TRUE(st.get("k3")->second.neighbors()[left][1]->get_key() == "k2.3");
		EXPECT_TRUE(st.get("k3")->second.neighbors()[left][2]->get_key() == "k2.3");
		
		EXPECT_TRUE(st.get("k2.2")->second.neighbors()[right][0]);
		EXPECT_TRUE(st.get("k2.2")->second.neighbors()[right][1]);
		EXPECT_TRUE(st.get("k2.2")->second.neighbors()[right][0]->get_key() == "k2.3");
		EXPECT_TRUE(st.get("k2.2")->second.neighbors()[right][1]->get_key() == "k2.3");
		EXPECT_TRUE(st.get("k2.2")->second.neighbors()[right][0]->get_host() == localhost);
		EXPECT_TRUE(st.get("k2.2")->second.neighbors()[right][1]->get_host() == localhost);
		
		
		EXPECT_TRUE(st.get("k2.3")->second.neighbors()[left][0]->get_key() == "k2.2");
		EXPECT_TRUE(st.get("k2.3")->second.neighbors()[left][1]->get_key() == "k2.2");
		EXPECT_TRUE(st.get("k2.3")->second.neighbors()[left][2]->get_key() == "k2.2");
		EXPECT_TRUE(st.get("k2.3")->second.neighbors()[right][0]);
		EXPECT_TRUE(st.get("k2.3")->second.neighbors()[right][0]->get_key() == "k2.5");
		EXPECT_TRUE(st.get("k2.3")->second.neighbors()[right][0]->get_host() == mockhost2);
		EXPECT_TRUE(st.get("k2.3")->second.neighbors()[right][1]->get_key() == "k3");
		EXPECT_TRUE(st.get("k2.3")->second.neighbors()[right][2]->get_key() == "k3");
	}
}

const host mockhost3("1.2.3.4",121);
const membership_vector mockvector3(1);

TEST(_13, more_middle_key){
		// k1       k2.2           k2.3         k3
		// k1  [k2] k2.2   new!    k2.3  [k2.5] k3
		// k1  [k2] k2.2  [k2.22]  k2.3  [k2.5] k3
	EXPECT_TRUE(logic::detail::left_is_near("k2.2","k2.22","k2.3"));
	EXPECT_TRUE(logic::detail::left_is_near("k2.3","k2.22","k2.2"));
	// request's input/output
	eval::object<key,host,membership_vector>
		obj("k2.22",mockhost3, mockvector3);
	StrictMock<mock_request> req;
	EXPECT_CALL(req, params())
		.WillOnce(ReturnRef(obj.get()));
	EXPECT_CALL(req, result(true));

	StrictMock<mock_server> sv;

	StrictMock<mock_session> sn1;
	EXPECT_CALL(sn1, call
		("link", "k2.22", 0, "k2.3", localhost));
	EXPECT_CALL(sn1, call
		("link", "k2.22", 0, "k2.2", localhost));
	EXPECT_CALL(sv, get_session(mockhost3.get_address()))
		.WillRepeatedly(ReturnRef(sn1));
	
	StrictMock<mock_session> sn3;
	EXPECT_CALL(sn3, call
		("introduce", "k2.22", "k2.5", mockhost3, mockvector3, 1));
	EXPECT_CALL(sv, get_session(mockhost2.get_address()))
		.WillRepeatedly(ReturnRef(sn3));
	
	StrictMock<mock_session> sn4;
	EXPECT_CALL(sn3, call
		("introduce", "k2.22", "k2", mockhost3, mockvector3, 1));
	EXPECT_CALL(sv, get_session(mockhost.get_address()))
		.WillRepeatedly(ReturnRef(sn3));
	
	// call
	logic::treat(&req, &sv);
}

TEST(_14, introduce_middle_key_to_left){
		// k1       k2.2           k2.3         k3
		// k1  [k2] k2.2           k2.3  [k2.5] k3
		// k1  [k2] k2.2<-[k2.22]  k2.3  [k2.5] k3
	EXPECT_TRUE(logic::detail::left_is_near("k2.2","k2.22","k2.3"));
	EXPECT_TRUE(logic::detail::left_is_near("k2.3","k2.22","k2.2"));
	// request's input/output
	eval::object<key,key,host,membership_vector,int>
		obj("k2.22", "k2.2",mockhost3, mockvector3, 0);
	StrictMock<mock_request> req;
	EXPECT_CALL(req, params())
		.WillOnce(ReturnRef(obj.get()));
	EXPECT_CALL(req, result(true));

	StrictMock<mock_server> sv;
	ON_CALL(sv, get_session(_)).
		WillByDefault(ReturnRef(sv.invalid_session));

	StrictMock<mock_session> sn1;
	EXPECT_CALL(sn1, call
		("link", "k2.22", 0, "k2.2", localhost));
	EXPECT_CALL(sv, get_session(mockhost3.get_address()))
		.WillRepeatedly(ReturnRef(sn1));
	
	StrictMock<mock_session> sn2;
	EXPECT_CALL(sn2, call
		("introduce", "k2.22", "k2", mockhost3, mockvector3, 1));
	EXPECT_CALL(sv, get_session(mockhost.get_address()))
		.WillRepeatedly(ReturnRef(sn2));
	
	// call
	logic::introduce(&req, &sv);
	{
		shared_data::storage_t& st = shared_data::instance().storage.get_ref();
		EXPECT_TRUE(st.get("k2.2")->second.neighbors()[right][0]->get_key() == "k2.22");
	}
}

TEST(_15, introduce_middle_key_to_right){
		// k1       k2.2           k2.3         k3
		// k1  [k2] k2.2           k2.3  [k2.5] k3
		// k1  [k2] k2.2  [k2.22]->k2.3  [k2.5] k3
	// request's input/output
	eval::object<key,key,host,membership_vector,int>
		obj("k2.22", "k2.3", mockhost3, mockvector3, 0);
	StrictMock<mock_request> req;
	EXPECT_CALL(req, params())
		.WillOnce(ReturnRef(obj.get()));
	EXPECT_CALL(req, result(true));

	StrictMock<mock_server> sv;
	StrictMock<mock_session> sn1;
	EXPECT_CALL(sn1, call
		("link", "k2.22", 0, "k2.3", localhost));
	EXPECT_CALL(sv, get_session(mockhost3.get_address()))
		.WillRepeatedly(ReturnRef(sn1));
	
	StrictMock<mock_session> sn2;
	EXPECT_CALL(sn2, call
		("introduce", "k2.22", "k2.5", mockhost3, mockvector3, 1));
	EXPECT_CALL(sv, get_session(mockhost2.get_address()))
		.WillRepeatedly(ReturnRef(sn2));
	
	// call
	logic::introduce(&req, &sv);
	{
		shared_data::storage_t& st = shared_data::instance().storage.get_ref();
		EXPECT_TRUE(st.get("k2.2")->second.neighbors()[right][0]->get_key() == "k2.22");
	}
}

TEST(_16, overwrite_old_key_in_other_node){
		// k1       k2.2         k2.3         k3
		// k1  [k2] k2.2         k2.3  [k2.5] k3
		// k1  [k2] k2.2  k2.22  k2.3  [k2.5] k3
	// request's input/output
	const membership_vector& localvector = shared_data::instance().myvector;
	
	eval::object<key,value> obj("k2.22", "k3");
	StrictMock<mock_request> req;
	EXPECT_CALL(req, params())
		.WillOnce(ReturnRef(obj.get()));
	EXPECT_CALL(req, result(true));

	
	StrictMock<mock_server> sv;

	StrictMock<mock_session> sn;

	EXPECT_CALL(sn, call("treat", "k2.22", localhost, localvector));
	EXPECT_CALL(sv, get_session(mockhost3.get_address()))
		.WillRepeatedly(ReturnRef(sn));
	// call
	logic::set(&req, &sv);
	{
		shared_data::storage_t& st = shared_data::instance().storage.get_ref();
		EXPECT_FALSE(st.get("k2.22")->second.neighbors()[right][0]);
		EXPECT_TRUE(st.get("k2.22")->second.neighbors()[left][0]->get_key() == "k2.2");
	}
}

TEST(_16, and_treat){
		// k1       k2.2         k2.3         k3
		// k1  [k2] k2.2         k2.3  [k2.5] k3
		// k1  [k2] k2.2  k2.22  k2.3  [k2.5] k3
	// request's input/output
	const membership_vector& localvector = shared_data::instance().myvector;
	
	eval::object<key,host,membership_vector>
		obj("k2.22",localhost, localvector);
	StrictMock<mock_request> req;
	EXPECT_CALL(req, params())
		.WillOnce(ReturnRef(obj.get()));
	EXPECT_CALL(req, result(true));

	
	StrictMock<mock_server> sv;

	StrictMock<mock_session> sn;

//	EXPECT_CALL(sn, call("introduce", "k2.22", "k2", localhost, localvector, ));
//	EXPECT_CALL(sv, get_session(mockhost.get_address()))
//		.WillRepeatedly(ReturnRef(sn));
	// call
	logic::treat(&req, &sv);
	{
		shared_data::storage_t& st = shared_data::instance().storage.get_ref();
		EXPECT_TRUE(st.get("k2.22")->second.neighbors()[right][0]);
		EXPECT_TRUE(st.get("k2.22")->second.neighbors()[left][0]);
		EXPECT_TRUE(st.get("k2.22")->second.neighbors()[right][0]->get_key() == "k2.3");
		EXPECT_TRUE(st.get("k2.22")->second.neighbors()[left][0]->get_key() == "k2.2");
	}
}

TEST(_17, more_overwrite_middle_key){
	// k1       k2.2           k2.3         k3
	// k1  [k2] k2.2   new!    k2.3  [k2.5] k3
	// k1  [k2] k2.2  [k2.22]  k2.3  [k2.5] k3

	EXPECT_TRUE(logic::detail::left_is_near("k2.2","k2.22","k2.3"));
	EXPECT_TRUE(logic::detail::left_is_near("k2.3","k2.22","k2.2"));
	// request's input/output
	eval::object<key,host,membership_vector>
		obj("k2.22",mockhost3, mockvector3);
	StrictMock<mock_request> req;
	EXPECT_CALL(req, params())
		.WillOnce(ReturnRef(obj.get()));
	EXPECT_CALL(req, result(true));

	StrictMock<mock_server> sv;
	
	StrictMock<mock_session> sn1,sn2,sn3;
	EXPECT_CALL(sn1, call
		("introduce", "k2.22", "k2.2", mockhost3, mockvector3, 0));
	EXPECT_CALL(sn1, call
		("introduce", "k2.22", "k2.3", mockhost3, mockvector3, 0));
	EXPECT_CALL(sv, get_session(localhost.get_address()))
		.WillRepeatedly(ReturnRef(sn1));
//	EXPECT_CALL(sn1, call
//		("introduce", "k2.22", "k2.3", mockhost3, mockvector3, 0));
	EXPECT_CALL(sv, get_session(mockhost.get_address()))
		.WillRepeatedly(ReturnRef(sn2));
	EXPECT_CALL(sv, get_session(mockhost3.get_address()))
		.WillRepeatedly(ReturnRef(sn3));
	
	// call
	logic::treat(&req, &sv);
}


const host search_origin("1.43.43.2", 48);
TEST(_18, search__will_find){
		// k1       k2.2           k2.3         k3
		// k1  [k2] k2.2           k2.3  [k2.5] k3
		// k1  [k2] k2.2  [k2.22]  k2.3  [k2.5] k3
	// request's input/output
	eval::object<key,int,host>
		obj("k2.2", 100, search_origin);
	StrictMock<mock_request> req;
	EXPECT_CALL(req, params())
		.WillOnce(ReturnRef(obj.get()));
	EXPECT_CALL(req, result(true));
	
	StrictMock<mock_server> sv;
	StrictMock<mock_session> sn;
	EXPECT_CALL(sn, call
		("found", "k2.2", "v2.2"));
	EXPECT_CALL(sv, get_session(search_origin.get_address()))
		.WillRepeatedly(ReturnRef(sn));
	
	// call
	logic::search(&req, &sv);
}


TEST(_18, search__will_relay){
//	k1       k2.2           k2.3         k3
//	k1  [k2] k2.2           k2.3  [k2.5] k3
//  k1  [k2] k2.2  [k2.22]  k2.3  [k2.5] k3
	// request's input/output
	eval::object<key,int,host>
		obj("k2.22", 100, search_origin);
	StrictMock<mock_request> req;
	EXPECT_CALL(req, params())
		.WillOnce(ReturnRef(obj.get()));
	EXPECT_CALL(req, result(true));
	
	StrictMock<mock_server> sv;
	StrictMock<mock_session> sn;
	EXPECT_CALL(sn, call("search", "k2.22", _, search_origin));
	EXPECT_CALL(sv, get_session(mockhost3.get_address()))
		.WillRepeatedly(ReturnRef(sn));
	
	// call
	logic::search(&req, &sv);
}


TEST(_18, search__will_not_found){
		// k1       k2.2           k2.3         k3
		// k1  [k2] k2.2           k2.3  [k2.5] k3
		// k1  [k2] k2.2  [k2.22]  k2.3  [k2.5] k3
	// request's input/output
	eval::object<key,int,host>
		obj("hoge", 100, search_origin);
	StrictMock<mock_request> req;
	EXPECT_CALL(req, params())
		.WillOnce(ReturnRef(obj.get()));
	EXPECT_CALL(req, result(true));
	
	StrictMock<mock_server> sv;
	StrictMock<mock_session> sn;
	EXPECT_CALL(sn, call("notfound", "hoge"));
	EXPECT_CALL(sv, get_session(search_origin.get_address()))
		.WillRepeatedly(ReturnRef(sn));
	
	// call
	logic::search(&req, &sv);
}

