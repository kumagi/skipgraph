#include <string>
#include <vector>

#include <boost/shared_ptr.hpp>
#include <boost/program_options.hpp>

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
	
	msgpack::rpc::client c(targetip, targetport);
	
	c.notify("dump");
	c.run_once();
}
