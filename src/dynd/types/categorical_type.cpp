//
// Copyright (C) 2011-15 DyND Developers
// BSD 2-Clause License, see LICENSE.txt
//

#include <cstring>
#include <map>
#include <set>

#include <dynd/auxiliary_data.hpp>
#include <dynd/types/categorical_type.hpp>
#include <dynd/kernels/assignment_kernels.hpp>
#include <dynd/kernels/comparison_kernels.hpp>
#include <dynd/types/fixed_dim_type.hpp>
#include <dynd/types/convert_type.hpp>
#include <dynd/func/make_callable.hpp>
#include <dynd/array_range.hpp>

using namespace dynd;
using namespace std;

namespace {

class sorter {
  const char *m_originptr;
  intptr_t m_stride;
  const expr_predicate_t m_less;
  ckernel_prefix *m_less_self;

public:
  sorter(const char *originptr, intptr_t stride, const expr_predicate_t less,
         ckernel_prefix *less_self)
      : m_originptr(originptr), m_stride(stride), m_less(less),
        m_less_self(less_self)
  {
  }
  bool operator()(intptr_t i, intptr_t j) const
  {
    const char *s[2] = {m_originptr + i * m_stride, m_originptr + j * m_stride};
    return m_less(s, m_less_self) != 0;
  }
};

class cmp {
  const expr_predicate_t m_less;
  ckernel_prefix *m_less_self;

public:
  cmp(const expr_predicate_t less, ckernel_prefix *less_self)
      : m_less(less), m_less_self(less_self)
  {
  }
  bool operator()(const char *a, const char *b) const
  {
    const char *s[2] = {a, b};
    bool result = m_less(s, m_less_self) != 0;
    return result;
  }
};

// Assign from a categorical type to some other type
template <typename UIntType>
struct categorical_to_other_kernel
    : nd::base_kernel<categorical_to_other_kernel<UIntType>,
                      kernel_request_host, 1> {
  typedef categorical_to_other_kernel extra_type;

  const ndt::categorical_type *src_cat_tp;

  void single(char *dst, char *const *src)
  {
    ckernel_prefix *echild = this->get_child_ckernel();
    expr_single_t opchild = echild->get_function<expr_single_t>();

    uint32_t value = *reinterpret_cast<const UIntType *>(src[0]);
    char *src_val =
        const_cast<char *>(src_cat_tp->get_category_data_from_value(value));
    opchild(dst, &src_val, echild);
  }

  static void destruct(ckernel_prefix *self)
  {
    extra_type *e = reinterpret_cast<extra_type *>(self);
    if (e->src_cat_tp != NULL) {
      base_type_decref(e->src_cat_tp);
    }
    self->destroy_child_ckernel(sizeof(extra_type));
  }
};

template <typename UIntType>
struct category_to_categorical_kernel_extra
    : nd::base_kernel<category_to_categorical_kernel_extra<UIntType>,
                      kernel_request_host, 1> {
  typedef category_to_categorical_kernel_extra self_type;

  const ndt::categorical_type *dst_cat_tp;
  const char *src_arrmeta;

  // Assign from an input matching the category type to a categorical type
  void single(char *dst, char *const *src)
  {
    uint32_t src_val = dst_cat_tp->get_value_from_category(src_arrmeta, src[0]);
    *reinterpret_cast<UIntType *>(dst) = src_val;
  }

  static void destruct(ckernel_prefix *self)
  {
    self_type *e = reinterpret_cast<self_type *>(self);
    if (e->dst_cat_tp != NULL) {
      base_type_decref(e->dst_cat_tp);
    }
  }
};

// struct assign_from_commensurate_category {
//     static void general_kernel(char *dst, intptr_t dst_stride, const char
//     *src, intptr_t src_stride,
//                         intptr_t count, const AuxDataBase *auxdata)
//     {
//         categorical_type *cat = reinterpret_cast<categorical_type *>(
//             get_raw_auxiliary_data(auxdata)&~1
//         );
//     }

//     static void scalar_kernel(char *dst, intptr_t DYND_UNUSED(dst_stride),
//     const char *src, intptr_t DYND_UNUSED(src_stride),
//                         intptr_t, const AuxDataBase *auxdata)
//     {
//         categorical_type *cat = reinterpret_cast<categorical_type *>(
//             get_raw_auxiliary_data(auxdata)&~1
//         );
//     }

//     static void contiguous_kernel(char *dst, intptr_t
//     DYND_UNUSED(dst_stride), const char *src, intptr_t
//     DYND_UNUSED(src_stride),
//                         intptr_t count, const AuxDataBase *auxdata)
//     {
//         categorical_type *cat = reinterpret_cast<categorical_type *>(
//             get_raw_auxiliary_data(auxdata)&~1
//         );
//     }

//     static void scalar_to_contiguous_kernel(char *dst, intptr_t
//     DYND_UNUSED(dst_stride), const char *src, intptr_t
//     DYND_UNUSED(src_stride),
//                         intptr_t count, const AuxDataBase *auxdata)
//     {
//         categorical_type *cat = reinterpret_cast<categorical_type *>(
//             get_raw_auxiliary_data(auxdata)&~1
//         );
//     }
// };

// static specialized_unary_operation_table_t
// assign_from_commensurate_category_specializations = {
//     assign_from_commensurate_category::general_kernel,
//     assign_from_commensurate_category::scalar_kernel,
//     assign_from_commensurate_category::contiguous_kernel,
//     assign_from_commensurate_category::scalar_to_contiguous_kernel
// };

} // anoymous namespace

