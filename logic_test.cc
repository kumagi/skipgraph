
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
	MOCK_METHOD5(notify,void(op&,const key&,const key&,const host&, const membership_vector&));
	// link
	MOCK_METHOD5(notify,void(op&,const key&,int,const key&, const host&));
	// introduce
	MOCK_METHOD6(notify,void(op&,const key&,const key&, const host&, const membership_vector&, int));
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
const host mockhost("133.21.1.1",11212); //host
const host mockhost2("213.232.121.12",1443);
const membership_vector mockvector2(6);

TEST(_1, set_first_key){
	/*
		k1  
		k1
		k1
	 */
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


TEST(_2, treat_to_firstkey){
	/*
		k1  
		k1  [k2]
		k1  [k2]<=new!
	 */
	assert(get_direction("k1", "k2")==right);
	// request's input/output
	// treat operation
	eval::object<key,host,membership_vector>
		obj("k2",mockhost, membership_vector(2));
	
	StrictMock<mock_request> req;
	EXPECT_CALL(req, params())
		.WillOnce(ReturnRef(obj.get()));
	
	StrictMock<mock_session> ssn;
	EXPECT_CALL(ssn, notify("link", "k2", 0, "k1", localhost));
	EXPECT_CALL(ssn, notify("link", "k2", 1, "k1", localhost));

	StrictMock<mock_server> sv;
	EXPECT_CALL(sv, get_session(mockhost.get_address()))
		.WillRepeatedly(ReturnRef(ssn));
	
	// call
	logic::treat(&req, &sv);

	// check
	//STORAGE_DUMP;
	{
		shared_data::ref_storage st(shared_data::instance().storage);
		assert(st->find("k1")->second.neighbors()[right][0]);
		assert(st->find("k1")->second.neighbors()[right][1]);
		assert(st->find("k1")->second.neighbors()[right][0]->get_key() == "k2");
		assert(st->find("k1")->second.neighbors()[right][1]->get_key() == "k2");
		assert(st->find("k1")->second.neighbors()[right][0]->get_host() == mockhost);
		assert(st->find("k1")->second.neighbors()[right][1]->get_host() == mockhost);
		assert(!st->find("k1")->second.neighbors()[right][2]);
	}
}

TEST(_3, more_new_key){
	/*
		k1   k2 is other node's key
		k1  [k2] 
		k1  [k2]  k3<=new!
	*/
	// request's input/output
	eval::object<key,value> obj("k3","v3");
	StrictMock<mock_request> req;
	EXPECT_CALL(req, params())
		.WillOnce(ReturnRef(obj.get()));
	EXPECT_CALL(req, result(true));

	// set [k2] as other node's key
	const membership_vector& localvector = shared_data::instance().myvector;

	StrictMock<mock_session> sn;
	EXPECT_CALL(sn, notify
		(std::string("treat"), key("k3"), "k2", localhost, localvector));
	StrictMock<mock_server> sv;
	EXPECT_CALL(sv, get_session(mockhost.get_address()))
		.WillRepeatedly(ReturnRef(sn));
	
	// call
	logic::set(&req, &sv);
	{
		shared_data::ref_storage st(shared_data::instance().storage);
		EXPECT_TRUE(st->find("k1") != st->end());
		EXPECT_EQ(st->size() , 2U);
	}
}

TEST(_4, link_to_newkey){
	/*
		k1   k2 is other node's key
		k1  [k2]->k3
		k1  [k2]->k3
	*/
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
		shared_data::ref_storage st(shared_data::instance().storage);
		EXPECT_EQ(st->size(),2U);
		EXPECT_TRUE(!st->find("k3")->second.neighbors()[right][0]);
		EXPECT_TRUE(!st->find("k3")->second.neighbors()[right][1]);
		EXPECT_TRUE(st->find("k3")->second.neighbors()[left][0]->get_key() == "k2");
		EXPECT_TRUE(st->find("k3")->second.neighbors()[left][1]->get_key() == "k2");
		EXPECT_TRUE(st->find("k3")->second.neighbors()[left][0]->get_host() == mockhost);
		EXPECT_TRUE(st->find("k3")->second.neighbors()[left][1]->get_host() == mockhost);
		EXPECT_TRUE(!st->find("k3")->second.neighbors()[left][2]);
	}
}

