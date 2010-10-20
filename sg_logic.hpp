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
#include "logic_detail.hpp"

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
RPC_OPERATION3(treat, key, org_key,// key, target_key,
    host,origin, membership_vector, org_vector);
RPC_OPERATION5(introduce, key, org_key, key, target_key, host, origin,
	membership_vector, org_vector, int, level);
namespace logic{
const host& h = shared_data::instance().get_host();

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
	{
		std::pair<const key,sg_node>* nearest
			= detail::get_nearest_node(arg.set_key); // search nearest node
		if(nearest != NULL){ // something found!
			if(nearest->first == arg.set_key){ // already exist in this node
				shared_data::ref_storage st(shared_data::instance().storage);
				//(*st)[arg.set_key] = arg.set_value;
				st->erase(arg.set_key);
				st->insert(std::make_pair(arg.set_key,
						sg_node(arg.set_value, shared_data::instance().myvector,
							shared_data::instance().maxlevel)));
				req->result(true);
				return;
			}else{ // other node -> relay
				// search nearest
				boost::shared_ptr<const neighbor> locked_nearest
					= nearest->second.search_nearest(nearest->first,arg.set_key);
				//const direction dir = get_direction(nearest->first, arg.set_key);
				const host& localhost = shared_data::instance().get_host();
				const membership_vector& localmv
					(shared_data::instance().myvector);
				std::pair<key, sg_node> newcomer 
					(arg.set_key, sg_node(arg.set_value, localmv,
						shared_data::instance().maxlevel));
				
				if(!locked_nearest){
					// select some neighbor( it may not be efficient
					locked_nearest = (shared_data::instance().get_nearest_neighbor(arg.set_key));
				}
				{
					shared_data::ref_storage st(shared_data::instance().storage);
					st->insert(newcomer);
				}
				req->result(true); // insertion ok!
				if(locked_nearest){ // send treat
					sv->get_session(locked_nearest->get_address())
						.notify("treat", arg.set_key, locked_nearest->get_key(), 
							localhost, shared_data::instance().myvector);
				}
				/*
					if(nearest->second.neighbors()[dir][0]){
					shared_data::instance().storage_dump();
					const neighbor& target = *nearest->second.neighbors()
					[dir][0];
					sv->get_session(target.get_host().get_address())
					.notify("introduce", arg.set_key, target.get_key(),
					localhost, localmv, 0);

					sg_node& node = nearest->second;
					for(size_t i=0; i<node.neighbors()[dir].size();i++){
					node.neighbors()[dir][i].reset();
					}
					}
				*/
			}
		}else { // not found == it's first key!
			shared_data::ref_storage st(shared_data::instance().storage);
			st->insert(std::make_pair(arg.set_key,
					sg_node(arg.set_value, shared_data::instance().myvector,
						shared_data::instance().maxlevel))); 

			req->result(true);
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
			int org_distance = detail::string_distance(found_key,arg.target_key);
			bool target_is_right = true;
			++node;
			if((node != storage.end()) && 
				org_distance > detail::string_distance(node->first,arg.target_key)){
				--node;
			}else{
				target_is_right = false;
			}
			
			// search target
			direction left_or_right = target_is_right 
				? right : left;
			
			int i;
			assert(node->second.neighbors()[left_or_right].size() > 0);
			for(i = node->second.neighbors()[left_or_right].size()-1; i>=0; --i){
				if(node->second.neighbors()[left_or_right][i].get() != NULL &&
					(node->second.neighbors()[left_or_right][i]->get_key() == arg.target_key
						||
						((node->second.neighbors()[left_or_right][i]->get_key() <
							arg.target_key)
							^
							(left_or_right == right))
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
		shared_data::storage_t::iterator node = st->find(arg.target_key);
		if(node != st->end()){
			node->second.new_link
				(arg.level, get_direction(arg.target_key, arg.origin_key), 
					arg.origin_key, arg.origin);
		}else{
			assert(!"no key found");
		}
	}
}

template <typename request, typename server>
void treat(request* req, server* sv){
	BLOCK("treat:");
	msg::treat arg(req->params());
	std::pair<const key,sg_node>* nearest
		= detail::get_nearest_node(arg.org_key);
	if(nearest == NULL) return; // I have no key.
	else{
		DEBUG_OUT("treated by %s\n", nearest->first.c_str());
		if(nearest->first == arg.org_key){ // already exist in local -> introduce to both
			// FIXME
			assert(false);
		}else{
			boost::shared_ptr<const neighbor> locked_nearest
				= nearest->second.search_nearest(nearest->first,arg.org_key);
			if(locked_nearest){ // more near node exists -> relay
				
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
	shared_data::ref_storage st(shared_data::instance().storage);
	shared_data::storage_t::iterator it = st->find(arg.target_key);
	if(it == st->end()) { // not found key -> retry
		sv->get_session(arg.origin.get_address())
			.notify("introduce",arg.org_key, arg.org_key,
				arg.origin, arg.org_vector, 0);
		return;
	}
	const direction dir(get_direction(arg.target_key, arg.org_key));
	{
		const key& my_key(it->first);
		sg_node& node = it->second;// alias

		if(arg.origin == shared_data::instance().get_host()){ return; }
	
		const membership_vector my_mv = node.get_vector();
		const host& localhost(shared_data::instance().get_host());
		const int matches = 
			std::min(my_mv.match(arg.org_vector), shared_data::instance().maxlevel);
		int i = arg.level;
		if(i==0 || i < matches){
			boost::shared_ptr<const neighbor> as_neighbor
				= shared_data::instance().get_nearest_neighbor(arg.org_key);
			DEBUG_OUT("matched:%d\n", matches);
			for(;i<=matches; ++i){
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
}
#endif
