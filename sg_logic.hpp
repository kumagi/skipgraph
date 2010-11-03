#ifndef SG_LOGIC_HPP
#define SG_LOGIC_HPP

#include <boost/function.hpp>
#include <map>
#include <string>
#include <algorithm>

#define DEBUG_MODE
#include "debug_macro.h"
#include "msgpack_macro.h"

#include <boost/function.hpp>
#include <boost/any.hpp>

#include "sg_objects.hpp"
#include "msgpack.hpp"

#include "objlib.h"
#include "logic_detail.hpp"

typedef std::string key;
typedef std::string value;

template <typename Server> 
class reflection{
public:
	typedef typename boost::function <void(msgpack::rpc::request*, Server*)>
	reaction;
	typedef std::map<std::string, reaction> table;
private:
	table func_table;
public:
	void regist(const std::string& name, reaction func){
		assert(func_table.find(name) == func_table.end() &&"you can't register one function two times");
		typedef std::pair<typename table::iterator, bool> its;
		its it = func_table.insert(std::make_pair(name,func));
		assert(it.second != false && "double insertion");
	}
	bool call(const std::string& name, msgpack::rpc::request* req,
		Server* sv)const{
		typename table::const_iterator it = func_table.find(name);
		if(it == func_table.end()) {
			assert(!"undefined function calll");
			return false;
		}
		it->second(req,sv);
		return true;
	}
	//const name_to_func* get_table_ptr(void)const{return &table;};
};

RPC_OPERATION1(die, std::string, name);
RPC_OPERATION1(dump, std::string, name);
RPC_OPERATION2(set, key, set_key, value, set_value);
RPC_OPERATION4(link, key, target_key, int, level, key, org_key, host, origin);

RPC_OPERATION2(found, key, found_key, value, found_value);
RPC_OPERATION1(notfound, key, target_key);
RPC_OPERATION2(range_search, range, target_range, host, origin)
//RPC_OPERATION3(range_found, range, target_range, std::vector<key>,keys, std::vector<value>,values);
//RPC_OPERATION2(range_notfound, range, target_range, std::vector<key>, keys);
RPC_OPERATION3(treat, key, org_key,host,origin, membership_vector, org_vector);
RPC_OPERATION5(introduce, key, org_key, key, target_key, host, origin,
	membership_vector, org_vector, int, level);