TEST(_5, more_key_in_the_middle){
	/*
		k1
		k1  [k2]  new  k3
		k1  [k2] k2.2  k3
	*/
	// request's input/output
	eval::object<key,value> obj("k2.2","v2.2");
	StrictMock<mock_request> req;
	EXPECT_CALL(req, params())
		.WillOnce(ReturnRef(obj.get()));
	EXPECT_CALL(req, result(true));
	
	const membership_vector& localvector = shared_data::instance().myvector;

	StrictMock<mock_session> sn;
	EXPECT_CALL(sn, notify
		("treat", "k2.2", "k2", localhost, localvector));
	StrictMock<mock_server> sv;
	EXPECT_CALL(sv, get_session(mockhost.get_address()))
		.WillRepeatedly(ReturnRef(sn));
	// call
	logic::set(&req, &sv);
	{
		shared_data::ref_storage st(shared_data::instance().storage);
		EXPECT_EQ(st->size(),3U);
		EXPECT_TRUE(!st->find("k3")->second.neighbors()[right][0]);
		EXPECT_TRUE(!st->find("k3")->second.neighbors()[right][1]);
		EXPECT_TRUE(st->find("k3")->second.neighbors()[left][0]->get_key() == "k2");
		EXPECT_TRUE(st->find("k3")->second.neighbors()[left][1]->get_key() == "k2");
		EXPECT_TRUE(st->find("k3")->second.neighbors()[left][0]->get_host() == mockhost);
		EXPECT_TRUE(st->find("k3")->second.neighbors()[left][1]->get_host() == mockhost);
		EXPECT_TRUE(!st->find("k3")->second.neighbors()[left][2]);
	}
}

TEST(_6, link_to_the_middle_key){
	/*
		k1
		k1  [k2]->k2.2 k3
		k1  [k2]->k2.2 k3
	*/
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
		shared_data::ref_storage st(shared_data::instance().storage);
		EXPECT_EQ(st->size(),3U);
	}
}

TEST(_7, expect_introduce){
	/*
		k1       k2.2  new!  k3
		k1  [k2] k2.2 [k2.5] k3
		k1  [k2] k2.2 [k2.5] k3 // k3's left is k2!!!!!!
	*/
	EXPECT_TRUE(logic::detail::left_is_near("k3","k2.5","k2.2"));
	EXPECT_TRUE(get_direction("k3", "k2.5") == left);
	// request's input/output
	eval::object<const key,const host, const membership_vector>
		obj("k2.5",mockhost2,mockvector2);
	StrictMock<mock_request> req1;
	EXPECT_CALL(req1, params())
		.WillOnce(ReturnRef(obj.get()));

	StrictMock<mock_server> sv;
	
	StrictMock<mock_session> sn1;
	EXPECT_CALL(sn1, notify("link", "k2.5", 0, "k3", localhost));
	EXPECT_CALL(sn1, notify("link", "k2.5", 1, "k3", localhost));
	EXPECT_CALL(sv, get_session(mockhost2.get_address()))
		.WillRepeatedly(ReturnRef(sn1));
	
	StrictMock<mock_session> sn2;
	EXPECT_CALL(sn2, notify("introduce", "k2.5", "k2.2", mockhost2, mockvector2, 0));
	EXPECT_CALL(sv, get_session(localhost.get_address()))
		.WillRepeatedly(ReturnRef(sn2));
	
	// call
	logic::treat(&req1, &sv);
	{
		shared_data::ref_storage st(shared_data::instance().storage);
		EXPECT_EQ(st->size(),3U);
		EXPECT_TRUE(!st->find("k3")->second.neighbors()[right][0]);
		EXPECT_TRUE(!st->find("k3")->second.neighbors()[right][1]);
		EXPECT_TRUE(!st->find("k3")->second.neighbors()[right][2]);
		EXPECT_TRUE(st->find("k3")->second.neighbors()[left][0]->get_key() == "k2.5");
		EXPECT_TRUE(st->find("k3")->second.neighbors()[left][1]->get_key() == "k2.5");
		EXPECT_TRUE(!st->find("k3")->second.neighbors()[left][2]);

		EXPECT_TRUE(st->find("k2.2")->second.neighbors()[left][0]);
		EXPECT_TRUE(st->find("k2.2")->second.neighbors()[left][1]);
		EXPECT_TRUE(st->find("k2.2")->second.neighbors()[left][0]->get_key() == "k2");
		EXPECT_TRUE(st->find("k2.2")->second.neighbors()[left][1]->get_key() == "k2");
		EXPECT_TRUE(st->find("k2.2")->second.neighbors()[left][0]->get_host() == mockhost);
		EXPECT_TRUE(st->find("k2.2")->second.neighbors()[left][1]->get_host() == mockhost);
	}
}

