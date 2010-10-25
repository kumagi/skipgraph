#include "msgpack/rpc/server.h"
#include "msgpack/rpc/client.h"

using namespace mp::placeholders;
namespace rpc { using namespace msgpack::rpc; }

class myserver : public rpc::dispatcher {
public:
	// // 中継先のサーバから応答が返ってきたら呼ばれる
	// static void callback(rpc::future f, rpc::request req)
	// {
	// 	req.result(f.get<int>());	 // ここで応答を返す
	// }

	// クライアントから要求を受け取ったら呼ばれる
	void dispatch(rpc::request req)
	{
		// const std::string& method = req.method().as<std::string>();
		// msgpack::type::tuple<int, int> params(req.params());

		// rpc::session s = m_svr.get_session("127.0.0.1", 8080); 
				
		// s.call(method, params.get<0>(), params.get<1>());
//			.attach_callback( mp::bind(&callback, _1, req) );
	}

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
		s.notify("hello1");
		//s.call("hello2");
	}
private:
	msgpack::rpc::server m_svr;
};

int main(void)
{
	myserver s;
	s.listen("0.0.0.0", 9090).run(4);		 // サーバを4スレッドで起動
}
