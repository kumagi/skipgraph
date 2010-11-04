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
	
	shared_data::sync_storage_t& sst = shared_data::instance().storage;
	const host& localhost = shared_data::instance().get_host();
	const membership_vector& localvector = shared_data::instance().myvector;
	const int& localmaxlevel = shared_data::instance().maxlevel;
	{
		while(1){
			shared_data::both_side its = sst.get_ref().get_pair(arg.set_key);
		
			const std::vector<size_t>& vec = get_hash_of_lock(its);
			shared_data::multi_ref_storage locks(sst, vec);// get_locks
			
			if(its.first->next[0]->first != its.second->first){continue;} 
			shared_data::storage_t& st = shared_data::instance().storage.get_ref();		
			if(its.first->first == arg.set_key){ // if I have that key
				its.first->second.set_value(arg.set_value);
				req->result(true);
				return;
			}else{ // new key
				st.add(arg.set_key, sg_node(arg.set_value,localvector,localmaxlevel));
				shared_data::storage_t::iterator result = st.get(arg.set_key);
			
				shared_neighbor newnbr
					= shared_data::instance().get_neighbor(arg.set_key,localhost);
				const neighbor* from_left = its.first->second.neighbors()[right][0].get();
				const neighbor* from_right = its.second->second.neighbors()[left][0].get();
				if(from_left &&  from_left->get_key() != its.second->first){
					if(its.first->is_valid()
						&& from_left && from_left->get_key() > arg.set_key){
						DEBUG_OUT("from left %s < %s\n"
							, arg.set_key.c_str(), from_left->get_key().c_str());
						detail::zip_two_node(its.first->first,&its.first->second
							,arg.set_key,&result->second);
						sv->get_session(from_left->get_address())
							.call("introduce"
								,arg.set_key, from_left->get_key(), localhost, localvector,0);
						req->result(true);
						return;
					}
					else if(its.second->is_valid() 
						&& from_right && arg.set_key > from_right->get_key()){
						DEBUG_OUT("from right %s > %s"
							, arg.set_key.c_str(), from_left->get_key().c_str());
						detail::zip_two_node(arg.set_key,&result->second
							,its.first->first,&its.first->second);
						sv->get_session(from_left->get_address())
							.call("introduce",
								arg.set_key, from_right->get_key(), localhost, localvector,0);
						req->result(true);
						return;
					}
				}
				
				for(int i=shared_data::instance().maxlevel-1; i>=0; --i){
					const neighbor* from_left = its.first->second.neighbors()[right][i].get();
					const neighbor* from_right = its.second->second.neighbors()[left][i].get();
					if(from_left && from_left->get_key() <= arg.set_key){
						DEBUG_OUT("from left treat");
						sv->get_session(from_left->get_address())
							.call("treat", arg.set_key, localhost, localvector);
						req->result(true);
						return;
					}else if(from_right && arg.set_key <= from_right->get_key()){
						DEBUG_OUT("from right treat");
						sv->get_session(from_left->get_address())
							.call("treat", arg.set_key, localhost, localvector);
						req->result(true);
						return;
					}
				}
			
				// there is no key between locked nodes
				for(int i=shared_data::instance().maxlevel-1; i>=0; --i){
					its.first->second.neighbors()[right][i] = newnbr;
					its.second->second.neighbors()[left][i] = newnbr;
				}
				req->result(true);
				return;
			}
			break;
		}
	}
}