TEST(_8, introduce_key){
	/*
		k1       k2.2         k3
		k1  [k2] k2.2<-[k2.5] k3
		k1  [k2] k2.2<-[k2.5] k3
	*/
	// request's input/output
	eval::object<const key,const key,const host, const membership_vector,int>
		obj("k2.5","k2.2",mockhost2,mockvector2,0);
	StrictMock<mock_request> req;
	EXPECT_CALL(req, params())
		.WillOnce(ReturnRef(obj.get()));

	StrictMock<mock_server> sv;
	StrictMock<mock_session> sn1;
	EXPECT_CALL(sn1, notify("introduce", "k2.5", "k2", mockhost2, mockvector2, 2));
	EXPECT_CALL(sv, get_session(mockhost.get_address()))
		.WillRepeatedly(ReturnRef(sn1));
	StrictMock<mock_session> sn2;
	EXPECT_CALL(sn2, notify("link", "k2.5", 0, "k2.2", localhost));
	EXPECT_CALL(sn2, notify("link", "k2.5", 1, "k2.2", localhost));
	EXPECT_CALL(sv, get_session(mockhost2.get_address()))
		.WillRepeatedly(ReturnRef(sn2));
	
	// call
	logic::introduce(&req, &sv);
	{
		shared_data::ref_storage st(shared_data::instance().storage);
		EXPECT_EQ(st->size(),3U);
		EXPECT_TRUE(!st->find("k3")->second.neighbors()[right][0]);
		EXPECT_TRUE(!st->find("k3")->second.neighbors()[right][1]);
		EXPECT_TRUE(!st->find("k3")->second.neighbors()[right][2]);
		EXPECT_TRUE(st->find("k3")->second.neighbors()[left][0]);
		EXPECT_TRUE(st->find("k3")->second.neighbors()[left][1]);
		EXPECT_TRUE(st->find("k3")->second.neighbors()[left][0]->get_key() == "k2.5");
		EXPECT_TRUE(st->find("k3")->second.neighbors()[left][1]->get_key() == "k2.5");
		EXPECT_TRUE(!st->find("k3")->second.neighbors()[left][2]);
		
		EXPECT_TRUE(st->find("k2.2")->second.neighbors()[right][0]);
		EXPECT_TRUE(st->find("k2.2")->second.neighbors()[right][1]);
		EXPECT_TRUE(st->find("k2.2")->second.neighbors()[right][0]->get_key() == "k2.5");
		EXPECT_TRUE(st->find("k2.2")->second.neighbors()[right][1]->get_key() == "k2.5");
		EXPECT_TRUE(st->find("k2.2")->second.neighbors()[right][0]->get_host() == mockhost2);
		EXPECT_TRUE(st->find("k2.2")->second.neighbors()[right][1]->get_host() == mockhost2);
	}
}