/** This function converts the set of char* pointers into a strided immutable
 * nd::array of the categories */
static nd::array make_sorted_categories(const set<const char *, cmp> &uniques,
                                        const ndt::type &element_tp,
                                        const char *arrmeta)
{
  nd::array categories = nd::empty(uniques.size(), element_tp);
  ckernel_builder<kernel_request_host> k;
  make_assignment_kernel(
      NULL, NULL, &k, 0, element_tp,
      categories.get_arrmeta() + sizeof(fixed_dim_type_arrmeta), element_tp,
      arrmeta, kernel_request_single, &eval::default_eval_context, nd::array());
  expr_single_t fn = k.get()->get_function<expr_single_t>();

  intptr_t stride = reinterpret_cast<const fixed_dim_type_arrmeta *>(
                        categories.get_arrmeta())->stride;
  char *dst_ptr = categories.get_readwrite_originptr();
  for (set<const char *, cmp>::const_iterator it = uniques.begin();
       it != uniques.end(); ++it) {
    char *src = const_cast<char *>(*it);
    fn(dst_ptr, &src, k.get());
    dst_ptr += stride;
  }
  categories.get_type().extended()->arrmeta_finalize_buffers(
      categories.get_arrmeta());
  categories.flag_as_immutable();

  return categories;
}

