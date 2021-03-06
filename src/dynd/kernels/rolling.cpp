//
// Copyright (C) 2011-15 DyND Developers
// BSD 2-Clause License, see LICENSE.txt
//

#include <dynd/kernels/ckernel_common_functions.hpp>
#include <dynd/kernels/rolling.hpp>
#include <dynd/types/var_dim_type.hpp>

using namespace std;
using namespace dynd;

void nd::functional::strided_rolling_ck::single(char *dst, char *const *src)
{
  ckernel_prefix *nachild = get_child_ckernel();
  ckernel_prefix *wopchild = get_child_ckernel(m_window_op_offset);
  expr_strided_t nachild_fn = nachild->get_function<expr_strided_t>();
  expr_strided_t wopchild_fn = wopchild->get_function<expr_strided_t>();
  // Fill in NA/NaN at the beginning
  if (m_dim_size > 0) {
    nachild_fn(dst, m_dst_stride, NULL, NULL,
               std::min(m_window_size - 1, m_dim_size), nachild);
  }
  // Use stride trickery to do this as one strided call
  if (m_dim_size >= m_window_size) {
    wopchild_fn(dst + m_dst_stride * (m_window_size - 1), m_dst_stride, src,
                &m_src_stride, m_dim_size - m_window_size + 1, wopchild);
  }
}

void nd::functional::strided_rolling_ck::destruct_children()
{
  // The NA filler
  get_child_ckernel()->destroy();
  // The window op
  destroy_child_ckernel(m_window_op_offset);
}

void nd::functional::var_rolling_ck::single(char *dst, char *const *src)
{
  // Get the child ckernels
  ckernel_prefix *nachild = get_child_ckernel();
  ckernel_prefix *wopchild = get_child_ckernel(m_window_op_offset);
  expr_strided_t nachild_fn = nachild->get_function<expr_strided_t>();
  expr_strided_t wopchild_fn = wopchild->get_function<expr_strided_t>();
  // Get pointers to the src and dst data
  var_dim_type_data *dst_dat = reinterpret_cast<var_dim_type_data *>(dst);
  intptr_t dst_stride =
      reinterpret_cast<const var_dim_type_arrmeta *>(m_dst_meta)->stride;
  var_dim_type_data *src_dat = reinterpret_cast<var_dim_type_data *>(src[0]);
  char *src_arr_ptr = src_dat->begin + m_src_offset;
  intptr_t dim_size = src_dat->size;
  // Allocate the output data
  ndt::var_dim_element_initialize(m_dst_tp, m_dst_meta, dst, dim_size);
  char *dst_arr_ptr = dst_dat->begin;

  // Fill in NA/NaN at the beginning
  if (dim_size > 0) {
    nachild_fn(dst_arr_ptr, dst_stride, NULL, NULL,
               std::min(m_window_size - 1, dim_size), nachild);
  }
  // Use stride trickery to do this as one strided call
  if (dim_size >= m_window_size) {
    wopchild_fn(dst_arr_ptr + dst_stride * (m_window_size - 1), dst_stride,
                &src_arr_ptr, &m_src_stride, dim_size - m_window_size + 1,
                wopchild);
  }
}

void nd::functional::var_rolling_ck::destruct_children()
{
  // The window op
  destroy_child_ckernel(m_window_op_offset);
  // The NA filler
  destroy_child_ckernel(sizeof(self_type));
}

