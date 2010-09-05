#include <string>
#include <map>

// external lib
#include <boost/program_options.hpp>
#include <boost/function.hpp>

// my include
#include "skipgraph.h"
#include "tcp_wrap.h"

using namespace mp::placeholders;
namespace rpc { using namespace msgpack::rpc; }

#define DEBUG_MODE
#include "debug_macro.h"

#include "sg_logic.hpp"

class sg_dispatcher : public msgpack::rpc::dispatcher, public boost::noncopyable{
public:
	sg_dispatcher():ref_table(reflection::instance()){}
	void dispatch(msgpack::rpc::request req){
		// get the name of method
		const std::string& method = req.method().as<std::string>();
		// call the method
		bool success = ref_table.call(method, req.params());
		if(!success){
			std::cerr << "undefined function [" << method <<
				"] called" << std::endl;
			return;
		}
	}
	const reflection& ref_table;
	//const reflection::name_to_func* ref_table; //name to function
};

void parse_skipgraph_command_line(int argc, char** argv, settings* s){
	BLOCK("setting");
	boost::program_options::options_description opt("options");
	std::string master;
	opt.add_options()
		("help,h", "view help")
		("verbose,v", "verbose mode")
		("interface,i",boost::program_options::value<std::string>
		 (&s->interface)->default_value("eth0"), "my interface")
		("port,p",boost::program_options::value<unsigned short>
		 (&s->myport)->default_value(11011), "my port number")
		("address,a",boost::program_options::value<std::string>
		 (&master)->default_value("127.0.0.1"), "master's address");
	s->myip = get_myip_interface2(s->interface.c_str());
 
	boost::program_options::variables_map vm;
	store(parse_command_line(argc,argv,opt), vm);
	notify(vm);
	if(vm.count("help")){
		std::cerr << opt << std::endl;
		exit(0);
	}
		
	// set options
	if(vm.count("verbose")){
		s->verbose++;
	}
}

int main(int argc, char** argv){
	settings& s = settings::instance();
	
	// (argc argv) -> settings
	parse_skipgraph_command_line(argc,argv,&s);

	// start server
	msgpack::rpc::server sg_server;
	sg_dispatcher sd; // main callback
	sg_server.serve(&sd); 
	sg_server.listen("127.0.0.1", s.myport);
	sg_server.start(4);
	
	sg_server.join();// wait for server end
	return 0;
}
