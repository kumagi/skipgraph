#ifndef SG_LOGIC_HPP
#define SG_LOGIC_HPP

#include <boost/function.hpp>
#include <map>
#include <string>
#include "skipgraph.h"
#include "debug_macro.h"

typedef boost::function<void (const msgpack::object&)> reaction;
typedef std::map<std::string, reaction> table;

class reflection : public singleton<reflection>{
public:
	typedef std::map<std::string, reaction> name_to_func;
private:
	name_to_func table;
public:
	void regist(const std::string& name, reaction func);
	bool call(const std::string& name, const msgpack::object& msg)const;
	
	//const name_to_func* get_table_ptr(void)const{return &table;};
};


RPC_OPERATION1(die, std::string, name);
RPC_OPERATION2(set, key, set_key, value, set_value);
RPC_OPERATION3(search, key, target_key, int, level, host, origin);
RPC_OPERATION2(found, key, found_key, value, found_value);
RPC_OPERATION1(notfound, key, target_key);
RPC_OPERATION2(range_search, range, target_range, host, origin);
RPC_OPERATION3(range_found, range, target_range, std::vector<key>,keys, std::vector<value>,values);
RPC_OPERATION2(range_notfound, range, target_range, std::vector<key>, keys);
RPC_OPERATION4(link, key, target_key, int, level, key, org_key, host, origin);
RPC_OPERATION4(treat, key, org_key, key, target_key, host,
							 origin, membership_vector, org_vector);
RPC_OPERATION5(introduce, key, org_key, key, target_key, host, origin,
							 membership_vector, org_vector, int, level);


#endif