// TODO This should handle both strided and var cases
intptr_t nd::functional::rolling_ck::instantiate(
    const arrfunc_type_data *af_self, const ndt::arrfunc_type *DYND_UNUSED(af_tp),
    char *DYND_UNUSED(data), void *ckb, intptr_t ckb_offset,
    const ndt::type &dst_tp, const char *dst_arrmeta, intptr_t nsrc,
    const ndt::type *src_tp, const char *const *src_arrmeta,
    kernel_request_t kernreq, const eval::eval_context *ectx,
    const nd::array &kwds, const std::map<nd::string, ndt::type> &tp_vars)
{
  typedef dynd::nd::functional::strided_rolling_ck self_type;
  rolling_arrfunc_data *data = *af_self->get_data_as<rolling_arrfunc_data *>();

  intptr_t root_ckb_offset = ckb_offset;
  self_type *self = self_type::make(ckb, kernreq, ckb_offset);
  const arrfunc_type_data *window_af = data->window_op.get();
  const ndt::arrfunc_type *window_af_tp = data->window_op.get_type();
  ndt::type dst_el_tp, src_el_tp;
  const char *dst_el_arrmeta, *src_el_arrmeta;
  if (!dst_tp.get_as_strided(dst_arrmeta, &self->m_dim_size,
                             &self->m_dst_stride, &dst_el_tp,
                             &dst_el_arrmeta)) {
    stringstream ss;
    ss << "rolling window ckernel: could not process type " << dst_tp;
    ss << " as a strided dimension";
    throw type_error(ss.str());
  }
  intptr_t src_dim_size;
  if (!src_tp[0].get_as_strided(src_arrmeta[0], &src_dim_size,
                                &self->m_src_stride, &src_el_tp,
                                &src_el_arrmeta)) {
    stringstream ss;
    ss << "rolling window ckernel: could not process type " << src_tp[0];
    ss << " as a strided dimension";
    throw type_error(ss.str());
  }
  if (src_dim_size != self->m_dim_size) {
    stringstream ss;
    ss << "rolling window ckernel: source dimension size " << src_dim_size
       << " for type " << src_tp[0] << " does not match dest dimension size "
       << self->m_dim_size << " for type " << dst_tp;
    throw type_error(ss.str());
  }
  self->m_window_size = data->window_size;
  // Create the NA-filling child ckernel
  ckb_offset = kernels::make_constant_value_assignment_ckernel(
      ckb, ckb_offset, dst_el_tp, dst_el_arrmeta,
      numeric_limits<double>::quiet_NaN(), kernel_request_strided, ectx);
  // Re-retrieve the self pointer, because it may be at a new memory location
  // now
  self = reinterpret_cast<ckernel_builder<kernel_request_host> *>(ckb)
             ->get_at<self_type>(root_ckb_offset);
  // Create the window op child ckernel
  self->m_window_op_offset = ckb_offset - root_ckb_offset;
  // We construct array arrmeta for the window op ckernel to use,
  // without actually creating an nd::array to hold it.
  arrmeta_holder(ndt::make_fixed_dim(data->window_size, src_el_tp))
      .swap(self->m_src_winop_meta);
  self->m_src_winop_meta.get_at<fixed_dim_type_arrmeta>(0)->dim_size =
      self->m_window_size;
  self->m_src_winop_meta.get_at<fixed_dim_type_arrmeta>(0)->stride =
      self->m_src_stride;
  if (src_el_tp.get_arrmeta_size() > 0) {
    src_el_tp.extended()->arrmeta_copy_construct(
        self->m_src_winop_meta.get() + sizeof(fixed_dim_type_arrmeta),
        src_el_arrmeta, NULL);
  }

  const char *src_winop_meta = self->m_src_winop_meta.get();
  return window_af->instantiate(
      window_af, window_af_tp, NULL, ckb, ckb_offset, dst_el_tp, dst_el_arrmeta,
      nsrc, &self->m_src_winop_meta.get_type(), &src_winop_meta,
      kernel_request_strided, ectx, kwds, tp_vars);
}

void nd::functional::rolling_ck::resolve_dst_type(
    const arrfunc_type_data *af_self, const ndt::arrfunc_type *af_tp,
    char *DYND_UNUSED(data), ndt::type &dst_tp, intptr_t nsrc,
    const ndt::type *src_tp, const nd::array &kwds,
    const std::map<nd::string, ndt::type> &tp_vars)

{
  if (nsrc != 1) {
    stringstream ss;
    ss << "Wrong number of arguments to rolling arrfunc with prototype ";
    ss << af_tp << ", got " << nsrc << " arguments";
    throw invalid_argument(ss.str());
  }
  nd::functional::rolling_arrfunc_data *data =
      *af_self->get_data_as<nd::functional::rolling_arrfunc_data *>();
  const arrfunc_type_data *child_af = data->window_op.get();
  // First get the type for the child arrfunc
  ndt::type child_dst_tp;
  if (child_af->resolve_dst_type) {
    ndt::type child_src_tp = ndt::make_fixed_dim(
        data->window_size, src_tp[0].get_type_at_dimension(NULL, 1));
    child_af->resolve_dst_type(child_af, af_tp, NULL, child_dst_tp, 1,
                               &child_src_tp, kwds, tp_vars);
  } else {
    child_dst_tp = data->window_op.get_type()->get_return_type();
  }

  if (src_tp[0].get_type_id() == var_dim_type_id) {
    dst_tp = ndt::make_var_dim(child_dst_tp);
  } else {
    dst_tp =
        ndt::make_fixed_dim(src_tp[0].get_dim_size(NULL, NULL), child_dst_tp);
  }
}