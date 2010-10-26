#include "msgpack/rpc/server.h"
#include "msgpack/rpc/client.h"

#include <cclog/cclog.h>
#include <cclog/cclog_tty.h>

using namespace mp::placeholders;
namespace rpc { using namespace msgpack::rpc; }


void returned(rpc::future f){
	std::cout << "returned!" << std::endl;
}

class myserver : public rpc::dispatcher {
public:
	void dispatch(rpc::request req)
	{}
	rpc::server& listen(const std::string& host, uint16_t port)
	{
		m_svr.serve(this);
		m_svr.listen(host, port);
		return m_svr;
	}

public:
	myserver() {
		// クライアントとして
		rpc::session s = m_svr.get_session("127.0.0.1", 8080);
//		s.call("hello2").attach_callback(returned);
		sleep(1);
		s.notify("hello1");
	}
private:
	msgpack::rpc::server m_svr;
};

int main(void)
{
	cclog::reset(new cclog_tty(cclog::TRACE, std::cerr));
	myserver s;
	s.listen("0.0.0.0", 9090).run(4);		 // サーバを4スレッドで起動
}
