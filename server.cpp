#include "msgpack/rpc/server.h"
#include "msgpack/rpc/client.h"

using namespace mp::placeholders;
namespace rpc { using namespace msgpack::rpc; }

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
		// クライアントとして働く
		rpc::session s = m_svr.get_session("127.0.0.1", 8080);
		s.notify("hello1"); // この単行だと無反応

		s.call("hello2");
                // ↑この行がある場合に限り
                // message: hello1
                // message: hello2
                // と表示される
	}
private:
	msgpack::rpc::server m_svr;
};

int main(void)
{
	myserver s;
	s.listen("0.0.0.0", 9090).run(4);		 // サーバを4スレッドで起動
}
