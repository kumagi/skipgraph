#ifndef SG_LOGIC_HPP
#define SG_LOGIC_HPP

#include <boost/function.hpp>
#include <map>
#include <string>

#define DEBUG_MODE
#include "debug_macro.h"
#include "msgpack_macro.h"

#include <boost/function.hpp>
#include <boost/any.hpp>

#include "sg_objects.hpp"
#include "msgpack.hpp"

#include "objlib.h"

typedef std::string key;
typedef std::string value;

template <typename Server> 
class reflection{
public:
	typedef typename boost::function <void
		(msgpack::rpc::request*, Server*)> reaction;
	typedef std::map<std::string, reaction> table;
private:
	table func_table;
public:
	void regist(const std::string& name, reaction func){
		assert(func_table.find(name) == func_table.end());
		typedef std::pair<typename table::iterator, bool> its;
		its it = func_table.insert(std::make_pair(name,func));
		assert(it.second != false && "double insertion");
	}
	bool call(const std::string& name, msgpack::rpc::request* req,
						Server* sv)const{
		typename table::const_iterator it = func_table.find(name);
		if(it == func_table.end()) return false;
		it->second(req,sv);
		return true;
	}
	//const name_to_func* get_table_ptr(void)const{return &table;};
};

RPC_OPERATION1(die, std::string, name);
RPC_OPERATION2(set, key, set_key, value, set_value);
RPC_OPERATION4(link, key, target_key, int, level, key, origin_key, host, origin);
RPC_OPERATION3(search, key, target_key, int, level, host, origin);
RPC_OPERATION2(found, key, found_key, value, found_value);
RPC_OPERATION1(notfound, key, target_key);
RPC_OPERATION2(range_search, range, target_range, host, origin);
RPC_OPERATION3(range_found, range, target_range, std::vector<key>,keys, std::vector<value>,values);
RPC_OPERATION2(range_notfound, range, target_range, std::vector<key>, keys);
RPC_OPERATION4(treat, key, org_key, key, target_key, host,
							 origin, membership_vector, org_vector);
RPC_OPERATION5(introduce, key, org_key, key, target_key, host, origin,
							 membership_vector, org_vector, int, level);

int string_distance(const std::string& lhs, const std::string& rhs){
	int len = lhs.length();
	for(int i=0;i<len;++i){
		if(lhs[i] != rhs[i]) return std::abs(lhs[i] - rhs[i]);
	}
	return 0;
}
bool left_is_near(const key& org, const key& lhs, const key& rhs){
	return string_distance(org,lhs) < string_distance(org, rhs);
}
key& which_near(const key& org,key& lhs, key& rhs){
	return left_is_near(org,lhs,rhs)
		? lhs : rhs;
}
const key& which_near(const key& org, const key& lhs, const key& rhs){
	return left_is_near(org,lhs,rhs)
		? lhs : rhs;
}


