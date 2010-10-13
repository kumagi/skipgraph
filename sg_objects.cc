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