RPC_OPERATION3(search, key, target_key, int, level, host, origin);
namespace logic{
const host& h = shared_data::instance().get_host();

template <typename request, typename server>
void range_search(request* req, server* sv){
	BLOCK("search:");
	msg::range_search arg(req->params());
	DEBUG(std::cerr << arg << std::endl);
	
	
}

template <typename request, typename server>
void die(request* req, server*){
	REACH("die:");
	msg::die arg(req->params());
	DEBUG(std::cerr << arg << std::endl);
	//	fprintf(stderr,"die called ok");
	exit(0);
};
template <typename request, typename server>
void dump(request*, server*){
	std::cerr << std::endl;
	BLOCK("dump");
	std::cerr << std::endl << shared_data::instance();
};

template <typename request, typename server>
void set(request* req, server* sv){
	BLOCK("set");
	const msg::set arg(req->params());
	
}

template <typename request, typename server>
void search(request* req, server* sv){
	BLOCK("search:");
	msg::search arg(req->params());
	DEBUG(std::cerr << arg << std::endl);
	shared_data::ref_storage st(shared_data::instance().storage);
	shared_data::storage_t::iterator node = st->get(arg.target_key);
	if(node != st->end()) { // found it
		DEBUG_OUT("found!");
		sv->get_session
			(msgpack::rpc::ip_address(arg.origin.hostname,arg.origin.port))
			.notify("found",arg.target_key,node->second.get_value());
	}else{// this node does not have target key
		st.reset();
		DEBUG_OUT("relay...");
		const std::pair<const key,sg_node>* nearest =
			detail::get_nearest_node(arg.target_key);
		assert(nearest && "save something before search!");
		// search target
		//const direction dir = get_direction(nearest->first, arg.target_key);
		
		boost::shared_ptr<const neighbor> locked_nearest
			= nearest->second.search_nearest(nearest->first,arg.target_key);
		if(locked_nearest){ // more near node exists -> relay
			sv->get_session(locked_nearest->get_address())
				.notify("search", arg.target_key, 100, arg.origin );
		}else{
			sv->get_session
				(msgpack::rpc::ip_address(arg.origin.hostname,arg.origin.port))
				.notify("notfound",arg.target_key);
		}
	}
}

template <typename request, typename server>
void found(request* req, server*){
	BLOCK("found:");
	const msg::found arg(req->params());
	DEBUG(std::cerr << arg << std::endl);
	std::cerr << "key:" <<
		arg.found_key << " & value:" << arg.found_value << " " << std::endl;
}

template <typename request, typename server>
void notfound(request* req, server*){
	BLOCK("found:");
	const msg::notfound arg(req->params());
	DEBUG(std::cerr << arg << std::endl);
	std::cerr << "key:" <<
		arg.target_key << " " << std::endl;
}

template <typename request, typename server>
void link(request* req,server* sv){
	BLOCK("link:");
	const msg::link arg(req->params());
	DEBUG(std::cerr << arg << std::endl);
	const host& localhost(shared_data::instance().get_host());
	const direction dir = get_direction(arg.target_key, arg.org_key);
	
	shared_data::ref_storage st(shared_data::instance().storage);
	shared_data::storage_t::iterator iter = st->geta(arg.target_key);
	if(iter != st->end()){
		const shared_data::storage_t::iterator next = (dir == left)
			? boost::prior(iter) :boost::next(iter);
		if((dir == left && arg.target_key < iter->first) ||
			(dir == right && iter->first < arg.target_key)){
			// if more near key exist in this node
			iter->second.new_link(arg.level, dir,arg.org_key, arg.origin);
			sv->get_session(arg.origin.get_address())
				.notify("link", arg.org_key, arg.level, iter->first, localhost);
			return;
		}
		if(!iter->second.neighbors()[dir][arg.level] ||
			detail::left_is_near(arg.target_key, arg.org_key,
				iter->second.neighbors()[dir][arg.level]->get_key())){
			iter->second.new_link(arg.level, dir,arg.org_key, arg.origin);
		}else if(iter->second.neighbors()[dir][arg.level]){
			const neighbor& target = *iter->second.neighbors()[dir][arg.level];
			sv->get_session(target.get_address())
				.notify("link", arg.org_key, arg.level, target.get_key(), arg.origin);
		}
	}
}

template <typename request, typename server>
void treat(request* req, server* sv){
	BLOCK("treat:");
	msg::treat arg(req->params());
	DEBUG(std::cerr << arg << std::endl);
	std::pair<const key,sg_node>* nearest
		= detail::get_nearest_node(arg.org_key);
	if(nearest == NULL) {DEBUG_OUT("I have no key");return;} // I have no key.
	else{
		DEBUG_OUT("treated by [%s]\n", nearest->first.c_str());
		if(nearest->first == arg.org_key){ // already exist in local -> introduce to both
			bool flag = false;
			shared_data::ref_storage st(shared_data::instance().storage);
			sg_node& node = nearest->second;
			{
				const boost::optional<std::pair<key, host> > nearest_node
					= detail::nearest_node_info(arg.org_key, node, left, st);
				if(nearest_node){
					sv->get_session(nearest_node->second.get_address())
						.notify("introduce", arg.org_key, nearest_node->first,
							arg.origin, arg.org_vector,0);
					flag = true;
				}
			}
			{
				const boost::optional<std::pair<key, host> > nearest_node
					= detail::nearest_node_info(arg.org_key, node, right, st);
				if(nearest_node){
					sv->get_session(nearest_node->second.get_address())
						.notify("introduce", arg.org_key, nearest_node->first,
							arg.origin, arg.org_vector,0);
					flag = true;
				}
			}
			if(arg.origin != shared_data::instance().get_host()){
				if(!flag){
					boost::shared_ptr<const neighbor> locked_nearest =
						shared_data::instance().get_nearest_neighbor(arg.org_key);
					sv->get_session(locked_nearest->get_address())
						.notify("treat", arg.org_key,
							arg.origin, arg.org_vector);
				}
				st->erase(arg.org_key);
			}
		}else{
			// relay
			boost::shared_ptr<const neighbor> locked_nearest
				= nearest->second.search_nearest(nearest->first,arg.org_key);
			if(locked_nearest){ // more near node exists -> relay
				sv->get_session(locked_nearest->get_address())
					.notify("treat", arg.org_key, arg.origin, arg.org_vector);
			}else{
				const membership_vector my_mv = nearest->second.get_vector();
				const int matches = my_mv.match(arg.org_vector);
				direction dir =
					get_direction(nearest->first, arg.org_key);
				
				// introduce to old node
				shared_data::ref_storage st(shared_data::instance().storage);
				
				const boost::optional<std::pair<key,host> > nearest_node = 
					detail::nearest_node_info(nearest->first, nearest->second,
						dir, st);
				if(nearest_node){
					DEBUG_OUT("1nearest_node->first = %s\n", nearest_node->first.c_str());
					sv->get_session(nearest_node->second.get_address())
						.notify("introduce", arg.org_key, nearest_node->first,
							arg.origin, arg.org_vector,0);
				}
				
				// linking to newnode
				int i=0;
				for(; i <= matches && i < shared_data::instance().maxlevel; ++i){
					DEBUG_OUT("link from %s to %s\n", nearest->first.c_str(), arg.org_key.c_str());
					std::cout << "address:" << arg.origin << std::endl;;
					sv->get_session(arg.origin.get_address())
						.notify("link", arg.org_key, i,
							nearest->first, h);
					nearest->second.new_link
						(i, dir , arg.org_key, arg.origin);
				}
				
				// introduce to opposite
				if(i >= shared_data::instance().maxlevel) return;
				{
					const boost::optional<std::pair<key,host> > nearest_node = 
						detail::nearest_node_info(nearest->first, nearest->second,
							inverse(dir), st);
					if(nearest_node){
						DEBUG_OUT("2nearest_node->first = %s\n", nearest_node->first.c_str());
						sv->get_session(nearest_node->second.get_address())
							.notify("introduce", arg.org_key, nearest_node->first,
								arg.origin, arg.org_vector,i);
					}
				}
			}
		}
	}
}
	
template <typename request, typename server>
void introduce(request* req, server* sv){
	msg::introduce arg(req->params());
	DEBUG(std::cerr << arg << std::endl);
	shared_data::ref_storage st(shared_data::instance().storage);
	shared_data::storage_t::iterator it = st->find(arg.target_key);
	if(it == st->end()) { // not found key -> retry
		sv->get_session(arg.origin.get_address())
			.notify("introduce",arg.org_key, arg.org_key,
				arg.origin, arg.org_vector, 0);
		return;
	}
	const direction dir(get_direction(arg.target_key, arg.org_key));

	// alias
	const key& my_key(it->first);
	sg_node& node = it->second;// alias

	const membership_vector my_mv = node.get_vector();
	const host& localhost(shared_data::instance().get_host());
	const int matches = 
		std::min(my_mv.match(arg.org_vector), shared_data::instance().maxlevel);
	
	if(arg.origin == localhost){
		// FIXME : it must retry treating again
		return;
	}
	
	int i = arg.level;
	if(i==0 || i < matches){
		boost::shared_ptr<const neighbor> as_neighbor
			= shared_data::instance().get_nearest_neighbor(arg.org_key);
		const int maxloop = std::min(matches, shared_data::instance().maxlevel-1);
		DEBUG_OUT("matched:%d\n", matches);
		for(;i<=maxloop; ++i){
			node.new_link(i, dir , arg.org_key, arg.origin);
			sv->get_session(arg.origin.get_address())
				.notify("link", arg.org_key, i, my_key, localhost);
		}
	}
	if(i < shared_data::instance().maxlevel){
		const boost::optional<std::pair<key, host> > nearest_node
			= detail::nearest_node_info(my_key, node, inverse(dir), st);
		if(nearest_node){
			std::cout << nearest_node->first << ":" << nearest_node->second;
			sv->get_session(nearest_node->second.get_address())
				.notify("introduce", arg.org_key, nearest_node->first,
					arg.origin, arg.org_vector,i);
		}
	}
	
}

}
#endif
