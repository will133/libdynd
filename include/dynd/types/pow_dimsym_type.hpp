//
// Copyright (C) 2011-15 DyND Developers
// BSD 2-Clause License, see LICENSE.txt
//

#pragma once

#include <vector>
#include <string>

#include <dynd/array.hpp>
#include <dynd/string.hpp>
#include <dynd/types/base_dim_type.hpp>

namespace dynd {
namespace ndt {

  class pow_dimsym_type : public base_dim_type {
    type m_base_tp;
    nd::string m_exponent;

  public:
    pow_dimsym_type(const type &base_tp, const nd::string &exponent,
                    const type &element_type);

    virtual ~pow_dimsym_type() {}

    const type &get_base_type() const { return m_base_tp; }

    const nd::string &get_exponent() const { return m_exponent; }

    std::string get_exponent_str() const { return m_exponent.str(); }

    void print_data(std::ostream &o, const char *arrmeta,
                    const char *data) const;

    void print_type(std::ostream &o) const;

    type apply_linear_index(intptr_t nindices, const irange *indices,
                            size_t current_i, const type &root_tp,
                            bool leading_dimension) const;
    intptr_t apply_linear_index(intptr_t nindices, const irange *indices,
                                const char *arrmeta, const type &result_tp,
                                char *out_arrmeta,
                                memory_block_data *embedded_reference,
                                size_t current_i, const type &root_tp,
                                bool leading_dimension, char **inout_data,
                                memory_block_data **inout_dataref) const;

    intptr_t get_dim_size(const char *arrmeta, const char *data) const;

    bool is_lossless_assignment(const type &dst_tp, const type &src_tp) const;

    bool operator==(const base_type &rhs) const;

    void arrmeta_default_construct(char *arrmeta, bool blockref_alloc) const;
    void arrmeta_copy_construct(char *dst_arrmeta, const char *src_arrmeta,
                                memory_block_data *embedded_reference) const;
    size_t
    arrmeta_copy_construct_onedim(char *dst_arrmeta, const char *src_arrmeta,
                                  memory_block_data *embedded_reference) const;
    void arrmeta_destruct(char *arrmeta) const;

    bool match(const char *arrmeta, const type &candidate_tp,
               const char *candidate_arrmeta,
               std::map<nd::string, type> &tp_vars) const;

    //    void get_dynamic_type_properties(
    //      const std::pair<std::string, gfunc::callable> **out_properties,
    //    size_t *out_count) const;
  }; // class pow_dimsym_type

  /** Makes a dimensional power type with the specified base and exponent */
  inline type make_pow_dimsym(const type &base_tp, const nd::string &exponent,
                              const type &element_type)
  {
    return type(new pow_dimsym_type(base_tp, exponent, element_type), false);
  }

} // namespace dynd::ndt
} // namespace dynd