ndt::categorical_type::categorical_type(const nd::array &categories, bool presorted)
    : base_type(categorical_type_id, custom_kind, 4, 4, type_flag_scalar, 0, 0,
                0)
{
  intptr_t category_count;
  if (presorted) {
    // This is construction shortcut, for the case when the categories are
    // already
    // sorted. No validation of this is done, the caller should have ensured it
    // was correct already, typically by construction.
    m_categories = categories.eval_immutable();
    m_category_tp = m_categories.get_type().at(0);

    category_count = categories.get_dim_size();
    m_value_to_category_index = nd::range(category_count);
    m_value_to_category_index.flag_as_immutable();
    m_category_index_to_value = m_value_to_category_index;
  } else {
    // Process the categories array to make sure it's valid
    const type &cdt = categories.get_type();
    if (cdt.get_type_id() != fixed_dim_type_id) {
      throw dynd::type_error("categorical_type only supports construction from "
                             "a fixed-dim array of categories");
    }
    m_category_tp = categories.get_type().at(0);
    if (!m_category_tp.is_scalar()) {
      throw dynd::type_error("categorical_type only supports construction from "
                             "a 1-dimensional strided array of categories");
    }

    category_count = categories.get_dim_size();
    intptr_t categories_stride =
        reinterpret_cast<const fixed_dim_type_arrmeta *>(
            categories.get_arrmeta())->stride;

    const char *categories_element_arrmeta =
        categories.get_arrmeta() + sizeof(fixed_dim_type_arrmeta);
    ckernel_builder<kernel_request_host> k;
    ::make_comparison_kernel(&k, 0, m_category_tp, categories_element_arrmeta,
                             m_category_tp, categories_element_arrmeta,
                             comparison_type_sorting_less,
                             &eval::default_eval_context);
    expr_predicate_t fn = k.get()->get_function<expr_predicate_t>();

    cmp less(fn, k.get());
    set<const char *, cmp> uniques(less);

    m_value_to_category_index =
        nd::empty(category_count, make_type<intptr_t>());
    m_category_index_to_value =
        nd::empty(category_count, make_type<intptr_t>());

    // create the mapping from indices of (to be lexicographically sorted)
    // categories to values
    for (size_t i = 0; i != (size_t)category_count; ++i) {
      unchecked_fixed_dim_get_rw<intptr_t>(m_category_index_to_value, i) = i;
      const char *category_value =
          categories.get_readonly_originptr() + i * categories_stride;

      if (uniques.find(category_value) == uniques.end()) {
        uniques.insert(category_value);
      } else {
        stringstream ss;
        ss << "categories must be unique: category value ";
        m_category_tp.print_data(ss, categories_element_arrmeta,
                                 category_value);
        ss << " appears more than once";
        throw std::runtime_error(ss.str());
      }
    }
    // TODO: Putting everything in a set already caused a sort operation to
    // occur,
    //       there's no reason we should need a second sort.
    std::sort(
        &unchecked_fixed_dim_get_rw<intptr_t>(m_category_index_to_value, 0),
        &unchecked_fixed_dim_get_rw<intptr_t>(m_category_index_to_value,
                                              category_count),
        sorter(categories.get_readonly_originptr(), categories_stride,
               fn, k.get()));

    // invert the m_category_index_to_value permutation
    for (intptr_t i = 0; i < category_count; ++i) {
      unchecked_fixed_dim_get_rw<intptr_t>(
          m_value_to_category_index,
          unchecked_fixed_dim_get<intptr_t>(m_category_index_to_value, i)) = i;
    }

    m_categories = make_sorted_categories(uniques, m_category_tp,
                                          categories_element_arrmeta);
  }

  // Use the number of categories to set which underlying integer storage to use
  if (category_count <= 256) {
    m_storage_type = make_type<uint8_t>();
  } else if (category_count <= 65536) {
    m_storage_type = make_type<uint16_t>();
  } else {
    m_storage_type = make_type<uint32_t>();
  }
  m_members.data_size = m_storage_type.get_data_size();
  m_members.data_alignment = (uint8_t)m_storage_type.get_data_alignment();
}

void ndt::categorical_type::print_data(std::ostream &o,
                                  const char *DYND_UNUSED(arrmeta),
                                  const char *data) const
{
  intptr_t category_count = m_categories.get_dim_size();
  uint32_t value;
  switch (m_storage_type.get_type_id()) {
  case uint8_type_id:
    value = *reinterpret_cast<const uint8_t *>(data);
    break;
  case uint16_type_id:
    value = *reinterpret_cast<const uint16_t *>(data);
    break;
  case uint32_type_id:
    value = *reinterpret_cast<const uint32_t *>(data);
    break;
  default:
    throw runtime_error("internal error in categorical_type::print_data");
  }
  if ((intptr_t)value < category_count) {
    m_category_tp.print_data(o, get_category_arrmeta(),
                             get_category_data_from_value(value));
  } else {
    o << "NA";
  }
}

void ndt::categorical_type::print_type(std::ostream &o) const
{
  size_t category_count = get_category_count();
  const char *arrmeta =
      m_categories.get_arrmeta() + sizeof(fixed_dim_type_arrmeta);

  o << "categorical[" << m_category_tp;
  o << ", [";
  m_category_tp.print_data(o, arrmeta, get_category_data_from_value(0));
  for (size_t i = 1; i != category_count; ++i) {
    o << ", ";
    m_category_tp.print_data(o, arrmeta,
                             get_category_data_from_value((uint32_t)i));
  }
  o << "]]";
}

void ndt::categorical_type::get_shape(intptr_t ndim, intptr_t i,
                                       intptr_t *out_shape,
                                       const char *DYND_UNUSED(arrmeta),
                                       const char *DYND_UNUSED(data)) const
{
  const type &cd = get_category_type();
  if (!cd.is_builtin()) {
    cd.extended()->get_shape(ndim, i, out_shape, get_category_arrmeta(), NULL);
  } else {
    stringstream ss;
    ss << "requested too many dimensions from type " << type(this, true);
    throw runtime_error(ss.str());
  }
}