TEST(_9, add_more_key){
	/*                 new! 
		k1       k2.2  k2.3         k3
		k1  [k2] k2.2  k2.3  [k2.5] k3
		k1  [k2] k2.2  k2.3  [k2.5] k3
	*/
	// request's input/output
	eval::object<const key,const value>
		obj("k2.3","v2.3");
	StrictMock<mock_request> req;
	EXPECT_CALL(req, params())
		.WillOnce(ReturnRef(obj.get()));
	EXPECT_CALL(req, result(true));

	mock_server sv;
	mock_session sn1;
	EXPECT_CALL(sn1, notify("treat", "k2.3", "k2.5", 
			localhost, shared_data::instance().myvector));
	EXPECT_CALL(sv, get_session(mockhost2.get_address()))
		.WillRepeatedly(ReturnRef(sn1));
	
	// call
	logic::set(&req, &sv);
	{
		shared_data::ref_storage st(shared_data::instance().storage);
		EXPECT_EQ(st->size(),4U);
		EXPECT_TRUE(!st->find("k3")->second.neighbors()[right][0]);
		EXPECT_TRUE(!st->find("k3")->second.neighbors()[right][1]);
		EXPECT_TRUE(!st->find("k3")->second.neighbors()[right][2]);
		EXPECT_TRUE(st->find("k3")->second.neighbors()[left][0]);
		EXPECT_TRUE(st->find("k3")->second.neighbors()[left][1]);
		EXPECT_TRUE(st->find("k3")->second.neighbors()[left][0]->get_key() == "k2.5");
		EXPECT_TRUE(st->find("k3")->second.neighbors()[left][1]->get_key() == "k2.5");
		EXPECT_TRUE(!st->find("k3")->second.neighbors()[left][2]);
		
		EXPECT_TRUE(st->find("k2.2")->second.neighbors()[right][0]);
		EXPECT_TRUE(st->find("k2.2")->second.neighbors()[right][1]);
		EXPECT_TRUE(st->find("k2.2")->second.neighbors()[right][0]->get_key() == "k2.5");
		EXPECT_TRUE(st->find("k2.2")->second.neighbors()[right][1]->get_key() == "k2.5");
		EXPECT_TRUE(st->find("k2.2")->second.neighbors()[right][0]->get_host() == mockhost2);
		EXPECT_TRUE(st->find("k2.2")->second.neighbors()[right][1]->get_host() == mockhost2);
		
		EXPECT_TRUE(!st->find("k2.3")->second.neighbors()[left][0]);
		EXPECT_TRUE(!st->find("k2.3")->second.neighbors()[left][1]);
		EXPECT_TRUE(!st->find("k2.3")->second.neighbors()[left][2]);
		EXPECT_TRUE(!st->find("k2.3")->second.neighbors()[right][0]);
		EXPECT_TRUE(!st->find("k2.3")->second.neighbors()[right][1]);
		EXPECT_TRUE(!st->find("k2.3")->second.neighbors()[right][2]);
	}
}

