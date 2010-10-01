#include <msgpack/rpc/server.h>
#include <msgpack/rpc/client.h>

using namespace mp::placeholders;
namespace rpc { using namespace msgpack::rpc; }

class myserver : public rpc::dispatcher {
public:

	void dispatch(rpc::request req)
	{
		const std::string& method = req.method().as<std::string>();
		msgpack::type::tuple<int, int> params(req.params());
		std::cout << "message " << method << std::endl;
		req.result(1);
	}

	rpc::server& listen(const std::string& host, uint16_t port)
	{
		m_svr.serve(this);
		m_svr.listen(host, port);
		return m_svr;
	}
public:
	myserver() : m_sp(m_svr.get_loop()) { }	 // イベントループを共有
		
private:
	msgpack::rpc::server m_svr;
	msgpack::rpc::session_pool m_sp;
};

int main(void)
{
	myserver s;
	
	s.listen("0.0.0.0", 8080).start(4);		 // サーバを4スレッドで起動
	sleep(10000);
}