uint32_t
ndt::categorical_type::get_value_from_category(const char *category_arrmeta,
                                          const char *category_data) const
{
  intptr_t i = nd::binary_search(m_categories, category_arrmeta, category_data);
  if (i < 0) {
    stringstream ss;
    ss << "Unrecognized category value ";
    m_category_tp.print_data(ss, category_arrmeta, category_data);
    ss << " assigning to dynd type " << type(this, true);
    throw std::runtime_error(ss.str());
  } else {
    return (uint32_t)unchecked_fixed_dim_get<intptr_t>(
        m_category_index_to_value, i);
  }
}

uint32_t
ndt::categorical_type::get_value_from_category(const nd::array &category) const
{
  if (category.get_type() == m_category_tp) {
    // If the type is right, get the category value directly
    return get_value_from_category(category.get_arrmeta(),
                                   category.get_readonly_originptr());
  } else {
    // Otherwise convert to the correct type, then get the category value
    nd::array c = nd::empty(m_category_tp);
    c.val_assign(category);
    return get_value_from_category(c.get_arrmeta(), c.get_readonly_originptr());
  }
}

const char *ndt::categorical_type::get_category_arrmeta() const
{
  const char *arrmeta = m_categories.get_arrmeta();
  m_categories.get_type().extended()->at_single(0, &arrmeta, NULL);
  return arrmeta;
}

nd::array ndt::categorical_type::get_categories() const
{
  // TODO: store categories in their original order
  //       so this is simply "return m_categories".
  nd::array categories = nd::empty(get_category_count(), m_category_tp);
  intptr_t dim_size, stride;
  type el_tp;
  const char *el_arrmeta;
  categories.get_type().get_as_strided(categories.get_arrmeta(), &dim_size,
                                       &stride, &el_tp, &el_arrmeta);
  ckernel_builder<kernel_request_host> k;
  ::make_assignment_kernel(NULL, NULL, &k, 0, m_category_tp, el_arrmeta, el_tp,
                           get_category_arrmeta(), kernel_request_single,
                           &eval::default_eval_context, nd::array());
  expr_single_t fn = k.get()->get_function<expr_single_t>();
  for (intptr_t i = 0; i < dim_size; ++i) {
    char *src = const_cast<char *>(get_category_data_from_value((uint32_t)i));
    fn(categories.get_readwrite_originptr() + i * stride, &src, k.get());
  }
  return categories;
}

bool ndt::categorical_type::is_lossless_assignment(const type &dst_tp,
                                              const type &src_tp) const
{
  if (dst_tp.extended() == this) {
    if (src_tp.extended() == this) {
      // Casting from identical types
      return true;
    } else {
      return false; // TODO
    }

  } else {
    return ::is_lossless_assignment(dst_tp, m_category_tp); // TODO
  }
}

