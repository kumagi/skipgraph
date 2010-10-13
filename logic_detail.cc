#include "logic_detail.hpp"
#include <map>

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

const std::pair<const key,sg_node>*
get_nearest_node(const key& k){
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
	
	if(left_is_near(k, lr_pair.first->first, lr_pair.second->first)){
		return &*lr_pair.first;
	}else{
		return &*lr_pair.second;
	}
}


}// namespace detail
}// namespace logic