TEST(_10, link_to_left_key){
	/*
		k1       k2.2  k2.3         k3
		k1  [k2] k2.2  k2.3  [k2.5] k3
		k1  [k2] k2.2  k2.3  [k2.5] k3
	*/
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
		shared_data::ref_storage st(shared_data::instance().storage);
		EXPECT_EQ(st->size(),4U);
		EXPECT_TRUE(!st->find("k3")->second.neighbors()[right][0]);
		EXPECT_TRUE(!st->find("k3")->second.neighbors()[right][1]);
		EXPECT_TRUE(!st->find("k3")->second.neighbors()[right][2]);
		EXPECT_TRUE(st->find("k3")->second.neighbors()[left][0]);
		EXPECT_TRUE(st->find("k3")->second.neighbors()[left][1]);
		EXPECT_TRUE(st->find("k3")->second.neighbors()[left][0]->get_key() == "k2.5");
		EXPECT_TRUE(st->find("k3")->second.neighbors()[left][1]->get_key() == "k2.5");
		EXPECT_TRUE(!st->find("k3")->second.neighbors()[left][2]);
		
		EXPECT_TRUE(st->find("k2.2")->second.neighbors()[right][0]);
		EXPECT_TRUE(st->find("k2.2")->second.neighbors()[right][1]);
		EXPECT_TRUE(st->find("k2.2")->second.neighbors()[right][0]->get_key() == "k2.5");
		EXPECT_TRUE(st->find("k2.2")->second.neighbors()[right][1]->get_key() == "k2.5");
		EXPECT_TRUE(st->find("k2.2")->second.neighbors()[right][0]->get_host() == mockhost2);
		EXPECT_TRUE(st->find("k2.2")->second.neighbors()[right][1]->get_host() == mockhost2);
		
		EXPECT_TRUE(!st->find("k2.3")->second.neighbors()[left][0]);
		EXPECT_TRUE(!st->find("k2.3")->second.neighbors()[left][1]);
		EXPECT_TRUE(!st->find("k2.3")->second.neighbors()[left][2]);
		EXPECT_TRUE(st->find("k2.3")->second.neighbors()[right][0]);
		EXPECT_TRUE(st->find("k2.3")->second.neighbors()[right][0]->get_key() == "k2.5");
		EXPECT_TRUE(st->find("k2.3")->second.neighbors()[right][0]->get_host() == mockhost2);
		EXPECT_TRUE(!st->find("k2.3")->second.neighbors()[right][1]);
		EXPECT_TRUE(!st->find("k2.3")->second.neighbors()[right][2]);
	}
}

TEST(_11, introduce_to_left_key){
	/*           introduce!
		k1       k2.2  k2.3         k3
		k1  [k2] k2.2  k2.3  [k2.5] k3
		k1  [k2] k2.2  k2.3  [k2.5] k3
	*/
	// request's input/output
	EXPECT_TRUE(logic::detail::left_is_near("k2.2","k2.3","k2.5"));
	eval::object<const key,const key, host, membership_vector, int >
		obj("k2.2", "k2.3",
			localhost, shared_data::instance().myvector, 0);
	StrictMock<mock_request> req;
	EXPECT_CALL(req, params())
		.WillOnce(ReturnRef(obj.get()));
	
	mock_server sv;
	
	// call
	logic::introduce(&req, &sv);
	{
		shared_data::ref_storage st(shared_data::instance().storage);
		EXPECT_EQ(st->size(),4U);
		EXPECT_TRUE(!st->find("k3")->second.neighbors()[right][0]);
		EXPECT_TRUE(!st->find("k3")->second.neighbors()[right][1]);
		EXPECT_TRUE(!st->find("k3")->second.neighbors()[right][2]);
		EXPECT_TRUE(st->find("k3")->second.neighbors()[left][0]);
		EXPECT_TRUE(st->find("k3")->second.neighbors()[left][1]);
		EXPECT_TRUE(st->find("k3")->second.neighbors()[left][0]->get_key() == "k2.5");
		EXPECT_TRUE(st->find("k3")->second.neighbors()[left][1]->get_key() == "k2.5");
		EXPECT_TRUE(!st->find("k3")->second.neighbors()[left][2]);
		
		EXPECT_TRUE(st->find("k2.2")->second.neighbors()[right][0]);
		EXPECT_TRUE(st->find("k2.2")->second.neighbors()[right][1]);
		EXPECT_TRUE(st->find("k2.2")->second.neighbors()[right][0]->get_key() == "k2.5");
		EXPECT_TRUE(st->find("k2.2")->second.neighbors()[right][1]->get_key() == "k2.5");
		EXPECT_TRUE(st->find("k2.2")->second.neighbors()[right][0]->get_host() == mockhost2);
		EXPECT_TRUE(st->find("k2.2")->second.neighbors()[right][1]->get_host() == mockhost2);
		
		EXPECT_TRUE(!st->find("k2.3")->second.neighbors()[left][0]);
		EXPECT_TRUE(!st->find("k2.3")->second.neighbors()[left][1]);
		EXPECT_TRUE(!st->find("k2.3")->second.neighbors()[left][2]);
		EXPECT_TRUE(st->find("k2.3")->second.neighbors()[right][0]);
		EXPECT_TRUE(st->find("k2.3")->second.neighbors()[right][0]->get_key() == "k2.5");
		EXPECT_TRUE(st->find("k2.3")->second.neighbors()[right][0]->get_host() == mockhost2);
		EXPECT_TRUE(!st->find("k2.3")->second.neighbors()[right][1]);
		EXPECT_TRUE(!st->find("k2.3")->second.neighbors()[right][2]);
	}
}

