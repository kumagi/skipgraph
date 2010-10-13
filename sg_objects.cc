#include "sg_objects.hpp"

boost::shared_ptr<neighbor> shared_data::get_neighbor(const key& k, const host& h){
	ref_ng_map ng(ngmap); // get lock
	shared_data::ng_map_t::iterator it = ng->find(k);
	boost::shared_ptr<neighbor> ans;
	if(it == ng->end()){
		ans = boost::shared_ptr<neighbor>
			(new neighbor(k, h));
		ng->insert(std::make_pair(k,boost::weak_ptr<neighbor>(ans)));
		return boost::shared_ptr<neighbor>(ans);
	}else if(!(ans = it->second.lock())){
		ng->erase(it);
		ans = boost::shared_ptr<neighbor>
			(new neighbor(k, h));
		ng->insert(std::make_pair(k,boost::weak_ptr<neighbor>(ans)));
		return boost::shared_ptr<neighbor>(ans);
	}else{
		return boost::shared_ptr<neighbor>(it->second);
	}
}

std::ostream& operator<<(std::ostream& ost, const sg_node& node)
{
	return ost << node.get_value();
}
static std::ostream& operator<<(std::ostream& ost, const std::pair<key, sg_node>& kvp){
	ost << " key:" << kvp.first << " value:" << kvp.second;
	return ost;
}

void shared_data::storage_dump()const{
	ref_storage st(shared_data::instance().storage);
	storage_t::iterator it = st->begin();
	std::cout << "dump[";
	while(it != st->end()){
		std::cout << *it;
		++it;
	}
	std::cout << "]";
}
