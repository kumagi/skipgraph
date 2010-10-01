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
  settings()
		:verbose(0),interface("eth0"),
		 myport(11011),myip(get_myip()),maxlevel(16){}
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
	opt.add_options() 
		("help,h", "view help")
		("verbose,v", "verbose mode")
		("interface,i",boost::program_options::value<std::string>
		 (&s->interface)->default_value("eth0"), "my interface")
		("port,p",boost::program_options::value<unsigned short>
		 (&s->myport)->default_value(11011), "my port number")
		("address,a",boost::program_options::value<std::string>
		 (&master)->default_value("127.0.0.1"), "master's address");
 
	boost::program_options::variables_map vm;
	store(parse_command_line(argc,argv,opt), vm);
	notify(vm);
	if(vm.count("help")){
		std::cerr << opt << std::endl;
		exit(0);
	}
	s->myip = get_myip_interface2(s->interface.c_str());
	
	// set options
	if(vm.count("verbose")){
		s->verbose++;
	}
}
#endif



