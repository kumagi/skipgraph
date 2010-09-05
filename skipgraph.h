#ifndef SKIPGRAPH_H
#define SKIPGRAPH_H
#include "objlib.h"
#include "tcp_wrap.h"
#include <string>
#include <vector>

#include <boost/shared_ptr.hpp>

#include <msgpack/rpc/server.h>
#include <msgpack/rpc/client.h>

#include "msgpack_macro.h"

typedef std::string key;
typedef std::string value;

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

struct host{
	host(const std::string& name_, const uint16_t& port_)
		:hostname(name_),port(port_){}
	std::string hostname;
	uint16_t port;
	MSGPACK_DEFINE(hostname,port);
private:
	host();
};




// node infomation
class neighbor : public boost::noncopyable{
	key key_;
	msgpack::rpc::address ad_;
	//friend struct hash;
public:
	neighbor(const key& _key,const msgpack::rpc::address& _ad)
		:key_(_key),ad_(_ad){}
	const key& get_key()const{
		return key_;
	}
	const msgpack::rpc::address& get_address()const{
		return ad_;
	}
	struct hash {
		size_t operator()(const neighbor& a)const {
			return mp::hash<std::string>()(a.get_key()) +
				msgpack::rpc::address::hash()(a.get_address());
		};
	};
};

class range{
	key begin_,end_;
	bool border_begin_,border_end_; // true = contain, false = not
public:
	// construction
	range():begin_(),end_(),border_begin_(false),border_end_(false){}
	range(const range& org)
		:begin_(org.begin_),
		 end_(org.end_),
		 border_begin_(org.border_begin_),
		 border_end_(org.border_end_){}
	range(const key& _begin, const key& _end, bool _border_begin,bool _border_end)
		:begin_(_begin),end_(_end),border_begin_(_border_begin),border_end_(_border_end){}

	// checker
	bool contain(const key& t)const{
	if(begin_ == t && border_begin_) return true;
	if(end_ == t && border_end_) return true;
	if(begin_ < t && t < end_) return true;
	return false;
}
	MSGPACK_DEFINE(begin_,end_,border_begin_,border_end_)
};



class sg_node{
  value value_;
  std::vector<boost::shared_ptr<const neighbor> > next_keys[2]; // left=0, right=1
public:
  enum direction{
    left = 0,
    right = 1,
  };
  sg_node(const value& _value, const int level = settings::instance().maxlevel)
    :value_(_value){
    next_keys[left].reserve(level);
    next_keys[right].reserve(level);
  }
  sg_node(const sg_node& o)
    :value_(o.value_){
    next_keys[left].reserve(o.next_keys[left].size());
    next_keys[right].reserve(o.next_keys[right].size());
  }
  
  const std::string& get_value()const{ return value_;}
  void set_value(const value& v){value_ = v;}
  const std::vector<boost::shared_ptr<const neighbor> >* neighbors()const{
    return next_keys;
  }
private:
  sg_node();
};
typedef std::pair<key,sg_node> kvp;



struct membership_vector{
	uint64_t vector;
	membership_vector(uint64_t v):vector(v){}
	membership_vector():vector(){}
	membership_vector(const membership_vector& org):vector(org.vector){}
	int match(const membership_vector& o)const{
		const uint64_t matched = ~(vector ^ o.vector);
		uint64_t bit = 1;
		int cnt = 0;
		while((matched & bit) && cnt < 64){
			bit *= 2;
			cnt++;
		}
		return cnt;
	}
	void dump()const{
		const char* bits = reinterpret_cast<const char*>(&vector);
		for(int i=7;i>=0;--i){
			fprintf(stderr,"%02x",(unsigned char)255&bits[i]);
		}
	}
	MSGPACK_DEFINE(vector); // serialize and deserialize ok
};

namespace{
	void dump(uint64_t vec){
		const char* bits = reinterpret_cast<const char*>(&vec);
		for(int i=7;i>=0;--i){
			fprintf(stderr,"%02x",(unsigned char)(255&bits[i]));
		}
	}
}


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


#endif