namespace logic{

namespace detail{
typedef shared_data::storage_t::iterator st_iter;
using boost::optional;

std::pair<st_iter, st_iter>
it_and_next(const st_iter& org){
	st_iter next = org;
	return std::make_pair(org,++next);
}

const std::pair<const key,sg_node>*
get_nearest_node(const key& k){
	typedef const optional<sg_node> result;
	shared_data::ref_storage st(shared_data::instance().storage);
	
	st_iter iter = st->lower_bound(k);
	if(iter == st->end()) { // most left data
		iter = st->upper_bound(k);
		if(iter == st->end()) {return NULL;}
		else{return &*iter;}
	}else if(iter->first == k){
		return &*iter;
	}
	
	const std::pair<st_iter, st_iter> lr_pair
		= it_and_next(iter);
	
	if(lr_pair.second == st->end()){// next key exists
		return &*lr_pair.first;
	}
	
	if(left_is_near(k, lr_pair.first->first, lr_pair.second->first))
		return &*lr_pair.first;
	else
		return &*lr_pair.second;
}
} // namespace detail
template <typename request, typename server>
void die(request*, server*){
	REACH("die:");
//	fprintf(stderr,"die called ok");
	exit(0);
};

template <typename request, typename server>
void set(request* req, server* sv){
	BLOCK("set");
	const msg::set arg(req->params());
	const host& h = shared_data::instance().get_host();
	{
		const std::pair<const key,sg_node>* nearest
			= detail::get_nearest_node(arg.set_key);
		if(nearest != NULL){
			if(nearest->first == arg.set_key){
			}
			boost::shared_ptr<const neighbor> locked_nearest
				= nearest->second.search_nearest(nearest->first,arg.set_key);
			assert(locked_nearest->get_host() != h);
			
			sv->get_session(locked_nearest->get_address())
				.notify("treat", arg.set_key, locked_nearest->get_key(), 
					h, membership_vector());
		}else {
			
		}
		DEBUG_OUT(" key:%s  value:%s  stored. ",
			arg.set_key.c_str(),arg.set_value.c_str());
	}

};

template <typename request, typename server>
void search(request* req, server* sv){
	BLOCK("search:");
	msg::search arg(req->params());
		
	shared_data::ref_storage ref(shared_data::instance().storage);
	shared_data::storage_t& storage = *ref;
  
	shared_data::storage_t::const_iterator node = storage.find(arg.target_key);
	if(node == storage.end()){ // this node does not have target key
		node = storage.lower_bound(arg.target_key);
		if(node == storage.end()){
			node = storage.upper_bound(arg.target_key);
			
			sv->get_session
				(msgpack::rpc::ip_address(arg.origin.hostname,arg.origin.port))
				.notify("notfound",arg.target_key);

		}else{
			const key& found_key = node->first;
			int org_distance = string_distance(found_key,arg.target_key);
			bool target_is_right = true;
			++node;
			if((node != storage.end()) && 
				org_distance > string_distance(node->first,arg.target_key)){
				--node;
			}else{
				target_is_right = false;
			}
			
			// search target
			sg_node::direction left_or_right = target_is_right 
				? sg_node::right : sg_node::left;
			
			int i;
			assert(node->second.neighbors()[left_or_right].size() > 0);
			for(i = node->second.neighbors()[left_or_right].size()-1; i>=0; --i){
				if(node->second.neighbors()[left_or_right][i].get() != NULL &&
					(node->second.neighbors()[left_or_right][i]->get_key() == arg.target_key
						||
						((node->second.neighbors()[left_or_right][i]->get_key() <
							arg.target_key)
							^
							(left_or_right == sg_node::right))
					)
				){ // relay query
					sv->get_session
						(node->second.neighbors()[left_or_right][i]->get_address())
						.notify("search",arg.target_key,i,arg.origin);
					return;
				}
			}
			if(i < 0){
				sv->get_session
					(msgpack::rpc::ip_address(arg.origin.hostname,arg.origin.port))
					.notify("notfound",arg.target_key);
			}
		}
	}else{ // found it
		sv->get_session
			(msgpack::rpc::ip_address(arg.origin.hostname,arg.origin.port))
			.notify("found",arg.target_key,node->second.get_value());
	}
}

template <typename request, typename server>
void found(request* req, server*){
	BLOCK("found:");
	const msg::found arg(req->params());
	std::cerr << "key:" <<
		arg.found_key << " & value:" << arg.found_value << " " << std::endl;
}
template <typename request, typename server>
void notfound(request* req, server*){
	BLOCK("found:");
	const msg::notfound arg(req->params());
	std::cerr << "key:" <<
		arg.target_key << " " << std::endl;
}

template <typename request, typename server>
void link(request* req,server*){
	BLOCK("link:");
	const msg::link arg(req->params());
	{
		shared_data::ref_storage st(shared_data::instance().storage);
		shared_data::storage_t& storage = *st;
		sg_node& node = storage.find(arg.target_key)->second;
		
		//assert(node != storage.end());
		sg_node::direction left_or_right = arg.target_key < arg.origin_key 
			? sg_node::right : sg_node::left;
		node.new_link
			(arg.level, left_or_right, arg.origin_key,arg.origin);
	}
}
template <typename request, typename server>
void treat(request* req, server* sv){
	BLOCK("treat:");
	msg::treat arg(req->params());
	
	shared_data::ref_storage ref(shared_data::instance().storage);
	shared_data::storage_t& storage = *ref;
  
	
	shared_data::storage_t::const_iterator node = storage.find(arg.target_key);
	if(node == storage.end()){ // this node does not have target key
		node = storage.lower_bound(arg.target_key);
		if(node == storage.end()){
			node = storage.upper_bound(arg.target_key);
			
			sv->get_session
				(msgpack::rpc::ip_address(arg.origin.hostname,arg.origin.port))
				.notify("notfound",arg.target_key);

		}else{
			const key& found_key = node->first;
			int org_distance = string_distance(found_key,arg.target_key);
			bool target_is_right = true;
			++node;
			if((node != storage.end()) && 
				org_distance > string_distance(node->first,arg.target_key)){
				--node;
			}else{
				target_is_right = false;
			}
			
			// search target
			sg_node::direction left_or_right = target_is_right 
				? sg_node::right : sg_node::left;
			
			int i;
			assert(node->second.neighbors()[left_or_right].size() > 0);
			for(i = node->second.neighbors()[left_or_right].size()-1; i>=0; --i){
				if(node->second.neighbors()[left_or_right][i].get() != NULL &&
					(node->second.neighbors()[left_or_right][i]->get_key() == arg.target_key
						||
						((node->second.neighbors()[left_or_right][i]->get_key() <
							arg.target_key)
							^
							(left_or_right == sg_node::right))
					)
				){ // relay query
					sv->get_session
						(node->second.neighbors()[left_or_right][i]->get_address())
						.notify("search",arg.target_key,i,arg.origin);
					return;
				}
			}
			if(i < 0){
				sv->get_session
					(msgpack::rpc::ip_address(arg.origin.hostname,arg.origin.port))
					.notify("notfound",arg.target_key);
			}
		}
	}else{ // found it
		sv->get_session
			(msgpack::rpc::ip_address(arg.origin.hostname,arg.origin.port))
			.notify("found",arg.target_key,node->second.get_value());
	}
}


}
#endif




