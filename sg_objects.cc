#include "sg_objects.hpp"

/*
 * if not found, it append neighbor information to neighbor map
 */
boost::shared_ptr<neighbor> shared_data::get_neighbor(const key& k, const host& h){
	using boost::shared_ptr;
	using boost::weak_ptr;
	ref_ng_map ng(ngmap); // get lock
	shared_data::ng_map_t::iterator it = ng->find(k);
	shared_ptr<neighbor> ans;
	if(it == ng->end()){
		ans = shared_ptr<neighbor>
			(new neighbor(k, h));
		ng->insert(std::make_pair(k,boost::weak_ptr<neighbor>(ans)));
		return shared_ptr<neighbor>(ans);
	}else if(!(ans = it->second.lock())){
		ng->erase(it);
		ans = shared_ptr<neighbor>
			(new neighbor(k, h));
		ng->insert(std::make_pair(k,weak_ptr<neighbor>(ans)));
		return shared_ptr<neighbor>(ans);
	}else{
		return shared_ptr<neighbor>(it->second);
	}
}
/*
 * if not found, it returns nearest neighbors node
 */
boost::shared_ptr<const neighbor> shared_data::get_nearest_neighbor(const key& k){
	using boost::shared_ptr;
	using boost::weak_ptr;
	ref_ng_map ng(ngmap); // get lock
	shared_data::ng_map_t::iterator it = ng->find(k);

	shared_ptr<neighbor> ans;
	while(1){
		if(it == ng->end()){ // search nearest
			ans = ng->begin()->second.lock();
			if(!ans){ ng->erase(it);}
			else {break;}
		}else if(!(ans = it->second.lock())){ // erase and next
			ng->erase(it);
		}else{
			break;
		}
	}
	return ans;
}



direction get_direction(const key& lhs, const key& rhs){
	assert(lhs != rhs);
	return !(lhs >= rhs) ? right : left;
}

direction inverse(const direction& d){
	return static_cast<direction>(1 - static_cast<int>(d));
}

std::ostream&  operator<<(std::ostream& ost, const host& h){
	ost << "(" << h.hostname << ":" << h.port << ")";
	return ost;
}

std::ostream& operator<<(std::ostream& ost, const neighbor& nbr){
	ost << nbr.get_key() << "#" << nbr.get_host();
	return ost;
}

std::ostream& operator<<(std::ostream& ost, const sg_node& node)
{
	ost << node.get_value() << "[";
	for(int i=0;i<2;i++){
		int max = node.neighbors()[i].size();
		for(int j=0;j<max;++j)
			if(node.neighbors()[i][j])
				ost << "lv{" << j << "}" <<*node.neighbors()[i][j] << " ";
		ost<< "|";
	}
	return ost;
}
static std::ostream& operator<<(std::ostream& ost, const std::pair<key, sg_node>& kvp){
	ost << " key:" << kvp.first << " value:" << kvp.second;
	return ost;
}
/*
std::ostream& operator<<(std::ostream& ost, const suspended_node& sn){
	ost << " value:" << sn.value_
			<< "[" <<  sn.con[left] << "|" << sn.con[right]  << "] ";
	return ost;
}
*/

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
