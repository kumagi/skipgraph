#include <string>
#include <map>

// external lib
#include <boost/function.hpp>
#include <boost/lexical_cast.hpp>

// my include
#include "skipgraph.h"
#include "tcp_wrap.h"
#include <cclog/cclog.h>
#include <cclog/cclog_tty.h>



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
		std::cerr << "@";
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
	host.regist(std::string(#x), logic::x<msgpack::rpc::request, msgpack::rpc::server>)

int main(int argc, char** argv){
	cclog::reset(new cclog_tty(cclog::TRACE, std::cerr));
	settings& s = settings::instance();
	
	// (argc argv) -> settings
	parse_skipgraph_command_line(argc,argv,&s);

	// start server
	msgpack::rpc::server sg_server;
	sg_host host(&sg_server); // main callback

	{// regist callbacks
		REGIST(set);
		REGIST(die);
		REGIST(dump);
		REGIST(search);
		REGIST(found);
		REGIST(notfound);
		REGIST(link);
		REGIST(introduce);
		REGIST(treat);
	}
	sg_server.serve(&host);
	sg_server.listen("0.0.0.0", s.myport);

	shared_data::instance().set_host(s.myhost.hostname, s.myhost.port);
	
	shared_data::instance().myvector = membership_vector(s.vec);
	
	sg_server.start(4);

	std::cout << s.master.hostname << ":" << s.master.port << std::endl;
	if(s.master.hostname == "127.0.0.1" && s.master.port == s.myport){
		shared_data::instance().myvector = membership_vector(0);
		shared_data::instance().maxlevel = s.maxlevel;
		// save minimum key
		std::string keyname = "__node_master" + std::string(":") +
			boost::lexical_cast<std::string>(s.myport);

		shared_data::storage_t& st = shared_data::instance().storage.get_ref();
		st.add(keyname, sg_node("dummy", shared_data::instance().myvector,
					shared_data::instance().maxlevel));
		
	}else{
		shared_data::instance().myvector = membership_vector(s.vec);
		shared_data::instance().maxlevel = s.maxlevel;
		// treat minimum key
		std::string keyname = "__node_"+ s.master.hostname + std::string(":") +
			boost::lexical_cast<std::string>(s.myport);

		shared_data::storage_t& st = shared_data::instance().storage.get_ref();
		st.add(keyname,
				sg_node("dummy", shared_data::instance().myvector,
					shared_data::instance().maxlevel));
		
		sg_server.get_session(s.master.get_address())
			.notify("treat",keyname, s.myhost, shared_data::instance().myvector);
	}
	
	//std::cout << shared_data::instance() << std::endl;
	
	
	sg_server.join();// wait for server ends
	return 0;
}
