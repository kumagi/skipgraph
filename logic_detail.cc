#include "logic_detail.hpp"
#include <map>
#include <boost/utility.hpp>

namespace logic{namespace detail{

typedef shared_data::storage_t::iterator st_iter;
int string_distance(const std::string& lhs, const std::string& rhs){
	int len = lhs.length();
	for(int i=0;i<len;++i){
		if(lhs[i] != rhs[i]) return std::abs(lhs[i] - rhs[i]);
	}
	return 0;
}
bool left_is_near(const key& org, const key& lhs, const key& rhs){
	assert(!(org<lhs && rhs<org));
	assert(!(org<rhs && lhs<org));
	assert(org != rhs);
	assert(org != lhs);
	if(org < lhs){
		if(lhs < rhs) return true;
		if(org < rhs) return false;
	}else{
		if(rhs < lhs) return true;
		if(rhs < org) return false;
	}
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

std::pair<st_iter, st_iter>
it_and_next(const st_iter& org){
	st_iter next = org;
	return std::make_pair(org,++next);
}

std::pair<const key,sg_node>*
//shared_data::storage_t::iterator
get_nearest_node(const key& k){
	shared_data::ref_storage st(shared_data::instance().storage);
	st_iter iter = st->lower_bound(k);
	if(iter == st->end()) { // most left data or biggest data
		iter = st->upper_bound(k);
		if(iter == st->end()) { // biggest data
			if(st->size() == 0){ return NULL;}
			else{return &*boost::prior(st->end());}
		}else{return &*iter;}
	}else if(iter->first == k){
		return &*iter;
	}

	const std::pair<st_iter, st_iter> lr_pair
		= it_and_next(iter);
	if(lr_pair.second == st->end()){// next key exists
		return &*lr_pair.first;
	}
	
	if(left_is_near(k, lr_pair.first->first, lr_pair.second->first)){
		return &*lr_pair.first;
	}else{
		return &*lr_pair.second;
	}
}

bool is_edge_key(const key& k){
	shared_data::ref_storage st(shared_data::instance().storage);
	if(st->begin()->first >= k)return true;
	if(boost::prior(st->end())->first <= k) return true;
	return false;
}


boost::optional<std::pair<key,host> >
nearest_node_info(const key& from_key, const sg_node& from_node,
	const direction& dir, shared_data::ref_storage& st){
	/*
	 * B is other node's key
	 * A  [B]  C  D
	 * D's left may be B because inner connection is omitted
	 * so select [B] or C which is nearer
	 */
	shared_data::storage_t::iterator relay = 
		st->lower_bound(from_key);
	if(relay == st->end()){
		relay = st->upper_bound(from_key);
		if(relay == st->end()){
			assert(!"there must be some key saved!");
		}
	}
	const shared_data::storage_t::iterator next = (dir == left)
		? boost::prior(relay) :boost::next(relay);

	if(from_node.neighbors()[dir][0]
		&&
		// inner node's key doesnt exist
		((next == st->end() || next->first == relay->first)
			|| // or nodes neighbor is nearer than inner node
			detail::left_is_near(relay->first
				,from_node.neighbors()[dir][0]->get_key(),next->first))){
		// the new node is nearer than old connection
		return boost::make_optional(
			std::make_pair(from_node.neighbors()[dir][0]->get_key()
				,from_node.neighbors()[dir][0]->get_host()));
	}else if(next != st->end() && next->first != relay->first){
		// inner node may be near
		return boost::make_optional(
			(std::make_pair(next->first, shared_data::instance().get_host())));
	}else{
		return NULL;
	}
}

}// namespace detail
}// namespace logic
