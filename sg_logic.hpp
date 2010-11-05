#ifndef SG_LOGIC_HPP
#define SG_LOGIC_HPP

#include <boost/function.hpp>
#include <map>
#include <string>
#include <algorithm>
#include <boost/functional/hash.hpp>

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
	{
	}
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
						if(from_left->get_key() == arg.set_key){
							DEBUG_OUT("itti! %s\n", result->first.c_str());
							const shared_neighbor& left_is 
								= shared_data::instance()
								.get_neighbor(its.first->first, localhost);
							while(i > 0 && its.first->second.neighbors()
										[right][i]->get_key() == arg.set_key)--i;
							for(;i<shared_data::instance().maxlevel;++i){
								its.first->second.neighbors()[right][i] = newnbr;
								//std::cout << "key" << newnbr->get_key().c_str();
								result->second.neighbors()[left][i] = left_is;
								std::cout << "key" << left_is->get_key().c_str();
							}
						}
						req->result(true);
						return;
					}else if(from_right && arg.set_key <= from_right->get_key()){
						DEBUG_OUT("from right treat");
						const neighbor* from_left = its.first->second.neighbors()[right][i].get();
						const neighbor* from_right = its.second->second.neighbors()[left][i].get();
						sv->get_session(from_right->get_address())
							.call("treat", arg.set_key, localhost, localvector);
						if(from_right->get_key() == arg.set_key){
							const shared_neighbor& right_is 
								= shared_data::instance()
								.get_neighbor(from_right->get_key(), localhost);
							for(;i<=shared_data::instance().maxlevel-1;++i){
								its.first->second.neighbors()[right][i] = newnbr;
								result->second.neighbors()[right][i] = right_is;
							}
						}
						if(from_right->get_key() == arg.set_key){
							DEBUG_OUT("itti! %d\n", i);
							const shared_neighbor& right_is 
								= shared_data::instance()
								.get_neighbor(its.first->first, localhost);
							while(i > 0 && its.first->second.neighbors()[right][i] &&
										its.first->second.neighbors()[right][i]->get_key() 
										== arg.set_key)--i;
							for(;i<shared_data::instance().maxlevel;++i){
								its.first->second.neighbors()[right][i] = newnbr;
								std::cout << "key" << newnbr->get_key().c_str();
								result->second.neighbors()[right][i] = right_is;
								std::cout << "key" << right_is->get_key().c_str();
							}
							req->result(true);
							return;
						}
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
			std::vector<size_t> vec = get_hash_of_lock(its);
			vec.push_back(key_hash()(arg.org_key));
			shared_data::multi_ref_storage locks(sst, vec);// get_locks
			
			if(its.first->next[0]->first != its.second->first){continue;}// relock
			shared_data::storage_t& st = shared_data::instance().storage.get_ref();

			if(its.second->first == arg.org_key){ // if I have that key
				if( its.first->first < arg.org_key 
					 && its.first->second.neighbors()[right][0]
					 && its.first->second.neighbors()[right][0]->get_key() 
					 == its.second->first
					 && arg.origin == localhost){
					shared_data::storage_t::iterator rhs = its.second;
					++rhs;
					detail::zip_two_node(its.first->first,&its.first->second
															 ,its.second->first, &its.second->second);
					detail::zip_two_node(its.second->first,&its.second->second
															 ,rhs->first, &rhs->second);
					req->result(true);
					return;
				}
				DEBUG_OUT("I have it!\n");
				const sg_node& target = its.second->second;
				if(target.neighbors()[left][0]){
					sv->get_session(target.neighbors()[left][0]->get_address())
						.call("introduce"
									, arg.org_key, target.neighbors()[left][0]->get_key()
									, arg.origin, arg.org_vector,0);
				}
				if(target.neighbors()[right][0]){
					sv->get_session(target.neighbors()[right][0]->get_address())
						.call("introduce"
									, arg.org_key, target.neighbors()[right][0]->get_key()
									, arg.origin, arg.org_vector,0);
				}
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
						std::min(localvector.match(arg.org_vector), localmaxlevel-1);
					if(from_left && from_left->get_key() > arg.org_key){
						sv->get_session(from_left->get_address())
							.call("introduce"
										, arg.org_key, from_left->get_key()
										, arg.origin, arg.org_vector,0);
						for(int i=0;i<=link_for;++i){
							its.first->second.neighbors()[right][i] = newnbr;							
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
							its.second->second.neighbors()[left][i] = newnbr;
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
					if(from_left && from_left->get_key() < arg.org_key){
						sv->get_session(from_left->get_address())
							.call("treat", arg.org_key, arg.origin, arg.org_vector);
						req->result(true);
						return;
					}else if(from_right && arg.org_key < from_right->get_key()){
						sv->get_session(from_right->get_address())
							.call("treat", arg.org_key, arg.origin, arg.org_vector);
						req->result(true);
						return;
					}
				}

				// there is no key between locked nodes
				const int link_for = 
					std::min(localvector.match(arg.org_vector), localmaxlevel-1);
				int i=0;
				if(link_for == 0){
					if(its.first->second.is_valid()){
						its.first->second.neighbors()[right][0] = newnbr;
						sv->get_session(arg.origin.get_address())
							.call("link", arg.org_key, 0, its.first->first, localhost);
					}
					if(its.second->second.is_valid()){
						its.second->second.neighbors()[left][0] = newnbr;
						sv->get_session(arg.origin.get_address())
							.call("link", arg.org_key, 0, its.second->first, localhost);
					}
					++i;
				}
								
				for(;i<=link_for;++i){
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
									, arg.origin, arg.org_vector,link_for+1);
				}
				if(its.second->second.is_valid() 
					 && its.second->second.neighbors()[right][0]){
					sv->get_session(its.second->second.neighbors()[right][0]->get_address())
						.call("introduce"
									, arg.org_key, its.second->second.neighbors()[right][0]->get_key()
									, arg.origin, arg.org_vector,link_for+1);
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
		//link(target_key, level, key, org_key, origin)
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
void introduce(request* req, server* sv){
	msg::introduce arg(req->params());
	DEBUG(std::cerr << arg << std::endl);
	{
		if(shared_data::instance().maxlevel <= arg.level){
			req->result(true);
			return;
		}
		
		const host& localhost = shared_data::instance().get_host();
		const membership_vector& localvector = shared_data::instance().myvector;
		const int& localmaxlevel = shared_data::instance().maxlevel;
		//introduce(org_key, target_key, origin, org_vector, level)
		shared_data::storage_t& st = shared_data::instance().storage.get_ref();
		
		shared_data::sync_storage_t& sst = shared_data::instance().storage;
		shared_data::ref_storage locks(sst, key_hash()(arg.target_key));
		shared_data::storage_t::iterator target = st.get(arg.target_key);
		if(target == st.end()){ req->result(false); return;}
		if(arg.org_key == arg.target_key){
			for(int i=0;i<2;i++){
				if(target->second.neighbors()[i][0]){
					const neighbor& nbr = *target->second.neighbors()[i][0];
					sv->get_session(nbr.get_address())
						.call("introduce", arg.org_key, nbr.get_key()
									, localhost, localvector,0);
				}
			}
			req->result(true);
			return;
		}
		const direction dir = get_direction(arg.target_key, arg.org_key);
		if(target->second.neighbors()[dir][arg.level]){
			const neighbor& target_point = *target->second.neighbors()[dir][arg.level];
			if((dir == left && target_point.get_key() < arg.org_key)
				 ||(dir == right && target_point.get_key() > arg.org_key)){
				sv->get_session(target_point.get_address())
					.call("introduce", arg.org_key, target_point.get_key()
								, arg.origin, arg.org_vector, arg.level);
				req->result(false);
			}
		}
		int lv = arg.level;
		const int link_for 
			= std::min(localvector.match(arg.org_vector), localmaxlevel-1);
		DEBUG_OUT("atai[%d]\n", link_for);
		const shared_neighbor nbr 
			= shared_data::instance().get_neighbor(arg.org_key, arg.origin);
		// if(arg.level == 0){
		// 	target->second.neighbors()[dir][0] = nbr;
		// 	sv->get_session(arg.origin.get_address())
		// 		.call("link", arg.org_key, 0, target->first, localhost);
		// 	lv++;
		// }

		if(nbr->get_host() == localhost){
			shared_data::ref_storage locks
				(shared_data::instance().storage, key_hash()(nbr->get_key()));
			shared_data::storage_t::iterator org_node = st.get(arg.org_key);
			const shared_neighbor target_node
				= shared_data::instance().get_neighbor(arg.target_key, localhost);
			DEBUG_OUT("hello");
			for(;lv <= link_for; ++lv){
				target->second.neighbors()[dir][lv] = nbr;
				org_node->second.neighbors()[inverse(dir)][lv] = target_node;
			}
		}else{
			for(;lv <= link_for; ++lv){
				target->second.neighbors()[dir][lv] = nbr;
				sv->get_session(arg.origin.get_address())
					.call("link", arg.org_key, lv, target->first, localhost);
			}
		}
		if(target->second.neighbors()[inverse(dir)][arg.level]){
			const neighbor& inv
				= *target->second.neighbors()[inverse(dir)][arg.level];
			sv->get_session(inv.get_address())
				.call("introduce", arg.org_key, inv.get_key(), 
							arg.origin, arg.org_vector, link_for+1);
		}
		req->result(true);
	}
}

template <typename request, typename server>
void search(request* req, server* sv){
	BLOCK("search:");
	msg::search arg(req->params());
	DEBUG(std::cerr << arg << std::endl);

	shared_data::sync_storage_t& sst = shared_data::instance().storage;
	const host& localhost = shared_data::instance().get_host();
	const membership_vector& localvector = shared_data::instance().myvector;
	const int& localmaxlevel = shared_data::instance().maxlevel;

	{//search(target_key, level, origin);

		shared_data::storage_t& st = shared_data::instance().storage.get_ref();
		shared_data::storage_t::iterator target = st.get(arg.target_key);		
		//if(!target->second.is_valid()){req->result(false);return;}
		if(target != st.end()){
			sv->get_session(arg.origin.get_address())
				.call("found", target->first, target->second.get_value());
			req->result(true);
			return;
		}else{
			shared_data::storage_t::iterator near = st.lower_bound(arg.target_key);
			if(near == st.begin()){++near;}
			if(near == st.end()){req->result(false);return;}
			const direction dir = get_direction(near->first, arg.target_key);
			
			for(int i=shared_data::instance().maxlevel-1; i>=0; --i){
				const neighbor* more_near = near->second.neighbors()[dir][i].get();
				if(more_near
					 && ((dir == left && more_near->get_key() < arg.target_key)
							 || 
							 (dir == right && arg.target_key <= more_near->get_key()))){
					sv->get_session(more_near->get_address())
						.call("search", arg.target_key, 10, arg.origin);
					req->result(true);
					return;
				}
			}
			req->result(true);
			sv->get_session(arg.origin.get_address())
				.call("notfound", arg.target_key);
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



}
#endif
