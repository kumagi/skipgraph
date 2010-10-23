#ifndef SKIPGRAPH_H
#define SKIPGRAPH_H

#include <string>
#include <vector>
#include <map>

#include <boost/shared_ptr.hpp>
#include <boost/program_options.hpp>

#include "msgpack/rpc/server.h"
#include "msgpack/rpc/client.h"

#define DEBUG_MODE
#include "debug_macro.h"

#include "objlib.h"
#include "tcp_wrap.h"

#include "sg_objects.hpp"
#include "msgpack_macro.h"
#include <mp/wavy.h>
#include <mp/sync.h>

struct settings: public singleton<settings>{
	enum status_{
		ready,
		unjoined,
	};
	int verbose;
	std::string interface;
	unsigned short myport;
	int myip;
	int maxlevel;
	int status;
	host myhost;
	host master;
	uint64_t vec;
	settings()
		:verbose(0),interface("eth0"),
		 myport(12321),myip(get_myip()),maxlevel(16),master("127.0.0.1",12321),vec(0){}
};


/*
namespace msg{
enum operation{
	DIE,
	DUMP,
	INFORM,
	SET,
	SET_OK,
	SEARCH,
	FOUND,
	NOTFOUND,
	RANGE,
	RANGE_FOUND,
	RANGE_NOTFOUND,
	LINK,
	TREAT,
	INTRODUCE,
};

}
*/

void parse_skipgraph_command_line(int argc, char** argv, settings* s){
	BLOCK("setting");
	boost::program_options::options_description opt("options");
	std::string master;
	unsigned short masterport;
	opt.add_options() 
		("help,h", "view help")
		("vector,v", boost::program_options::value<uint64_t>
		 (&s->vec)->default_value(1), "membership vector")
		("level,l",boost::program_options::value<int>
		 (&s->maxlevel)->default_value(8), "max level")
		("interface,i",boost::program_options::value<std::string>
		 (&s->interface)->default_value("eth0"), "my interface")
		("port,p",boost::program_options::value<unsigned short>
		 (&s->myport)->default_value(12321), "my port number")
		("address,a",boost::program_options::value<std::string>
		 (&master)->default_value("127.0.0.1"), "master's address")
		("mport,P",boost::program_options::value<unsigned short>
		 (&masterport)->default_value(12321), "master port number");
	
 
	boost::program_options::variables_map vm;
	store(parse_command_line(argc,argv,opt), vm);
	notify(vm);
	if(vm.count("help")){
		std::cerr << opt << std::endl;
		exit(0);
	}
	s->myhost = host(ntoa(get_myip_interface2(s->interface.c_str())), s->myport);
	s->master = host(master, masterport);
	
	// set options
	if(vm.count("verbose")){
		s->verbose++;
	}
}
#endif



