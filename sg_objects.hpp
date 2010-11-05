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
#include <boost/utility.hpp>
#include <boost/io/ios_state.hpp>
#include <boost/functional/hash.hpp>
#include <mp/sync.h>
#include "msgpack/rpc/server.h"
#include "msgpack/rpc/client.h"

#include "objlib.h"
#include "debug_macro.h"
#include "msgpack.hpp"

#include "sl.hpp"
#include "striped_sync.hpp"

enum direction{
left = 0,
	right = 1,
};



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
std::ostream&  operator<<(std::ostream& ost, const host& h);


struct membership_vector{
	uint64_t vector;
	explicit membership_vector(uint64_t v):vector(v){}
	membership_vector():vector(0){}
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
	bool operator==(const membership_vector& rhs)const
	{ return vector == rhs.vector;}
	MSGPACK_DEFINE(vector); // serialize and deserialize ok
};
std::ostream&  operator<<(std::ostream& ost, const membership_vector& v);

typedef std::string key;
typedef boost::hash<key> key_hash;
typedef std::string value;

static key min_key("0");
static key max_key("~~~~~");

std::ostream& operator<<(std::ostream& ost, const std::vector<key>& v);

direction get_direction(const key& lhs, const key& rhs);
direction inverse(const direction& d);

// node infomation
class neighbor {
	key key_;
	host ad_;
	//friend struct hash;
public:
	explicit neighbor(const key& _key,const host& _ad = host())
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
typedef boost::shared_ptr<neighbor> shared_neighbor;

class range{
	key begin_,end_;
	bool border_begin_,border_end_; // true = contain, false = not
	friend std::ostream&  operator<<(std::ostream& ost, const range& r);
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
std::ostream&  operator<<(std::ostream& ost, const range& r);

class sg_node;

class sg_node{
	value value_;
	std::vector<boost::shared_ptr<const neighbor> > next_keys[2]; // left=0, right=1
	membership_vector vec_;
	bool valid;
public:
	sg_node():valid(false){
		 next_keys[left].reserve(64);
		 next_keys[right].reserve(64);
		 for(int i=0;i<64;i++){
			 next_keys[left].push_back(boost::shared_ptr<const neighbor>());
			 next_keys[right].push_back(boost::shared_ptr<const neighbor>());
		 }
	}
	void set_vector(const membership_vector& mv){
		vec_ = mv;
	}
	const membership_vector& get_vector()const{ return vec_;}
	sg_node(const value& _value, const membership_vector& mv, int level)
		:value_(_value),vec_(mv),valid(true){
		next_keys[left].reserve(level);
		next_keys[right].reserve(level);
		for(int i=0;i<level;i++){
			next_keys[left].push_back(boost::shared_ptr<const neighbor>());
			next_keys[right].push_back(boost::shared_ptr<const neighbor>());
		}
	}
	bool is_valid()const{return valid;}
	
	boost::shared_ptr<const neighbor>
	search_nearest(const key& mykey,const key& target)const{
		assert(mykey != target);
		direction dir = mykey < target ? right : left;
		for(int i = next_keys[dir].size()-1; i>=0; --i){
			if(next_keys[dir][i] != NULL && (
					 (target <= next_keys[dir][i]->get_key() && dir == left) ||
					 (next_keys[dir][i]->get_key() <= target && dir == right)
				 )
			){
				return next_keys[dir][i];
			}
		}
		return boost::shared_ptr<const neighbor>();
	}
	
	const std::string& get_value()const{ return value_;}
	void set_value(const value& v){value_ = v;}
	std::vector<boost::shared_ptr<const neighbor> >* neighbors(){
		return next_keys;
	}
	const std::vector<boost::shared_ptr<const neighbor> >* neighbors()const{
		return next_keys;
	}
//void new_link(int level, direction left_or_right, const key& k,
//								const host& h){
//		next_keys[left_or_right][level] = shared_data::instance()
//			.get_neighbor(k,h);
//	}
	size_t get_maxlevel()const{
		return next_keys[left].size();
	}
	//friend std::ostream& operator<<()
private:
	
	sg_node& operator=(const sg_node&);
};
//struct suspended_node;


struct shared_data: public singleton<shared_data>{
	int maxlevel;
// initializer
	shared_data():maxlevel(8),storage(min_key,max_key){}
	void init(){
		ref_storage rs(storage,0); rs->clear();
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
	typedef sl<key,sg_node> storage_t;
	typedef std::pair<sl<key,sg_node>::iterator,sl<key,sg_node>::iterator> both_side;
	typedef striped_sync<storage_t,32> sync_storage_t;
	typedef sync_storage_t::ref ref_storage;
	typedef sync_storage_t::multi_ref multi_ref_storage;
	sync_storage_t storage;

	/* neighbors */
	typedef boost::unordered_map<key, boost::weak_ptr<neighbor> > ng_map_t;
	typedef mp::sync<ng_map_t> sync_ng_map_t;
	typedef sync_ng_map_t::ref ref_ng_map;
	sync_ng_map_t ngmap;
	
	boost::shared_ptr<neighbor> get_neighbor(const key& k, const host& h = host());
	boost::shared_ptr<const neighbor> get_nearest_neighbor(const key& k);
	
	/*
	// susupended storage
	typedef boost::unordered_map<key, suspended_node> suspended_t;
	typedef mp::sync<suspended_t> sync_suspended_t;
	typedef sync_suspended_t::ref ref_suspended;
	sync_suspended_t suspended;
	*/
	
	// membership_vector
	membership_vector myvector;
};
std::vector<size_t> get_hash_of_lock(shared_data::both_side& its);

std::ostream& operator<<(std::ostream& ost, shared_data& s);

/*
struct suspended_node: public sg_node{
	bool con[2];
	explicit suspended_node(const value& v, const membership_vector& mv, int lv)
		:sg_node(v,mv,lv){
		con[0] = false, con[1] = false;
	}
	
	void set_con(const key& self, const key& org){
		con[get_direction(self, org)] = true;
	}
	bool is_complete()const{ return con[0] & con[1]; }

	sg_node en_node(){
		assert((con[0] & con[1]) == true);
		return sg_node(*this);
	}
};
std::ostream& operator<<(std::ostream& ost, const suspended_node& sn);
*/

typedef std::pair<key,sg_node> kvp;



#endif
