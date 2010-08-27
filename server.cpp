#include <msgpack/rpc/server.h>
#include <msgpack/rpc/client.h>

using namespace mp::placeholders;
namespace rpc { using namespace msgpack::rpc; }

class myserver : public rpc::dispatcher {
public:
	// 中継先のサーバから応答が返ってきたら呼ばれる
	static void callback(rpc::future f, rpc::request req)
	{
		req.result(f.get<int>());	 // ここで応答を返す
	}

	// クライアントから要求を受け取ったら呼ばれる
	void dispatch(rpc::request req)
	{
		std::string method = req.method().as<std::string>();
		msgpack::type::tuple<int, int> params(req.params());

		// 別のサーバに処理を中継
		rpc::session s = m_sp.get_session("127.0.0.1", 8080); 
				
		// コールバック呼び出し
		s.call(method, params.get<0>(), params.get<1>())
			.attach_callback( mp::bind(&callback, _1, req) );
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
	s.listen("0.0.0.0", 9090).start(4);		 // サーバを4スレッドで起動

	// クライアントから要求を発行
	msgpack::rpc::client c("127.0.0.1", 9090);
	int result = c.call("add", 1, 2).get<int>();
		
	std::cout << "complete :"<< result << std::endl;;
}
