//
// Copyright (C) 2011-15 DyND Developers
// BSD 2-Clause License, see LICENSE.txt
//

#include <dynd/func/arrfunc.hpp>

namespace dynd {
namespace nd {

  typedef type_id_sequence<int8_type_id, int16_type_id, int32_type_id,
                           int64_type_id, float32_type_id, float64_type_id,
                           complex_float32_type_id,
                           complex_float64_type_id> arithmetic_type_ids;

  extern struct plus : declfunc<plus> {
    static arrfunc children[DYND_TYPE_ID_MAX + 1];
    static arrfunc default_child;

    static arrfunc make();
  } plus;

  extern struct minus : declfunc<minus> {
    static arrfunc children[DYND_TYPE_ID_MAX + 1];
    static arrfunc default_child;

    static arrfunc make();
  } minus;

  extern struct add : declfunc<add> {
    static arrfunc children[DYND_TYPE_ID_MAX + 1][DYND_TYPE_ID_MAX + 1];
    static arrfunc default_child;

    static arrfunc make();
  } add;

  extern struct subtract : declfunc<subtract> {
    static arrfunc children[DYND_TYPE_ID_MAX + 1][DYND_TYPE_ID_MAX + 1];
    static arrfunc default_child;

    static arrfunc make();
  } subtract;

  extern struct multiply : declfunc<multiply> {
    static arrfunc children[DYND_TYPE_ID_MAX + 1][DYND_TYPE_ID_MAX + 1];
    static arrfunc default_child;

    static arrfunc make();
  } multiply;

  extern struct divide : declfunc<divide> {
    static arrfunc children[DYND_TYPE_ID_MAX + 1][DYND_TYPE_ID_MAX + 1];
    static arrfunc default_child;

    static arrfunc make();
  } divide;

} // namespace nd
} // namespace dynd