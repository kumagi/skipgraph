#ifndef MSGPACK_MACRO_H
#define MSGPACK_MACRO_H

#define RPC_OPERATION1(NAME, type1, arg1)\
	struct NAME{ type1 arg1;\
		NAME(const type1& arg1_)										\
			:arg1(arg1_){}														\
		MSGPACK_DEFINE(arg1);												\
	};\
	namespace msg{\
	void NAME(const msgpack::object& obj);\
	};
	
#define RPC_OPERATION2(NAME, type1, arg1, type2, arg2)\
	struct NAME{ type1 arg1; type2 arg2;										\
		NAME(const type1& arg1##_, const type2& arg2##_)\
			:arg1(arg1##_),arg2(arg2##_){}\
		MSGPACK_DEFINE(arg1,arg2);\
	};\
	namespace msg{\
	void NAME(const msgpack::object& obj);\
	};
#define RPC_OPERATION3(NAME, type1, arg1, type2, arg2, type3, arg3)	\
	struct NAME{ type1 arg1; type2 arg2; type3 arg3;											\
		NAME(const type1& arg1##_, const type2& arg2##_, const type3& arg3##_)										\
			:arg1(arg1##_),arg2(arg2##_),arg3(arg3##_){}															\
		MSGPACK_DEFINE(arg1,arg2,arg3);																					\
	};\
	namespace msg{\
	void NAME(const msgpack::object& obj);\
	};
#define RPC_OPERATION4(NAME, type1, arg1, type2, arg2, type3, arg3,type4,arg4) \
	struct NAME{ type1 arg1; type2 arg2; type3 arg3; type4 arg4;					\
		NAME(const type1& arg1##_, const type2& arg2##_, const type3& arg3##_, const type4& arg4##_)	\
			:arg1(arg1##_),arg2(arg2##_),arg3(arg3##_),arg4(arg4##_){}								\
		MSGPACK_DEFINE(arg1,arg2,arg3,arg4);																		\
	};\
	namespace msg{\
	void NAME(const msgpack::object& obj);\
	};
#define RPC_OPERATION5(NAME, type1, arg1, type2, arg2, type3, arg3,type4, arg4, type5, arg5) \
	struct NAME{ type1 arg1; type2 arg2; type3 arg3; type4 arg4; type5 arg5; \
		NAME(const type1& arg1##_, const type2& arg2##_, const type3& arg3##_, const type4& arg4##_, const type5& arg5##_)	\
			:arg1(arg1##_),arg2(arg2##_),arg3(arg3##_),arg4(arg4##_),arg5(arg5##_){} \
		MSGPACK_DEFINE(arg1,arg2,arg3,arg4,arg5);																\
	};\
	namespace msg{\
	void NAME(const msgpack::object& obj);\
	};


#endif






