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
		DEBUG_OUT("not found and create");
		ans = shared_ptr<neighbor>
			(new neighbor(k, h));
		ng->insert(std::make_pair(k,boost::weak_ptr<neighbor>(ans)));
		return shared_ptr<neighbor>(ans);
	}else if(!(ans = it->second.lock())){

		DEBUG_OUT("could not catch");
		ng->erase(it);
		ans = shared_ptr<neighbor>
			(new neighbor(k, h));
		ng->insert(std::make_pair(k,weak_ptr<neighbor>(ans)));
		return shared_ptr<neighbor>(ans);
	}else{
		DEBUG_OUT("found");
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
			if(ng->empty()) return ans;
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

std::ostream& operator<<(std::ostream& ost, const std::vector<std::string>& v){
	ost << "[";
	for(std::vector<key>::const_iterator it = v.begin(); it != v.end() ; ++it){
		ost << *it << ",";
	}
	ost << "]";
	return ost;
}


std::ostream&  operator<<(std::ostream& ost, const membership_vector& v){
	boost::io::ios_flags_saver ifs( ost );
	ost << std::hex << v.vector;
	return ost;
} 

std::ostream&  operator<<(std::ostream& ost, const range& r){
	ost << (r.border_begin_ ? "[" : "(") << r.begin_ << " ~ " << r.end_
			<< (r.border_end_ ? "]" : ")");
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
		if(i==0){ost<< "|";}
	}
	ost << "]";
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

std::ostream& operator<<(std::ostream& ost, shared_data& s){
	ost << "maxlevel:" << s.maxlevel << std::endl
			<< "host:" << s.myhost << std::endl
			<< "vector:" << s.myvector << std::endl;
	ost << "storages [" << std::endl;
	{
		shared_data::ref_storage st(s.storage);
		for(shared_data::storage_t::iterator it = st->begin();
				it != st->end();
				++it){
			ost << *it  << std::endl;
		}
		ost << "]" << std::endl;
	}
	ost << "neighbors ["  << std::endl;
	{
		shared_data::ref_ng_map st(s.ngmap);
		for(shared_data::ng_map_t::iterator it = st->begin();
				it != st->end();
				++it){
			boost::shared_ptr<const neighbor> ng 
				= it->second.lock();
			if(!ng) continue;
			ost << "key:" << it->first << "  host:" << ng->get_key() << " | " << ng->get_host()  << std::endl;
		}
		ost << "]" << std::endl;;
	}
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
