#ifndef LOGIC_DETAIL_HPP
#define LOGIC_DETAIL_HPP
#include "sg_objects.hpp"

namespace logic{
namespace detail{

int string_distance(const std::string& lhs, const std::string& rhs);
bool left_is_near(const key& org, const key& lhs, const key& rhs);
const key& which_near(const key& org, const key& lhs, const key& rhs);

typedef shared_data::storage_t::iterator st_iter;
using boost::optional;

std::pair<st_iter, st_iter>
it_and_next(const st_iter& org);

std::pair<const key,sg_node>* get_nearest_node(const key& k);
} // namespace detail
} // namespace logic
#endif
