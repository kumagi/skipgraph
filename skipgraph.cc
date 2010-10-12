#include <string>
#include <map>

// external lib
#include <boost/function.hpp>

// my include
#include "skipgraph.h"
#include "tcp_wrap.h"

using namespace mp::placeholders;
namespace rpc { using namespace msgpack::rpc; }

#include "sg_logic.hpp"

class sg_host : public msgpack::rpc::dispatcher, public boost::noncopyable{
public:
	sg_host(msgpack::rpc::server* sv):
		ref_table(reflection<msgpack::rpc::server>()),m_sv(sv){}
	void dispatch(msgpack::rpc::request req){
		// get the name of method
		const std::string& method = req.method().as<std::string>();
		// call the method
		bool success = ref_table.call(method, &req, m_sv);
		if(!success){ 
			std::cerr << "undefined function ["
								<< method << "] called" << std::endl;
			return;
		}
	}
	void regist(const std::string& name,
							reflection<msgpack::rpc::server>::reaction func){
		ref_table.regist(name,func);
	}
private:
	reflection<msgpack::rpc::server> ref_table;
	msgpack::rpc::server* m_sv;
	//const reflection::name_to_func* ref_table; //name to function
private:
	sg_host();
};

#define REGIST(x) \
	host.regist("##x##", logic::x<msgpack::rpc::request, msgpack::rpc::server>)

int main(int argc, char** argv){
	settings& s = settings::instance();
	
	// (argc argv) -> settings
	parse_skipgraph_command_line(argc,argv,&s);

	// start server
	msgpack::rpc::server sg_server;
	sg_host host(&sg_server); // main callback

	{// regist callbacks
		REGIST(die);
		REGIST(search);
		REGIST(found);
		REGIST(notfound);
		REGIST(link);
		REGIST(treat);
	}
	sg_server.serve(&host);
	sg_server.listen("127.0.0.1", s.myport);
	sg_server.start(4);
	
	sg_server.join();// wait for server ends
	return 0;
}
