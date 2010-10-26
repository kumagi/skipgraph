#include <string>
#include <vector>

#include <boost/shared_ptr.hpp>
#include <boost/program_options.hpp>
#include <boost/lexical_cast.hpp>

#include "msgpack/rpc/server.h"
#include "msgpack/rpc/client.h"

#define DEBUG_MODE
#include "debug_macro.h"

#include "objlib.h"
#include "tcp_wrap.h"
#include "sg_logic.hpp"

#include "sg_objects.hpp"
#include "msgpack_macro.h"

#include <cclog/cclog.h>
#include <cclog/cclog_tty.h>


class disp : public msgpack::rpc::dispatcher{
	void dispatch(msgpack::rpc::request req)
	{}
};


int main(int argc, char** argv){

	cclog::reset(new cclog_tty(cclog::TRACE, std::cerr));
	
	boost::program_options::options_description opt("options");
	std::string targetip;
	unsigned short targetport;
	opt.add_options() 
		("help,h", "view help")
		("address,a",boost::program_options::value<std::string>
		 (&targetip)->default_value("127.0.0.1"), "target's address")
		("mport,p",boost::program_options::value<unsigned short>
		 (&targetport)->default_value(12321), "target's port number");
	
	boost::program_options::variables_map vm;
	store(parse_command_line(argc,argv,opt), vm);
	notify(vm);
	if(vm.count("help")){
		std::cerr << opt << std::endl;
		exit(0);
	}

	disp d;
	msgpack::rpc::server svr;
	svr.serve(&d);
	svr.listen("0.0.0.0",8091);
	svr.start(4);
	
	for(int i=0;i<1000000;i++){
		msgpack::rpc::session s = svr.get_session(targetip,targetport);
		s.call("set",
			std::string("k")+boost::lexical_cast<std::string>(i),
			std::string("v")+boost::lexical_cast<std::string>(i));
	}
	svr.join();
}
