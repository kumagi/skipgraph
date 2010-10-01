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


int main(void){
	msgpack::rpc::client c("127.0.0.1", 11011);
	
	c.notify("set",key("k1"),value("v1"));
	c.notify("search",key("k1"));
	
	c.run_once();

}
