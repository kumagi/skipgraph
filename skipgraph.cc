// standard lib
#include <string>
#include <map>

// external lib
#include <boost/program_options.hpp>
#include <boost/function.hpp>

// my lib
#include "skipgraph.h"
#include "tcp_wrap.h"




using namespace mp::placeholders;
namespace rpc { using namespace msgpack::rpc; }

#define DEBUG_MODE
#include "debug_macro.h"

typedef boost::function<void (msgpack::object)> reaction;
typedef std::map<std::string, reaction> table;

static table name_to_func;



#define MSGPACK_OPERATION(NAME, type1, arg1)\
	struct NAME :public def_base{ type1 arg1##_;							\
	NAME(const type1& arg1)		\
		:arg1##_(arg1){\
		name_to_func.insert(std::make_pair("##NAME##",NAME));	\
	}\
	MSGPACK_DEFINE(arg1##_);\
	};

/*
#define MSGPACK_OPERATION(NAME, type1, arg1, type2 ,arg2)\
	struct name{ type1 type1##_;\
	type2 type2##_;\
	NAME(const type1& arg1, const type2& arg2)		\
		:type1##_(arg1),type2##_(arg2){}\
	MSGPACK_DEFINE(arg1,arg2);\
	}
*/
typedef boost::function<void (msgpack::object)> reaction;
typedef std::map<std::string, reaction> table;
typedef boost::function<void (msgpack::object)> reaction;
typedef std::map<std::string, reaction> table;
MSGPACK_OPERATION(die, std::string, name);

void die(const msgpack::object& obj){
    REACH("die:");
 

}


class sg_dispatcher : public msgpack::rpc::dispatcher, boost::noncopyable{
	void dispatch(msgpack::rpc::request req){
		const std::string& method = req.method().as<std::string>();
		table::iterator it = reflection.find(method);
		if(it == reflection.end()){
			req.error(1);
			std::cerr << "undefined function [" << method <<
				"] called" << std::endl;
			return;
		}
		it->second(req.params());
	}
	msgpack::rpc::session_pool sp;
	table reflection; //name to function
};



int main(int argc, char** argv){
	settings& s = settings::instance();
	
	{
		BLOCK("setting");
		boost::program_options::options_description opt("options");
		std::string master;
		opt.add_options()
			("help,h", "view help")
			("verbose,v", "verbose mode")
			("interface,i",boost::program_options::value<std::string>
			 (&s.interface)->default_value("eth0"), "my interface")
			("port,p",boost::program_options::value<unsigned short>
			 (&s.myport)->default_value(11011), "my port number")
			("address,a",boost::program_options::value<std::string>
			 (&master)->default_value("127.0.0.1"), "master's address");
  
		s.myip = get_myip_interface2(s.interface.c_str());
 
		boost::program_options::variables_map vm;
		store(parse_command_line(argc,argv,opt), vm);
		notify(vm);
		if(vm.count("help")){
			std::cerr << opt << std::endl;
			return 0;
		}
		
		// set options
		if(vm.count("verbose")){
			s.verbose++;
		}
	}
	
	msgpack::rpc::server sg_server;
	sg_dispatcher sd; // main callback
	sg_server.serve(&sd); 
	sg_server.listen("127.0.0.1", s.myport);
	sg_server.start(4);
	
	sg_server.join();// wait for server end
	return 0;
}
