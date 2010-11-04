#include "logic_detail.hpp"
#include <map>
#include <boost/utility.hpp>

namespace logic{namespace detail{

void zip_two_node(const key& left_key, sg_node* left_node
									, const key& right_key, sg_node* right_node){
	shared_neighbor left_nbr = shared_data::instance().get_neighbor(left_key);
	shared_neighbor right_nbr = shared_data::instance().get_neighbor(right_key);
	for(int i=0;i<shared_data::instance().maxlevel;i++){
		left_node->neighbors()[right][i] = right_nbr;
		right_node->neighbors()[left][i] = left_nbr;
	}
}


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

boost::optional<std::pair<key,host> >
nearest_node_info(const key& from_key, const sg_node& from_node,
	const direction& dir, shared_data::ref_storage& st){

}

}// namespace detail
}// namespace logic
