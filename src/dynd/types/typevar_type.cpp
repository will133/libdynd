//
// Copyright (C) 2011-15 DyND Developers
// BSD 2-Clause License, see LICENSE.txt
//

#include <dynd/types/typevar_type.hpp>
#include <dynd/func/make_callable.hpp>

using namespace std;
using namespace dynd;

ndt::typevar_type::typevar_type(const nd::string &name)
    : base_type(typevar_type_id, pattern_kind, 0, 1, type_flag_symbolic, 0, 0,
                0),
      m_name(name)
{
  if (m_name.is_null()) {
    throw type_error("dynd typevar name cannot be null");
  } else if (!is_valid_typevar_name(m_name.begin(), m_name.end())) {
    stringstream ss;
    ss << "dynd typevar name ";
    print_escaped_utf8_string(ss, m_name.begin(), m_name.end());
    ss << " is not valid, it must be alphanumeric and begin with a capital";
    throw type_error(ss.str());
  }
}

void ndt::typevar_type::get_vars(std::unordered_set<std::string> &vars) const
{
  vars.insert(m_name.str());
}

void ndt::typevar_type::print_data(std::ostream &DYND_UNUSED(o),
                                   const char *DYND_UNUSED(arrmeta),
                                   const char *DYND_UNUSED(data)) const
{
  throw type_error("Cannot store data of typevar type");
}

void ndt::typevar_type::print_type(std::ostream &o) const
{
  // Type variables are barewords starting with a capital letter
  o << m_name.str();
}

ndt::type ndt::typevar_type::apply_linear_index(
    intptr_t DYND_UNUSED(nindices), const irange *DYND_UNUSED(indices),
    size_t DYND_UNUSED(current_i), const type &DYND_UNUSED(root_tp),
    bool DYND_UNUSED(leading_dimension)) const
{
  throw type_error("Cannot store data of typevar type");
}

intptr_t ndt::typevar_type::apply_linear_index(
    intptr_t DYND_UNUSED(nindices), const irange *DYND_UNUSED(indices),
    const char *DYND_UNUSED(arrmeta), const type &DYND_UNUSED(result_tp),
    char *DYND_UNUSED(out_arrmeta),
    memory_block_data *DYND_UNUSED(embedded_reference),
    size_t DYND_UNUSED(current_i), const type &DYND_UNUSED(root_tp),
    bool DYND_UNUSED(leading_dimension), char **DYND_UNUSED(inout_data),
    memory_block_data **DYND_UNUSED(inout_dataref)) const
{
  throw type_error("Cannot store data of typevar type");
}

bool ndt::typevar_type::is_lossless_assignment(const type &dst_tp,
                                               const type &src_tp) const
{
  if (dst_tp.extended() == this) {
    if (src_tp.extended() == this) {
      return true;
    } else if (src_tp.get_type_id() == typevar_type_id) {
      return *dst_tp.extended() == *src_tp.extended();
    }
  }

  return false;
}

bool ndt::typevar_type::operator==(const base_type &rhs) const
{
  if (this == &rhs) {
    return true;
  } else if (rhs.get_type_id() != typevar_type_id) {
    return false;
  } else {
    const typevar_type *tvt = static_cast<const typevar_type *>(&rhs);
    return m_name == tvt->m_name;
  }
}

void ndt::typevar_type::arrmeta_default_construct(
    char *DYND_UNUSED(arrmeta), bool DYND_UNUSED(blockref_alloc)) const
{
  throw type_error("Cannot store data of typevar type");
}

void ndt::typevar_type::arrmeta_copy_construct(
    char *DYND_UNUSED(dst_arrmeta), const char *DYND_UNUSED(src_arrmeta),
    memory_block_data *DYND_UNUSED(embedded_reference)) const
{
  throw type_error("Cannot store data of typevar type");
}

void ndt::typevar_type::arrmeta_destruct(char *DYND_UNUSED(arrmeta)) const
{
  throw type_error("Cannot store data of typevar type");
}

bool ndt::typevar_type::match(const char *DYND_UNUSED(arrmeta),
                              const type &candidate_tp,
                              const char *DYND_UNUSED(candidate_arrmeta),
                              std::map<nd::string, type> &tp_vars) const
{
  if (candidate_tp.get_type_id() == typevar_type_id) {
    return *this == *candidate_tp.extended();
  }

  if (candidate_tp.get_ndim() > 0 ||
      candidate_tp.get_type_id() == any_sym_type_id) {
    return false;
  }

  type &tv_type = tp_vars[m_name];
  if (tv_type.is_null()) {
    // This typevar hasn't been seen yet
    tv_type = candidate_tp;
    return true;
  } else {
    // Make sure the type matches previous
    // instances of the type var
    return candidate_tp == tv_type;
  }
}

static nd::array property_get_name(const ndt::type &tp)
{
  return tp.extended<ndt::typevar_type>()->get_name();
}

void ndt::typevar_type::get_dynamic_type_properties(
    const std::pair<std::string, gfunc::callable> **out_properties,
    size_t *out_count) const
{
  static pair<string, gfunc::callable> type_properties[] = {
      pair<string, gfunc::callable>(
          "name", gfunc::make_callable(&property_get_name, "self")),
  };

  *out_properties = type_properties;
  *out_count = sizeof(type_properties) / sizeof(type_properties[0]);
}

bool ndt::is_valid_typevar_name(const char *begin, const char *end)
{
  if (begin != end) {
    if (*begin < 'A' || *begin > 'Z') {
      return false;
    }
    ++begin;
    while (begin < end) {
      char c = *begin;
      if ((c < 'a' || c > 'z') && (c < 'A' || c > 'Z') &&
          (c < '0' || c > '9') && c != '_') {
        return false;
      }
      ++begin;
    }
    return true;
  } else {
    return false;
  }
}

nd::array ndt::make_typevar_range(const char *name, intptr_t count)
{
  nd::array result;
  string s(name);
  s += '0';
  if (count > 10) {
    throw runtime_error("TODO: extend make_typevar_range");
  }
  result = nd::empty(count, make_type());
  auto result_ptr = reinterpret_cast<type *>(result.get_readwrite_originptr());
  for (int i = 0; i < count; ++i) {
    result_ptr[i] = make_typevar(s);
    s[s.size() - 1]++;
  }
  return move(result);
}