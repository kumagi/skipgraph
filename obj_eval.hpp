#ifndef OBJ_EVAL_HPP
#define OBJ_EVAL_HPP
#include <msgpack.hpp>

#include <boost/mpl/int.hpp>
#include <boost/preprocessor/arithmetic/sub.hpp>
#include <boost/preprocessor/cat.hpp>
#include <boost/preprocessor/repetition/repeat.hpp>
#include <boost/preprocessor/repetition/repeat_from_to.hpp>
#include <boost/preprocessor/repetition/enum.hpp>

#include <boost/preprocessor/repetition/enum_params.hpp>
#include <boost/preprocessor/repetition/enum_trailing_params.hpp>
#include <boost/preprocessor/repetition/enum_trailing.hpp>

namespace{
#define TYPENAME_DEF(ignore1,param,ignore2) typename A##param = void
#define EVAL_OBJECT_PARENT(n)										\
	template <BOOST_PP_ENUM(n,TYPENAME_DEF,fuga)>	\
		class object{};
	
#define TEXT(ignore1,ignore2, text) text
#define ARG(ignore,param,arg) arg(param)

#define CONST_REF_ARG_A(ignore1, param, text) const text##param& a##param
#define EVAL_OBJECT(n)																									\
	template <BOOST_PP_ENUM_PARAMS(n,typename A)>													\
		class object<BOOST_PP_ENUM_PARAMS(n,A)>{\
	public:\
	object(BOOST_PP_ENUM(n,CONST_REF_ARG_A,A)){														\
		typename msgpack::type::tuple<BOOST_PP_ENUM_PARAMS(n,A)> tuple(BOOST_PP_ENUM_PARAMS(n,a)); \
		msgpack::pack(&sb, tuple);																									\
		obj = msgpack::unpack(sb.data(),sb.size(),z,NULL);				\
	}\
	const typename msgpack::object& get()const{return obj;}\
private:\
	msgpack::sbuffer sb;\
	msgpack::zone z;																\
	msgpack::object obj;\
};

}

namespace eval{
#define MAX_PARAMS 4

#define RPT2EVL(ignore1,param,ignore2) EVAL_OBJECT(param)
EVAL_OBJECT_PARENT(MAX_PARAMS)
BOOST_PP_REPEAT_FROM_TO_D(5,1,MAX_PARAMS,RPT2EVL,43)
}

#undef EVAL_OBJECT
#undef CONST_REF_ARG_A
#undef EVAL_OBJECT_PARENT
#undef TEXT
#undef ARG
#undef MAX_PARAMS
#undef RPT2EVL
#endif