TEST(_12, introduce_to_right_key){
	/*                       introduce!
		k1       k2.2  k2.3         k3
		k1  [k2] k2.2  k2.3  [k2.5] k3
		k1  [k2] k2.2  k2.3  [k2.5] k3
	*/
	
	// request's input/output
	eval::object<const key,const key, host, membership_vector, int >
		obj("k2.3", "k3",
			localhost, shared_data::instance().myvector, 2);
	StrictMock<mock_request> req;
	EXPECT_CALL(req, params())
		.WillOnce(ReturnRef(obj.get()));
	
	mock_server sv;
	
	// call
	logic::introduce(&req, &sv);
	{
		shared_data::ref_storage st(shared_data::instance().storage);
		EXPECT_EQ(st->size(),4U);
		EXPECT_TRUE(!st->find("k3")->second.neighbors()[right][0]);
		EXPECT_TRUE(!st->find("k3")->second.neighbors()[right][1]);
		EXPECT_TRUE(!st->find("k3")->second.neighbors()[right][2]);
		EXPECT_TRUE(st->find("k3")->second.neighbors()[left][0]);
		EXPECT_TRUE(st->find("k3")->second.neighbors()[left][1]);
		EXPECT_TRUE(st->find("k3")->second.neighbors()[left][0]->get_key() == "k2.5");
		EXPECT_TRUE(st->find("k3")->second.neighbors()[left][1]->get_key() == "k2.5");
		EXPECT_TRUE(!st->find("k3")->second.neighbors()[left][2]);
		
		EXPECT_TRUE(st->find("k2.2")->second.neighbors()[right][0]);
		EXPECT_TRUE(st->find("k2.2")->second.neighbors()[right][1]);
		EXPECT_TRUE(st->find("k2.2")->second.neighbors()[right][0]->get_key() == "k2.5");
		EXPECT_TRUE(st->find("k2.2")->second.neighbors()[right][1]->get_key() == "k2.5");
		EXPECT_TRUE(st->find("k2.2")->second.neighbors()[right][0]->get_host() == mockhost2);
		EXPECT_TRUE(st->find("k2.2")->second.neighbors()[right][1]->get_host() == mockhost2);
		
		EXPECT_TRUE(!st->find("k2.3")->second.neighbors()[left][0]);
		EXPECT_TRUE(!st->find("k2.3")->second.neighbors()[left][1]);
		EXPECT_TRUE(!st->find("k2.3")->second.neighbors()[left][2]);
		EXPECT_TRUE(st->find("k2.3")->second.neighbors()[right][0]);
		EXPECT_TRUE(st->find("k2.3")->second.neighbors()[right][0]->get_key() == "k2.5");
		EXPECT_TRUE(st->find("k2.3")->second.neighbors()[right][0]->get_host() == mockhost2);
		EXPECT_TRUE(!st->find("k2.3")->second.neighbors()[right][1]);
		EXPECT_TRUE(!st->find("k2.3")->second.neighbors()[right][2]);
	}
}

const host mockhost3("1.2.3.4",121);
const membership_vector mockvector3(1);

