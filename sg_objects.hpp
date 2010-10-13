#ifndef SG_OBJECTS_HPP
#define SG_OBJECTS_HPP
#include <string>
#include <stdint.h>

#include <netinet/in.h>
#include <arpa/inet.h>

#include <boost/noncopyable.hpp>
#include <boost/unordered_map.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/weak_ptr.hpp>
#include <boost/optional.hpp>
#include <mp/sync.h>
#include "msgpack/rpc/server.h"
#include "msgpack/rpc/client.h"

#include "objlib.h"
#include "msgpack.hpp"

struct host{
	host(){}
	host(const std::string& name_, const uint16_t& port_)
		:hostname(name_),port(port_){}
	std::string hostname;
	uint16_t port;
	msgpack::rpc::ip_address get_address()const{
		return msgpack::rpc::ip_address(hostname,port);
	}
	static host get_host(const msgpack::rpc::ip_address& ad){
		sockaddr_in addr;
		ad.get_addr(reinterpret_cast<sockaddr*>(&addr));
		in_addr ip = addr.sin_addr;
		return host(inet_ntoa(ip), ad.get_port());
	}
	bool operator==(const host& rhs)const{
		return hostname == rhs.hostname && port == rhs.port;
	}
	bool operator!=(const host& rhs)const{
		return !(operator==(rhs));
	}
	MSGPACK_DEFINE(hostname,port);
private:
};

typedef std::string key;
typedef std::string value;


// node infomation
class neighbor {
	key key_;
	host ad_;
	//friend struct hash;
public:
	neighbor(const key& _key,const host& _ad)
		:key_(_key),ad_(_ad){}

	// getter
	const key& get_key()const{ return key_; }
	const host& get_host()const{ return ad_; }
	const msgpack::rpc::address get_address()const{ return ad_.get_address(); }

	/*
	struct hash {
		size_t operator()(const neighbor& a)const {
			return mp::hash<std::string>()(a.get_key()) +
				msgpack::rpc::address::hash()(a.get_address());
		};
		};*/
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

class sg_node;
class membership_vector;

struct shared_data: public singleton<shared_data>{
	int maxlevel;
	// initializer
	shared_data():maxlevel(8){}
	void init(){
		ref_storage rs(storage); rs->clear();
		ref_ng_map rn(ngmap); rn->clear();
	}
	// selfdata
	host myhost;
	void set_host(const std::string& name, const uint16_t& port){
		myhost = host(name,port);
	}
	const host& get_host()const{return myhost;}
 
	void storage_dump()const;
	
	// storage
	typedef std::map<key,sg_node> storage_t;
	typedef mp::sync<storage_t> sync_storage_t;
	typedef sync_storage_t::ref ref_storage;
	sync_storage_t storage;
	
	// neighbors
	typedef std::map<key, boost::weak_ptr<neighbor> > ng_map_t;
	typedef mp::sync<ng_map_t> sync_ng_map_t;
	typedef sync_ng_map_t::ref ref_ng_map;
	sync_ng_map_t ngmap;
	boost::shared_ptr<neighbor> get_neighbor(const key& k, const host& h);
	
};


class sg_node{
	value value_;
	std::vector<boost::shared_ptr<const neighbor> > next_keys[2]; // left=0, right=1
public:
	enum direction{
		left = 0,
		right = 1,
	};
	static direction inverse(const direction d){
		return static_cast<direction>(1 - static_cast<int>(d));
	}
	sg_node(const value& _value)
		:value_(_value){
		const int level = shared_data::instance().maxlevel;
		next_keys[left].reserve(level);
		next_keys[right].reserve(level);
		for(int i=0;i<level;i++){
			next_keys[left].push_back(boost::shared_ptr<const neighbor>());
			next_keys[right].push_back(boost::shared_ptr<const neighbor>());
		}
	}
	boost::shared_ptr<const neighbor>
	search_nearest(const key& mykey,const key& target)const{
		assert(mykey != target);
		direction dir = mykey < target ? right : left;
		for(int i = next_keys[dir].size()-1; i>=0; --i){
			if(next_keys[dir][i] != NULL && (
				 (target < next_keys[dir][i]->get_key() && dir == left) ||
				 (next_keys[dir][i]->get_key() < target && dir == right)
				 )
			){
				return next_keys[dir][i];
			}
		}
		return boost::shared_ptr<const neighbor>();
	}
	
	const std::string& get_value()const{ return value_;}
	void set_value(const value& v){value_ = v;}
	const std::vector<boost::shared_ptr<const neighbor> >* neighbors()const{
		return next_keys;
	}
	void new_link(int level, direction left_or_right, const key& k,
								const host& h){
		{
			next_keys[left_or_right][level] = shared_data::instance()
				.get_neighbor(k,h);
		}
	}
	size_t get_maxlevel()const{
		return next_keys[left].size();
	}
	//friend std::ostream& operator<<()
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

#endif
