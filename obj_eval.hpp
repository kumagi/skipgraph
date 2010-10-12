#ifndef OBJ_EVAL_HPP
#define OBJ_EVAL_HPP
#include <msgpack.hpp>

#include <boost/mpl/int.hpp>
#include <boost/preprocessor/arithmetic/sub.hpp>
#include <boost/preprocessor/cat.hpp>
#include <boost/preprocessor/repetition/repeat.hpp>
#include <boost/preprocessor/repetition/enum.hpp>

#include <boost/preprocessor/repetition/enum_params.hpp>
#include <boost/preprocessor/repetition/enum_trailing_params.hpp>
#include <boost/preprocessor/repetition/enum_trailing.hpp>


namespace eval{

#define MAX_PARAMS 16

#define EVAL_OBJECT_PARENT(n)\
	template <BOOST_PP_ENUM_PARAMS(n, typename A)>	\
		class object{};
EVAL_OBJECT_PARENT(MAX_PARAMS)

#define TEXT(ignore1, ignore2, text) text
#define TEXT_TRAILING(ignore1, ignore2, text) text
#define EVAL_OBJECT(n)\
	template <BOOST_PP_ENUM_PARAMS(n ,typename A)>	\
	class object<BOOST_PP_ENUM_PARAMS(n ,typename A) \
		BOOST_PP_ENUM_TRAILING(BOOST_PP_SUB(MAX_PARAMS,n),TEXT_TRAILING,void)>{	\
public:\
	object(const A0& a0){\
		msgpack::type::tuple<BOOST_PP_ENUM_PARAMS(n,A)> tuple(BOOST_PP_ENUM_PARAMS(n,a)); \
		tuple.msgpack_object(&obj,&z);\
	}\
	const msgpack::object& get()const{return obj;}\
private:\
	msgpack::zone z;\
	msgpack::object obj;\
};\

EVAL_OBJECT(1)

/*
template <typename A0>
class object<A0,void, void, void>{
public:
	object(const A0& a0){
		msgpack::type::tuple<A0> tuple(a0);
		tuple.msgpack_object(&obj,&z);
	}
	const msgpack::object& get()const{return obj;}
private:
	msgpack::zone z;
	msgpack::object obj;
};

template <typename A0, typename A1>
class object<A0, A1, void, void>{
public:
	object(const A0& a0, const A1& a1){
		msgpack::type::tuple<A0> tuple(a0,a1);
		tuple.msgpack_object(&obj,&z);
s	}
	const msgpack::object& get()const{return obj;}
private:
	msgpack::zone z;
	msgpack::object obj;
};
template <typename A0, typename A1, typename A2>
class object<A0, A1, A2, void>{
public:
	object(const A0& a0, const A1& a1, const A2& a2){
		msgpack::type::tuple<A0> tuple(a0,a1, a2);
		tuple.msgpack_object(&obj,&z);
	}
	const msgpack::object& get()const{return obj;}
private:
	msgpack::zone z;
	msgpack::object obj;
};
*/
}


#endif