TEST(_13, more_middle_key){

	/*                    
		k1       k2.2           k2.3         k3
		k1  [k2] k2.2   new!    k2.3  [k2.5] k3
		k1  [k2] k2.2  [k2.22]  k2.3  [k2.5] k3
	*/
	EXPECT_TRUE(logic::detail::left_is_near("k2.2","k2.22","k2.3"));
	EXPECT_TRUE(logic::detail::left_is_near("k2.3","k2.22","k2.2"));
	// request's input/output
	eval::object<key,host,membership_vector>
		obj("k2.22",mockhost3, mockvector3);
	StrictMock<mock_request> req;
	EXPECT_CALL(req, params())
		.WillOnce(ReturnRef(obj.get()));

	StrictMock<mock_server> sv;

	StrictMock<mock_session> sn1;
	EXPECT_CALL(sn1, notify
		("link", "k2.22", 0, "k2.3", localhost));
	EXPECT_CALL(sv, get_session(mockhost3.get_address()))
		.WillRepeatedly(ReturnRef(sn1));
	
	StrictMock<mock_session> sn2;
	EXPECT_CALL(sn2, notify
		("introduce", "k2.22", "k2.2", mockhost3, mockvector3, 0));
	EXPECT_CALL(sv, get_session(localhost.get_address()))
		.WillRepeatedly(ReturnRef(sn2));
	
	StrictMock<mock_session> sn3;
	EXPECT_CALL(sn3, notify
		("introduce", "k2.22", "k2.5", mockhost3, mockvector3, 1));
	EXPECT_CALL(sv, get_session(mockhost2.get_address()))
		.WillRepeatedly(ReturnRef(sn3));
	
	// call
	logic::treat(&req, &sv);
}

TEST(_14, introduce_middle_key){
	/*            introduce
		k1       k2.2           k2.3         k3
		k1  [k2] k2.2           k2.3  [k2.5] k3
		k1  [k2] k2.2  [k2.22]  k2.3  [k2.5] k3
	*/
	EXPECT_TRUE(logic::detail::left_is_near("k2.2","k2.22","k2.3"));
	EXPECT_TRUE(logic::detail::left_is_near("k2.3","k2.22","k2.2"));
	// request's input/output
	eval::object<key,key,host,membership_vector,int>
		obj("k2.22", "k2.2",mockhost3, mockvector3, 0);
	StrictMock<mock_request> req;
	EXPECT_CALL(req, params())
		.WillOnce(ReturnRef(obj.get()));

	StrictMock<mock_server> sv;
	ON_CALL(sv, get_session(_)).
		WillByDefault(ReturnRef(sv.invalid_session));

	StrictMock<mock_session> sn1;
	EXPECT_CALL(sn1, notify
		("link", "k2.22", 0, "k2.2", localhost));
	EXPECT_CALL(sv, get_session(mockhost3.get_address()))
		.WillRepeatedly(ReturnRef(sn1));
	
	StrictMock<mock_session> sn2;
	EXPECT_CALL(sn2, notify
		("introduce", "k2.22", "k2", mockhost3, mockvector3, 1));
	EXPECT_CALL(sv, get_session(mockhost.get_address()))
		.WillRepeatedly(ReturnRef(sn2));
	
	// call
	logic::introduce(&req, &sv);
	{
		shared_data::ref_storage st(shared_data::instance().storage);
		EXPECT_TRUE(st->find("k1") != st->end());
	}
}

TEST(_15, introduce_middle_key_to_right){
	/*            introduce
		k1       k2.2           k2.3         k3
		k1  [k2] k2.2           k2.3  [k2.5] k3
		k1  [k2] k2.2  [k2.22]  k2.3  [k2.5] k3
	*/
	// request's input/output
	eval::object<key,key,host,membership_vector,int>
		obj("k2.22", "k3",mockhost3, mockvector3, 1);
	StrictMock<mock_request> req;
	EXPECT_CALL(req, params())
		.WillOnce(ReturnRef(obj.get()));

	StrictMock<mock_server> sv;
	
	// call
	logic::introduce(&req, &sv);
	{
		shared_data::ref_storage st(shared_data::instance().storage);
		EXPECT_TRUE(st->find("k1") != st->end());
	}
}
