//
// Copyright (C) 2011-15 DyND Developers
// BSD 2-Clause License, see LICENSE.txt
//

#include <dynd/types/typevar_dim_type.hpp>
#include <dynd/types/typevar_type.hpp>
#include <dynd/func/make_callable.hpp>

using namespace std;
using namespace dynd;

ndt::typevar_dim_type::typevar_dim_type(const nd::string &name,
                                        const type &element_type)
    : base_dim_type(typevar_dim_type_id, pattern_kind, element_type, 0, 1, 0,
                    type_flag_symbolic, false),
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

void
ndt::typevar_dim_type::get_vars(std::unordered_set<std::string> &vars) const
{
  vars.insert(m_name.str());
  m_element_tp.get_vars(vars);
}

void ndt::typevar_dim_type::print_data(std::ostream &DYND_UNUSED(o),
                                       const char *DYND_UNUSED(arrmeta),
                                       const char *DYND_UNUSED(data)) const
{
  throw type_error("Cannot store data of typevar type");
}

void ndt::typevar_dim_type::print_type(std::ostream &o) const
{
  // Type variables are barewords starting with a capital letter
  o << m_name.str() << " * " << get_element_type();
}

intptr_t
ndt::typevar_dim_type::get_dim_size(const char *DYND_UNUSED(arrmeta),
                                    const char *DYND_UNUSED(data)) const
{
  return -1;
}

bool ndt::typevar_dim_type::is_lossless_assignment(const type &dst_tp,
                                                   const type &src_tp) const
{
  if (dst_tp.extended() == this) {
    if (src_tp.extended() == this) {
      return true;
    } else if (src_tp.get_type_id() == typevar_dim_type_id) {
      return *dst_tp.extended() == *src_tp.extended();
    }
  }

  return false;
}

bool ndt::typevar_dim_type::operator==(const base_type &rhs) const
{
  if (this == &rhs) {
    return true;
  } else if (rhs.get_type_id() != typevar_dim_type_id) {
    return false;
  } else {
    const typevar_dim_type *tvt = static_cast<const typevar_dim_type *>(&rhs);
    return m_name == tvt->m_name && m_element_tp == tvt->m_element_tp;
  }
}

ndt::type ndt::typevar_dim_type::get_type_at_dimension(
    char **DYND_UNUSED(inout_arrmeta), intptr_t i, intptr_t total_ndim) const
{
  if (i == 0) {
    return type(this, true);
  } else {
    return m_element_tp.get_type_at_dimension(NULL, i - 1, total_ndim + 1);
  }
}

void ndt::typevar_dim_type::arrmeta_default_construct(
    char *DYND_UNUSED(arrmeta), bool DYND_UNUSED(blockref_alloc)) const
{
  throw type_error("Cannot store data of typevar type");
}

void ndt::typevar_dim_type::arrmeta_copy_construct(
    char *DYND_UNUSED(dst_arrmeta), const char *DYND_UNUSED(src_arrmeta),
    memory_block_data *DYND_UNUSED(embedded_reference)) const
{
  throw type_error("Cannot store data of typevar type");
}

size_t ndt::typevar_dim_type::arrmeta_copy_construct_onedim(
    char *DYND_UNUSED(dst_arrmeta), const char *DYND_UNUSED(src_arrmeta),
    memory_block_data *DYND_UNUSED(embedded_reference)) const
{
  throw type_error("Cannot store data of typevar type");
}

void ndt::typevar_dim_type::arrmeta_destruct(char *DYND_UNUSED(arrmeta)) const
{
  throw type_error("Cannot store data of typevar type");
}

bool ndt::typevar_dim_type::match(const char *arrmeta, const type &candidate_tp,
                                  const char *DYND_UNUSED(candidate_arrmeta),
                                  std::map<nd::string, type> &tp_vars) const
{
  if (!candidate_tp.is_dim()) {
    return false;
  }

  type &tv_type = tp_vars[get_name()];
  if (tv_type.is_null()) {
    // This typevar hasn't been seen yet
    tv_type = candidate_tp;
    return m_element_tp.match(
        arrmeta, candidate_tp.get_type_at_dimension(NULL, 1), NULL, tp_vars);
  } else {
    // Make sure the type matches previous
    // instances of the type var
    if (candidate_tp.get_type_id() != tv_type.get_type_id()) {
      return false;
    }
    switch (candidate_tp.get_type_id()) {
    case fixed_dim_type_id:
      if (candidate_tp.extended<fixed_dim_type>()->get_fixed_dim_size() !=
          tv_type.extended<fixed_dim_type>()->get_fixed_dim_size()) {
        return false;
      }
      break;
    default:
      break;
    }
    return m_element_tp.match(
        arrmeta, candidate_tp.get_type_at_dimension(NULL, 1), NULL, tp_vars);
  }
}

static nd::array property_get_name(const ndt::type &tp)
{
  return tp.extended<ndt::typevar_dim_type>()->get_name();
}

static ndt::type property_get_element_type(const ndt::type &dt)
{
  return dt.extended<ndt::typevar_dim_type>()->get_element_type();
}

void ndt::typevar_dim_type::get_dynamic_type_properties(
    const std::pair<std::string, gfunc::callable> **out_properties,
    size_t *out_count) const
{
  static pair<string, gfunc::callable> type_properties[] = {
      pair<string, gfunc::callable>(
          "name", gfunc::make_callable(&property_get_name, "self")),
      pair<string, gfunc::callable>(
          "element_type",
          gfunc::make_callable(&property_get_element_type, "self")),
  };

  *out_properties = type_properties;
  *out_count = sizeof(type_properties) / sizeof(type_properties[0]);
}