intptr_t ndt::categorical_type::make_assignment_kernel(
    const arrfunc_type_data *self, const arrfunc_type *af_tp, void *ckb,
    intptr_t ckb_offset, const type &dst_tp, const char *dst_arrmeta,
    const type &src_tp, const char *src_arrmeta, kernel_request_t kernreq,
    const eval::eval_context *ectx, const nd::array &kwds) const
{
  if (this == dst_tp.extended()) {
    if (this == src_tp.extended()) {
      // When assigning identical types, just use a POD copy
      return make_pod_typed_data_assignment_kernel(
          ckb, ckb_offset, get_data_size(), get_data_alignment(), kernreq);
    }
    // try to assign from another categorical type if it can be mapped
    else if (src_tp.get_type_id() == categorical_type_id) {
      // out_kernel.specializations =
      // assign_from_commensurate_category_specializations;
      // TODO auxdata
      throw std::runtime_error(
          "assignment between different categorical types isn't supported yet");
    }
    // assign from the same category value type
    else if (src_tp == m_category_tp) {
      switch (m_storage_type.get_type_id()) {
      case uint8_type_id: {
        category_to_categorical_kernel_extra<uint8_t> *e =
            category_to_categorical_kernel_extra<uint8_t>::make(ckb, kernreq,
                                                                ckb_offset);
        e->dst_cat_tp =
            static_cast<const categorical_type *>(type(dst_tp).release());
        e->src_arrmeta = src_arrmeta;
      } break;
      case uint16_type_id: {
        category_to_categorical_kernel_extra<uint16_t> *e =
            category_to_categorical_kernel_extra<uint16_t>::make(ckb, kernreq,
                                                                 ckb_offset);
        e->dst_cat_tp =
            static_cast<const categorical_type *>(type(dst_tp).release());
        e->src_arrmeta = src_arrmeta;
      } break;
      case uint32_type_id: {
        category_to_categorical_kernel_extra<uint32_t> *e =
            category_to_categorical_kernel_extra<uint32_t>::make(ckb, kernreq,
                                                                 ckb_offset);
        e->dst_cat_tp =
            static_cast<const categorical_type *>(type(dst_tp).release());
        e->src_arrmeta = src_arrmeta;
      } break;
      default:
        throw runtime_error(
            "internal error in categorical_type::make_assignment_kernel");
      }
      // The kernel type owns a reference to this type
      return ckb_offset;
    } else if (src_tp.value_type() != m_category_tp &&
               src_tp.value_type().get_type_id() != categorical_type_id) {
      // Make a convert type to the category type, and have it do the chaining
      type src_cvt_tp = make_convert(m_category_tp, src_tp);
      return src_cvt_tp.extended()->make_assignment_kernel(
          self, af_tp, ckb, ckb_offset, dst_tp, dst_arrmeta, src_cvt_tp,
          src_arrmeta, kernreq, ectx, kwds);
    } else {
      // Let the src_tp handle it
      return src_tp.extended()->make_assignment_kernel(
          self, af_tp, ckb, ckb_offset, dst_tp, dst_arrmeta, src_tp,
          src_arrmeta, kernreq, ectx, kwds);
    }
  } else {
    if (dst_tp.value_type().get_type_id() != categorical_type_id) {
      switch (m_storage_type.get_type_id()) {
      case uint8_type_id: {
        categorical_to_other_kernel<uint8_t> *e =
            categorical_to_other_kernel<uint8_t>::make(ckb, kernreq,
                                                       ckb_offset);
        // The kernel type owns a reference to this type
        e->src_cat_tp =
            static_cast<const categorical_type *>(type(src_tp).release());

      } break;
      case uint16_type_id: {
        categorical_to_other_kernel<uint16_t> *e =
            categorical_to_other_kernel<uint16_t>::make(ckb, kernreq,
                                                        ckb_offset);
        // The kernel type owns a reference to this type
        e->src_cat_tp =
            static_cast<const categorical_type *>(type(src_tp).release());

      } break;
      case uint32_type_id: {
        categorical_to_other_kernel<uint32_t> *e =
            categorical_to_other_kernel<uint32_t>::make(ckb, kernreq,
                                                        ckb_offset);
        // The kernel type owns a reference to this type
        e->src_cat_tp =
            static_cast<const categorical_type *>(type(src_tp).release());
      } break;
      default:
        throw runtime_error(
            "internal error in categorical_type::make_assignment_kernel");
      }
      return ::make_assignment_kernel(self, af_tp, ckb, ckb_offset, dst_tp,
                                      dst_arrmeta, get_category_type(),
                                      get_category_arrmeta(),
                                      kernel_request_single, ectx, kwds);
    } else {
      stringstream ss;
      ss << "Cannot assign from " << src_tp << " to " << dst_tp;
      throw runtime_error(ss.str());
    }
  }
}

bool ndt::categorical_type::operator==(const base_type &rhs) const
{
  if (this == &rhs)
    return true;
  if (rhs.get_type_id() != categorical_type_id)
    return false;
  if (!m_categories.equals_exact(
          static_cast<const categorical_type &>(rhs).m_categories))
    return false;
  if (!m_category_index_to_value.equals_exact(
          static_cast<const categorical_type &>(rhs).m_category_index_to_value))
    return false;
  if (!m_value_to_category_index.equals_exact(
          static_cast<const categorical_type &>(rhs).m_value_to_category_index))
    return false;

  return true;
}

void ndt::categorical_type::arrmeta_default_construct(
    char *DYND_UNUSED(arrmeta), bool DYND_UNUSED(blockref_alloc)) const
{
  // Data is stored as uint##, no arrmeta to process
}

