//
// Copyright (C) 2011-15 DyND Developers
// BSD 2-Clause License, see LICENSE.txt
//

#pragma once

#include <dynd/func/arrfunc.hpp>
#include <dynd/kernels/apply.hpp>

namespace dynd {
namespace nd {
  namespace functional {
    /**
     * Makes an arrfunc out of function ``func``, using the provided keyword
     * parameter names. This function takes ``func`` as a template
     * parameter, so can call it efficiently.
     */
    template <kernel_request_t kernreq, typename func_type, func_type func,
              typename... T>
    arrfunc apply(T &&... names)
    {
      typedef as_apply_function_ck<kernreq, func_type, func,
                                   arity_of<func_type>::value - sizeof...(T)>
          CKT;

      ndt::type self_tp =
          ndt::make_arrfunc<typename funcproto_of<func_type>::type>(
              std::forward<T>(names)...);

      return arrfunc::make<CKT>(self_tp, 0);
    }

    template <typename func_type, func_type func, typename... T>
    arrfunc apply(T &&... names)
    {
      return apply<kernel_request_host, func_type, func>(
          std::forward<T>(names)...);
    }

    /**
     * Makes an arrfunc out of the function object ``func``, using the provided
     * keyword parameter names. This version makes a copy of provided ``func``
     * object.
     */
    template <kernel_request_t kernreq, typename func_type, typename... T>
    typename std::enable_if<!is_function_pointer<func_type>::value,
                            arrfunc>::type
    apply(func_type func, T &&... names)
    {
      typedef as_apply_callable_ck<kernreq, func_type,
                                   arity_of<func_type>::value - sizeof...(T)>
          ck_type;

      ndt::type self_tp =
          ndt::make_arrfunc<kernreq, typename funcproto_of<func_type>::type>(
              std::forward<T>(names)...);

      return arrfunc::make<ck_type>(self_tp, func, 0);
    }

    template <typename func_type, typename... T>
    typename std::enable_if<!is_function_pointer<func_type>::value,
                            arrfunc>::type
    apply(func_type func, T &&... names)
    {
      static_assert(all_char_string_params<T...>::value,
                    "All the names must be strings");
      return apply<kernel_request_host>(func, std::forward<T>(names)...);
    }

    template <kernel_request_t kernreq, typename func_type, typename... T>
    arrfunc apply(func_type *func, T &&... names)
    {
      typedef as_apply_callable_ck<kernreq, func_type *,
                                   arity_of<func_type>::value - sizeof...(T)>
          ck_type;

      ndt::type self_tp =
          ndt::make_arrfunc<kernreq, typename funcproto_of<func_type>::type>(
              std::forward<T>(names)...);

      return arrfunc::make<ck_type>(self_tp, func, 0);
    }


/*
    template <kernel_request_t kernreq, typename func_type, typename... T>
    arrfunc apply(func_type *func, T &&... names)
    {
      return apply<kernreq>(func, static_cast<arrfunc_free_t>(NULL),
                            std::forward<T>(names)...);
    }

    template <typename func_type, typename... T>
    arrfunc apply(func_type *func, arrfunc_free_t free, T &&... names)
    {
      return apply<kernel_request_host>(func, free, std::forward<T>(names)...);
    }
*/

    template <typename func_type, typename... T>
    arrfunc apply(func_type *func, T &&... names)
    {
      return apply<kernel_request_host>(func, std::forward<T>(names)...);
    }

    template <kernel_request_t kernreq, typename T, typename R, typename... A,
              typename... S>
    arrfunc apply(T *obj, R (T::*mem_func)(A...), S &&... names)
    {
      typedef as_apply_member_function_ck<kernreq, T *, R (T::*)(A...),
                                          sizeof...(A) - sizeof...(S)> ck_type;

      ndt::type self_tp = ndt::make_arrfunc<
          kernreq, typename funcproto_of<R (T::*)(A...)>::type>(
          std::forward<S>(names)...);

      return arrfunc::make<ck_type>(self_tp,
                                 typename ck_type::data_type(obj, mem_func), 0);
    }

    template <typename O, typename R, typename... A, typename... T>
    arrfunc apply(O *obj, R (O::*mem_func)(A...), T &&... names)
    {
      return apply<kernel_request_host>(obj, mem_func,
                                        std::forward<T>(names)...);
    }

    /**
     * Makes an arrfunc out of the provided function object type, specialized
     * for a memory_type such as cuda_device based on the ``kernreq``.
     */
    template <kernel_request_t kernreq, typename func_type, typename... K,
              typename... T>
    arrfunc apply(T &&... names)
    {
      typedef as_construct_then_apply_callable_ck<kernreq, func_type, K...>
          ck_type;

      ndt::type self_tp = ndt::make_arrfunc<
          kernreq, typename funcproto_of<func_type, K...>::type>(
          std::forward<T>(names)...);

      return arrfunc::make<ck_type>(self_tp, 0);
    }

    /**
     * Makes an arrfunc out of the provided function object type, which
     * constructs and calls the function object on demand.
     */
    template <typename func_type, typename... K, typename... T>
    arrfunc apply(T &&... names)
    {
      return apply<kernel_request_host, func_type, K...>(
          std::forward<T>(names)...);
    }
  } // namespace dynd::nd::functional
} // namespace dynd::nd
} // namespace dynd