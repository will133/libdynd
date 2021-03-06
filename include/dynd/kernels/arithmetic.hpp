#pragma once

#include <dynd/kernels/base_kernel.hpp>
#include <dynd/kernels/multidispatch_by_type_id.hpp>

namespace dynd {
namespace nd {

  template <type_id_t I0>
  struct plus_kernel
      : base_kernel<plus_kernel<I0>, kernel_request_cuda_host_device, 1> {
    typedef typename type_of<I0>::type A0;
    typedef decltype(+std::declval<A0>()) R;

    DYND_CUDA_HOST_DEVICE void single(char *dst, char *const *src)
    {
      *reinterpret_cast<R *>(dst) = +*reinterpret_cast<A0 *>(src[0]);
    }

    static void resolve_dst_type(
        const arrfunc_type_data *DYND_UNUSED(self),
        const ndt::arrfunc_type *DYND_UNUSED(self_tp), char *DYND_UNUSED(data),
        ndt::type &dst_tp, intptr_t DYND_UNUSED(nsrc),
        const ndt::type *DYND_UNUSED(src_tp),
        const dynd::nd::array &DYND_UNUSED(kwds),
        const std::map<dynd::nd::string, ndt::type> &DYND_UNUSED(tp_vars))
    {
      dst_tp = ndt::make_type<R>();
    }

    static ndt::type make_type()
    {
      std::map<string, ndt::type> tp_vars;
      tp_vars["A0"] = ndt::make_type<A0>();
      tp_vars["R"] = ndt::make_type<R>();

      return ndt::substitute(ndt::type("(A0) -> R"), tp_vars, true);
    }
  };

  template <type_id_t I0>
  struct minus_kernel
      : base_kernel<minus_kernel<I0>, kernel_request_cuda_host_device, 1> {
    typedef typename type_of<I0>::type A0;
    typedef decltype(-std::declval<A0>()) R;

    DYND_CUDA_HOST_DEVICE void single(char *dst, char *const *src)
    {
      *reinterpret_cast<R *>(dst) = -*reinterpret_cast<A0 *>(src[0]);
    }

    static ndt::type make_type()
    {
      std::map<string, ndt::type> tp_vars;
      tp_vars["A0"] = ndt::make_type<A0>();
      tp_vars["R"] = ndt::make_type<R>();

      return ndt::substitute(ndt::type("(A0) -> R"), tp_vars, true);
    }

    static void resolve_dst_type(
        const arrfunc_type_data *DYND_UNUSED(self),
        const ndt::arrfunc_type *DYND_UNUSED(self_tp), char *DYND_UNUSED(data),
        ndt::type &dst_tp, intptr_t DYND_UNUSED(nsrc),
        const ndt::type *DYND_UNUSED(src_tp),
        const dynd::nd::array &DYND_UNUSED(kwds),
        const std::map<dynd::nd::string, ndt::type> &DYND_UNUSED(tp_vars))
    {
      dst_tp = ndt::make_type<R>();
    }
  };

  template <type_id_t I0, type_id_t I1>
  struct add_ck
      : base_kernel<add_ck<I0, I1>, kernel_request_cuda_host_device, 2> {
    typedef add_ck self_type;
    typedef typename type_of<I0>::type A0;
    typedef typename type_of<I1>::type A1;
    typedef decltype(std::declval<A0>() + std::declval<A1>()) R;

    DYND_CUDA_HOST_DEVICE void single(char *dst, char *const *src)
    {
      *reinterpret_cast<R *>(dst) =
          *reinterpret_cast<A0 *>(src[0]) + *reinterpret_cast<A1 *>(src[1]);
    }

    static void resolve_dst_type(
        const arrfunc_type_data *DYND_UNUSED(self),
        const ndt::arrfunc_type *DYND_UNUSED(self_tp), char *DYND_UNUSED(data),
        ndt::type &dst_tp, intptr_t DYND_UNUSED(nsrc),
        const ndt::type *DYND_UNUSED(src_tp),
        const dynd::nd::array &DYND_UNUSED(kwds),
        const std::map<dynd::nd::string, ndt::type> &DYND_UNUSED(tp_vars))
    {
      dst_tp = ndt::make_type<R>();
    }

    static ndt::type make_type()
    {
      std::map<string, ndt::type> tp_vars;
      tp_vars["A0"] = ndt::make_type<A0>();
      tp_vars["A1"] = ndt::make_type<A1>();
      tp_vars["R"] = ndt::make_type<R>();

      return ndt::substitute(ndt::type("(A0, A1) -> R"), tp_vars, true);
    }
  };

