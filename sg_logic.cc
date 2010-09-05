#include "sg_logic.hpp"

void reflection::regist(const std::string& name, reaction func){
	assert(table.find(name) == table.end());
	typedef std::pair<name_to_func::iterator, bool> its;
	its it = table.insert(std::make_pair(name,func));
	assert(it.second != false);
}
bool reflection::call(const std::string& name, const msgpack::object& msg)const{
	name_to_func::const_iterator it = table.find(name);
	if(it == table.end()) return false;
	it->second(msg);
	return true;
}

//const name_to_func* get_table_ptr(void)const{return &table;};

namespace msg{
void die(const msgpack::object& obj){
	REACH("die:");
	fprintf(stderr,"die called ok");
	exit(0);
}
void notfound(const msgpack::object& obj){
	REACH("notfound:")
}

}
