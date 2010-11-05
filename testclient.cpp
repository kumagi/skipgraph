#include <string>
#include <vector>

#include <boost/shared_ptr.hpp>
#include <boost/program_options.hpp>
#include <boost/random.hpp>
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
	unsigned int seed;
	opt.add_options() 
		("help,h", "view help")
		("address,a",boost::program_options::value<std::string>
		 (&targetip)->default_value("127.0.0.1"), "target's address")
		("random,r",boost::program_options::value<unsigned int>
		 (&seed)->default_value(0), "random seed")
		("mport,p",boost::program_options::value<unsigned short>
		 (&targetport)->default_value(12321), "target's port number");
	
	boost::program_options::variables_map vm;
	store(parse_command_line(argc,argv,opt), vm);
	notify(vm);
	if(vm.count("help")){
		std::cerr << opt << std::endl;
		exit(0);
	}

	boost::mt19937 engine(seed);
	boost::uniform_smallint<> range(0, 10000);
	boost::variate_generator<boost::mt19937&, boost::uniform_smallint<> > 
		rand(engine,range);
	
	disp d;
	msgpack::rpc::server svr;
	svr.serve(&d);
	svr.listen("0.0.0.0",8091);
	svr.start(4);
	
	for(int i=0;i<100;i++){
		msgpack::rpc::session s = svr.get_session(targetip,targetport);
		int r = rand();
		s.call("set",
			std::string("k")+boost::lexical_cast<std::string>(r),
			std::string("v")+boost::lexical_cast<std::string>(r));
	}
	svr.join();
}
