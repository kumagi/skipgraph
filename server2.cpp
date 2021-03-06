#include <msgpack/rpc/server.h>
#include <msgpack/rpc/client.h>

#include <cclog/cclog.h>
#include <cclog/cclog_tty.h>
using namespace mp::placeholders;
namespace rpc { using namespace msgpack::rpc; }

class myserver : public rpc::dispatcher {
public:

	void dispatch(rpc::request req)
	{
		// 呼ばれたらメッセージを表示するだけ
		const std::string& method = req.method().as<std::string>();
		std::cout << "message :" << method << std::endl;
		req.result(1);
	}

	rpc::server& listen(const std::string& host, uint16_t port)
	{
		m_svr.serve(this);
		m_svr.listen(host, port);
		return m_svr;
	}
public:
	myserver() { }	 // イベントループを共有
		
private:
	msgpack::rpc::server m_svr;
};

int main(void)
{
	cclog::reset(new cclog_tty(cclog::TRACE, std::cout));
	myserver s;
	s.listen("0.0.0.0", 8080).run(4);		 // サーバを4スレッドで起動
}