void ndt::categorical_type::arrmeta_copy_construct(
    char *DYND_UNUSED(dst_arrmeta), const char *DYND_UNUSED(src_arrmeta),
    memory_block_data *DYND_UNUSED(embedded_reference)) const
{
  // Data is stored as uint##, no arrmeta to process
}

void ndt::categorical_type::arrmeta_destruct(char *DYND_UNUSED(arrmeta)) const
{
  // Data is stored as uint##, no arrmeta to process
}

void ndt::categorical_type::arrmeta_debug_print(
    const char *DYND_UNUSED(arrmeta), std::ostream &DYND_UNUSED(o),
    const std::string &DYND_UNUSED(indent)) const
{
  // Data is stored as uint##, no arrmeta to process
}

ndt::type ndt::factor_categorical(const nd::array &values)
{
  // Do the factor operation on a concrete version of the values
  // TODO: Some cases where we don't want to do this?
  nd::array values_eval = values.eval();

  intptr_t dim_size, stride;
  type el_tp;
  const char *el_arrmeta;
  values_eval.get_type().get_as_strided(values_eval.get_arrmeta(), &dim_size,
                                        &stride, &el_tp, &el_arrmeta);

  ckernel_builder<kernel_request_host> k;
  ::make_comparison_kernel(&k, 0, el_tp, el_arrmeta, el_tp, el_arrmeta,
                           comparison_type_sorting_less,
                           &eval::default_eval_context);
  expr_predicate_t fn = k.get()->get_function<expr_predicate_t>();

  cmp less(fn, k.get());
  set<const char *, cmp> uniques(less);

  for (intptr_t i = 0; i < dim_size; ++i) {
    const char *data = values_eval.get_readonly_originptr() + i * stride;
    if (uniques.find(data) == uniques.end()) {
      uniques.insert(data);
    }
  }

  // Copy the values (now sorted and unique) into a new nd::array
  nd::array categories = make_sorted_categories(uniques, el_tp, el_arrmeta);

  return type(new categorical_type(categories, true), false);
}

static nd::array property_ndo_get_ints(const nd::array &n)
{
  ndt::type udt = n.get_dtype().value_type();
  const ndt::categorical_type *cd = udt.extended<ndt::categorical_type>();
  return n.view_scalars(cd->get_storage_type());
}

static size_t categorical_array_properties_size() { return 1; }

static const pair<string, gfunc::callable> *categorical_array_properties()
{
  static pair<string, gfunc::callable> categorical_array_properties[1] = {
      pair<string, gfunc::callable>(
          "ints", gfunc::make_callable(&property_ndo_get_ints, "self"))};

  return categorical_array_properties;
}

void ndt::categorical_type::get_dynamic_array_properties(
    const std::pair<std::string, gfunc::callable> **out_properties,
    size_t *out_count) const
{
  *out_properties = categorical_array_properties();
  *out_count = categorical_array_properties_size();
}

static nd::array property_type_get_categories(const ndt::type &d)
{
  const ndt::categorical_type *cd = d.extended<ndt::categorical_type>();
  return cd->get_categories();
}

static ndt::type property_type_get_storage_type(const ndt::type &d)
{
  const ndt::categorical_type *cd = d.extended<ndt::categorical_type>();
  return cd->get_storage_type();
}

static ndt::type property_type_get_category_type(const ndt::type &d)
{
  const ndt::categorical_type *cd = d.extended<ndt::categorical_type>();
  return cd->get_category_type();
}

void ndt::categorical_type::get_dynamic_type_properties(
    const std::pair<std::string, gfunc::callable> **out_properties,
    size_t *out_count) const
{
  static pair<string, gfunc::callable> categorical_type_properties[] = {
      pair<string, gfunc::callable>(
          "categories",
          gfunc::make_callable(&property_type_get_categories, "self")),
      pair<string, gfunc::callable>(
          "storage_type",
          gfunc::make_callable(&property_type_get_storage_type, "self")),
      pair<string, gfunc::callable>(
          "category_type",
          gfunc::make_callable(&property_type_get_category_type, "self"))};

  *out_properties = categorical_type_properties;
  *out_count = sizeof(categorical_type_properties) /
               sizeof(categorical_type_properties[0]);
}