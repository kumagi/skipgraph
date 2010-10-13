#ifndef DEBUG_MACRO_H
#define DEBUG_MACRO_H
#include <stdio.h>
#include <string>
#include <iostream>
#include <boost/noncopyable.hpp>

namespace debugmode{
class block :boost::noncopyable{
	mutable std::string message;
public:
	block(const std::string& msg):message(msg){
		std::cerr << "start[" << message << "]";
	}
	~block(){
		std::cerr << "end[" << message << "]" << std::endl;
	}
};
}//namespace
#ifdef DEBUG_MODE
#define DEBUG_OUT(...) fprintf(stderr,__VA_ARGS__)
#define DEBUG(x) x
#define REACH(x) fprintf(stderr,"reach line:%d %s",__LINE__, x)
#define BLOCK(x) debugmode::block DEBUGBLOCK_(x)
#else
#define NDEBUG
#define DEBUG_OUT(...) 
#define DEBUG(...)
#define REACH(...)
#define BLOCK(...)
#endif//DEBUG_MODE


#endif//DEBUG_MACRO_H