template <typename request, typename server>
void treat(request* req, server* sv){
	BLOCK("treat:");
	msg::treat arg(req->params());
	DEBUG(std::cerr << arg << std::endl);
	{
		shared_data::sync_storage_t& sst = shared_data::instance().storage;
		const host& localhost = shared_data::instance().get_host();
		const membership_vector& localvector = shared_data::instance().myvector;
		const int& localmaxlevel = shared_data::instance().maxlevel;
	
		while(1){
			shared_data::both_side its = sst.get_ref().get_pair(arg.org_key);
			const std::vector<size_t>& vec = get_hash_of_lock(its);
			shared_data::multi_ref_storage locks(sst, vec);// get_locks
			
			if(its.first->next[0]->first != its.second->first){continue;}// relock
			shared_data::storage_t& st = shared_data::instance().storage.get_ref();
					
			if(its.first->first == arg.org_key){ // if I have that key
				const sg_node& target = its.first->second;
				sv->get_session(target.neighbors()[left][0]->get_address())
					.call("introduce"
						, arg.org_key, target.neighbors()[left][0]->get_key()
						, arg.origin, arg.org_vector,0);
				sv->get_session(target.neighbors()[right][0]->get_address())
					.call("introduce"
						, arg.org_key, target.neighbors()[right][0]->get_key()
						, arg.origin, arg.org_vector,0);
				req->result(true);
				st.remove(arg.org_key);
				return;
			}else{

				// if near, begin to treat
				shared_neighbor newnbr = 
					shared_data::instance().get_neighbor(arg.org_key, arg.origin);
				const neighbor* from_left = its.first->second.neighbors()[right][0].get();
				const neighbor* from_right = its.second->second.neighbors()[left][0].get();
				if(from_left && from_left->get_key() != its.second->first){
					const int link_for = 
						std::min(localvector.match(arg.org_vector), localmaxlevel);
					if(from_left && from_left->get_key() > arg.org_key){
						sv->get_session(from_left->get_address())
							.call("introduce"
								, arg.org_key, from_left->get_key()
								, arg.origin, arg.org_vector,0);
						for(int i=0;i<link_for;++i){
							sv->get_session(arg.origin.get_address())
								.call("link", arg.org_key, i, its.first->first, localhost);
						}
						if(its.first->second.neighbors()[left][0]){
							const neighbor& target = *its.first->second.neighbors()[left][0];
							sv->get_session(target.get_address())
								.call("introduce"
									, arg.org_key, target.get_key()
									, arg.origin, arg.org_vector, link_for);
						}
						req->result(true);
						return;
					}
					else if(from_right && arg.org_key > from_right->get_key()){
						sv->get_session(from_right->get_address())
							.call("introduce"
								, arg.org_key, from_right->get_key()
								, arg.origin, arg.org_vector,0);
						for(int i=0;i<=link_for;++i){
							sv->get_session(arg.origin.get_address())
								.call("link", arg.org_key, i, its.second->first, localhost);
						}
						if(its.second->second.neighbors()[right][0]){
							const neighbor& target = *its.second->second.neighbors()[right][0];
							sv->get_session(target.get_address())
								.call("introduce"
									, arg.org_key, target.get_key()
									, arg.origin, arg.org_vector,link_for);
						}
						req->result(true);
						return;
					}
				}
				
				// relay treat
				for(int i=shared_data::instance().maxlevel-1; i>=0; --i){
					const neighbor* from_left = its.first->second.neighbors()[right][i].get();
					const neighbor* from_right = its.second->second.neighbors()[left][i].get();
					if(from_left && from_left->get_key() <= arg.org_key){
						sv->get_session(from_left->get_address())
							.call("treat", arg.org_key, arg.origin, arg.org_vector);
						req->result(true);
						return;
					}else if(from_right && arg.org_key <= from_right->get_key()){
						sv->get_session(from_left->get_address())
							.call("treat", arg.org_key, arg.origin, arg.org_vector);
						req->result(true);
						return;
					}
				}

				
				// there is no key between locked nodes
				const int link_for = 
					std::min(localvector.match(arg.org_vector), localmaxlevel);
				for(int i=0;i<=link_for;++i){
					if(its.first->second.is_valid()){
						its.first->second.neighbors()[right][i] = newnbr;
						sv->get_session(arg.origin.get_address())
							.call("link", arg.org_key, i, its.first->first, localhost);
					}
					if(its.second->second.is_valid()){
						its.second->second.neighbors()[left][i] = newnbr;
						sv->get_session(arg.origin.get_address())
							.call("link", arg.org_key, i, its.second->first, localhost);
					}
				}
				if(its.first->second.is_valid()
					&& its.first->second.neighbors()[left][0]){
					sv->get_session(its.first->second.neighbors()[left][0]->get_address())
						.call("introduce"
							, arg.org_key, its.first->second.neighbors()[left][0]->get_key()
							, arg.origin, arg.org_vector,0);
				}
				if(its.second->second.is_valid() 
					&& its.second->second.neighbors()[right][0]){
					sv->get_session(its.second->second.neighbors()[right][0]->get_address())
						.call("introduce"
							, arg.org_key, its.second->second.neighbors()[right][0]->get_key()
							, arg.origin, arg.org_vector,0);
				}
				req->result(true);			
			}
			break;
		}	
	}
}

template <typename request, typename server>
void link(request* req,server* sv){
	BLOCK("link:");
	const msg::link arg(req->params());
	DEBUG(std::cerr << arg << std::endl);
	{
		//link(link, target_key, level, key, org_key, origin)
		shared_data::storage_t& st = shared_data::instance().storage.get_ref();
		shared_data::storage_t::iterator target = st.get(arg.target_key);
		if(target == st.end()) req->result(false);

		const direction dir = get_direction(arg.target_key, arg.org_key);
		const shared_neighbor& new_nbr 
			= shared_data::instance().get_neighbor(arg.org_key, arg.origin);
		target->second.neighbors()[dir][arg.level] = new_nbr;
	}

}

template <typename request, typename server>
void search(request* req, server* sv){
	BLOCK("search:");
	msg::search arg(req->params());
	DEBUG(std::cerr << arg << std::endl);
	{
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
void introduce(request* req, server* sv){
	msg::introduce arg(req->params());
	DEBUG(std::cerr << arg << std::endl);
	{
	}
}

}
#endif