  template <type_id_t I0, type_id_t I1>
  struct subtract_ck
      : base_kernel<subtract_ck<I0, I1>, kernel_request_cuda_host_device, 2> {
    typedef subtract_ck self_type;
    typedef typename type_of<I0>::type A0;
    typedef typename type_of<I1>::type A1;
    typedef decltype(std::declval<A0>() - std::declval<A1>()) R;

    DYND_CUDA_HOST_DEVICE void single(char *dst, char *const *src)
    {
      *reinterpret_cast<R *>(dst) =
          *reinterpret_cast<A0 *>(src[0]) - *reinterpret_cast<A1 *>(src[1]);
    }

    static ndt::type make_type()
    {
      std::map<string, ndt::type> tp_vars;
      tp_vars["A0"] = ndt::make_type<A0>();
      tp_vars["A1"] = ndt::make_type<A1>();
      tp_vars["R"] = ndt::make_type<R>();

      return ndt::substitute(ndt::type("(A0, A1) -> R"), tp_vars, true);
    }

    static void resolve_dst_type(
        const arrfunc_type_data *DYND_UNUSED(self),
        const ndt::arrfunc_type *DYND_UNUSED(self_tp), char *DYND_UNUSED(data),
        ndt::type &dst_tp, intptr_t DYND_UNUSED(nsrc),
        const ndt::type *DYND_UNUSED(src_tp),
        const dynd::nd::array &DYND_UNUSED(kwds),
        const std::map<dynd::nd::string, ndt::type> &DYND_UNUSED(tp_vars))
    {
      dst_tp = ndt::make_type<R>();
    }
  };

  template <type_id_t I0, type_id_t I1>
  struct multiply_ck
      : base_kernel<multiply_ck<I0, I1>, kernel_request_cuda_host_device, 2> {
    typedef multiply_ck self_type;
    typedef typename type_of<I0>::type A0;
    typedef typename type_of<I1>::type A1;
    typedef decltype(std::declval<A0>() * std::declval<A1>()) R;

    DYND_CUDA_HOST_DEVICE void single(char *dst, char *const *src)
    {
      *reinterpret_cast<R *>(dst) =
          *reinterpret_cast<A0 *>(src[0]) * *reinterpret_cast<A1 *>(src[1]);
    }

    static ndt::type make_type()
    {
      std::map<string, ndt::type> tp_vars;
      tp_vars["A0"] = ndt::make_type<A0>();
      tp_vars["A1"] = ndt::make_type<A1>();
      tp_vars["R"] = ndt::make_type<R>();

      return ndt::substitute(ndt::type("(A0, A1) -> R"), tp_vars, true);
    }

    static void resolve_dst_type(
        const arrfunc_type_data *DYND_UNUSED(self),
        const ndt::arrfunc_type *DYND_UNUSED(self_tp), char *DYND_UNUSED(data),
        ndt::type &dst_tp, intptr_t DYND_UNUSED(nsrc),
        const ndt::type *DYND_UNUSED(src_tp),
        const dynd::nd::array &DYND_UNUSED(kwds),
        const std::map<dynd::nd::string, ndt::type> &DYND_UNUSED(tp_vars))
    {
      dst_tp = ndt::make_type<R>();
    }
  };

  template <type_id_t I0, type_id_t I1>
  struct divide_ck
      : base_kernel<divide_ck<I0, I1>, kernel_request_cuda_host_device, 2> {
    typedef divide_ck self_type;
    typedef typename type_of<I0>::type A0;
    typedef typename type_of<I1>::type A1;
    typedef decltype(std::declval<A0>() / std::declval<A1>()) R;

    DYND_CUDA_HOST_DEVICE void single(char *dst, char *const *src)
    {
      *reinterpret_cast<R *>(dst) =
          *reinterpret_cast<A0 *>(src[0]) / *reinterpret_cast<A1 *>(src[1]);
    }

    static ndt::type make_type()
    {
      std::map<string, ndt::type> tp_vars;
      tp_vars["A0"] = ndt::make_type<A0>();
      tp_vars["A1"] = ndt::make_type<A1>();
      tp_vars["R"] = ndt::make_type<R>();

      return ndt::substitute(ndt::type("(A0, A1) -> R"), tp_vars, true);
    }

    static void resolve_dst_type(
        const arrfunc_type_data *DYND_UNUSED(self),
        const ndt::arrfunc_type *DYND_UNUSED(self_tp), char *DYND_UNUSED(data),
        ndt::type &dst_tp, intptr_t DYND_UNUSED(nsrc),
        const ndt::type *DYND_UNUSED(src_tp),
        const dynd::nd::array &DYND_UNUSED(kwds),
        const std::map<dynd::nd::string, ndt::type> &DYND_UNUSED(tp_vars))
    {
      dst_tp = ndt::make_type<R>();
    }
  };

} // namespace dynd::nd
} // namespace dynd