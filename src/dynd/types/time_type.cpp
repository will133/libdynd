//
// Copyright (C) 2011-15 DyND Developers
// BSD 2-Clause License, see LICENSE.txt
//

#include <dynd/array.hpp>
#include <dynd/func/arrfunc.hpp>
#include <dynd/types/time_type.hpp>
#include <dynd/types/property_type.hpp>
#include <dynd/types/option_type.hpp>
#include <dynd/types/typevar_type.hpp>
#include <dynd/kernels/assignment_kernels.hpp>
#include <dynd/kernels/time_assignment_kernels.hpp>
#include <dynd/func/make_callable.hpp>
#include <dynd/parser_util.hpp>

using namespace std;
using namespace dynd;

ndt::time_type::time_type(datetime_tz_t timezone)
    : base_type(time_type_id, datetime_kind, 8, scalar_align_of<int64_t>::value,
                type_flag_scalar, 0, 0, 0),
      m_timezone(timezone)
{
}

ndt::time_type::~time_type() {}

void ndt::time_type::set_time(const char *DYND_UNUSED(arrmeta), char *data,
                              assign_error_mode errmode, int32_t hour,
                              int32_t minute, int32_t second,
                              int32_t tick) const
{
  if (errmode != assign_error_nocheck &&
      !time_hmst::is_valid(hour, minute, second, tick)) {
    stringstream ss;
    ss << "invalid input time " << hour << ":" << minute << ":" << second
       << ", ticks: " << tick;
    throw runtime_error(ss.str());
  }

  *reinterpret_cast<int64_t *>(data) =
      time_hmst::to_ticks(hour, minute, second, tick);
}

void ndt::time_type::set_from_utf8_string(
    const char *DYND_UNUSED(arrmeta), char *data, const char *utf8_begin,
    const char *utf8_end, const eval::eval_context *DYND_UNUSED(ectx)) const
{
  time_hmst hmst;
  const char *tz_begin = NULL, *tz_end = NULL;
  // TODO: Use errmode to adjust strictness of the parsing
  hmst.set_from_str(utf8_begin, utf8_end, tz_begin, tz_end);
  if (m_timezone != tz_abstract && tz_begin != tz_end) {
    if (m_timezone == tz_utc &&
        (parse::compare_range_to_literal(tz_begin, tz_end, "Z") ||
         parse::compare_range_to_literal(tz_begin, tz_end, "UTC"))) {
      // It's a UTC time to a UTC time zone
    } else {
      stringstream ss;
      ss << "DyND time zone support is partial, cannot handle ";
      ss.write(tz_begin, tz_end - tz_begin);
      throw runtime_error(ss.str());
    }
  }
  *reinterpret_cast<int64_t *>(data) = hmst.to_ticks();
}

time_hmst ndt::time_type::get_time(const char *DYND_UNUSED(arrmeta),
                                   const char *data) const
{
  time_hmst hmst;
  hmst.set_from_ticks(*reinterpret_cast<const int64_t *>(data));
  return hmst;
}

void ndt::time_type::print_data(std::ostream &o,
                                const char *DYND_UNUSED(arrmeta),
                                const char *data) const
{
  time_hmst hmst;
  hmst.set_from_ticks(*reinterpret_cast<const int64_t *>(data));
  o << hmst.to_str();
  if (m_timezone == tz_utc) {
    o << "Z";
  }
}

void ndt::time_type::print_type(std::ostream &o) const
{
  if (m_timezone == tz_abstract) {
    o << "time";
  } else {
    o << "time[tz='";
    switch (m_timezone) {
    case tz_utc:
      o << "UTC";
      break;
    default:
      o << "(invalid " << (int32_t)m_timezone << ")";
      break;
    }
    o << "']";
  }
}

bool ndt::time_type::is_lossless_assignment(const type &dst_tp,
                                            const type &src_tp) const
{
  if (dst_tp.extended() == this) {
    if (src_tp.extended() == this) {
      return true;
    } else if (src_tp.get_type_id() == time_type_id) {
      // There is only one possibility for the time type (TODO: timezones!)
      return true;
    } else {
      return false;
    }
  } else {
    return false;
  }
}

bool ndt::time_type::operator==(const base_type &rhs) const
{
  if (this == &rhs) {
    return true;
  } else if (rhs.get_type_id() != time_type_id) {
    return false;
  } else {
    const time_type &r = static_cast<const time_type &>(rhs);
    // TODO: When "other" timezone data is supported, need to compare them too
    return m_timezone == r.m_timezone;
  }
}

intptr_t ndt::time_type::make_assignment_kernel(
    const arrfunc_type_data *self, const arrfunc_type *af_tp, void *ckb,
    intptr_t ckb_offset, const type &dst_tp, const char *dst_arrmeta,
    const type &src_tp, const char *src_arrmeta, kernel_request_t kernreq,
    const eval::eval_context *ectx, const nd::array &kwds) const
{
  if (this == dst_tp.extended()) {
    if (src_tp.get_type_id() == time_type_id) {
      return make_pod_typed_data_assignment_kernel(
          ckb, ckb_offset, get_data_size(), get_data_alignment(), kernreq);
    } else if (src_tp.get_kind() == string_kind) {
      // Assignment from strings
      return make_string_to_time_assignment_kernel(
          ckb, ckb_offset, dst_tp, src_tp, src_arrmeta, kernreq, ectx);
    } else if (src_tp.get_kind() == struct_kind) {
      // Convert to struct using the "struct" property
      return ::make_assignment_kernel(
          self, af_tp, ckb, ckb_offset, make_property(dst_tp, "struct"),
          dst_arrmeta, src_tp, src_arrmeta, kernreq, ectx, kwds);
    } else if (!src_tp.is_builtin()) {
      return src_tp.extended()->make_assignment_kernel(
          self, af_tp, ckb, ckb_offset, dst_tp, dst_arrmeta, src_tp,
          src_arrmeta, kernreq, ectx, kwds);
    }
  } else {
    if (dst_tp.get_kind() == string_kind) {
      // Assignment to strings
      return make_time_to_string_assignment_kernel(
          ckb, ckb_offset, dst_tp, dst_arrmeta, src_tp, kernreq, ectx);
    } else if (dst_tp.get_kind() == struct_kind) {
      // Convert to struct using the "struct" property
      return ::make_assignment_kernel(
          self, af_tp, ckb, ckb_offset, dst_tp, dst_arrmeta,
          make_property(src_tp, "struct"), src_arrmeta, kernreq, ectx, kwds);
    }
    // TODO
  }

  stringstream ss;
  ss << "Cannot assign from " << src_tp << " to " << dst_tp;
  throw dynd::type_error(ss.str());
}

size_t ndt::time_type::make_comparison_kernel(
    void *ckb, intptr_t ckb_offset, const type &src0_tp,
    const char *src0_arrmeta, const type &src1_tp, const char *src1_arrmeta,
    comparison_type_t comptype, const eval::eval_context *ectx) const
{
  if (this == src0_tp.extended()) {
    if (*this == *src1_tp.extended()) {
      return make_builtin_type_comparison_kernel(ckb, ckb_offset, int64_type_id,
                                                 int64_type_id, comptype);
    } else if (!src1_tp.is_builtin()) {
      return src1_tp.extended()->make_comparison_kernel(
          ckb, ckb_offset, src0_tp, src0_arrmeta, src1_tp, src1_arrmeta,
          comptype, ectx);
    }
  }

  throw not_comparable_error(src0_tp, src1_tp, comptype);
}

///////// properties on the type

void ndt::time_type::get_dynamic_type_properties(
    const std::pair<std::string, gfunc::callable> **out_properties,
    size_t *out_count) const
{
  *out_properties = NULL;
  *out_count = 0;
}

///////// functions on the type

void ndt::time_type::get_dynamic_type_functions(
    const std::pair<std::string, gfunc::callable> **out_functions,
    size_t *out_count) const
{
  *out_functions = NULL;
  *out_count = 0;
}

///////// properties on the nd::array

static nd::array property_ndo_get_hour(const nd::array &n)
{
  return n.replace_dtype(ndt::make_property(n.get_dtype(), "hour"));
}

static nd::array property_ndo_get_minute(const nd::array &n)
{
  return n.replace_dtype(ndt::make_property(n.get_dtype(), "minute"));
}

static nd::array property_ndo_get_second(const nd::array &n)
{
  return n.replace_dtype(ndt::make_property(n.get_dtype(), "second"));
}

static nd::array property_ndo_get_microsecond(const nd::array &n)
{
  return n.replace_dtype(ndt::make_property(n.get_dtype(), "microsecond"));
}

static nd::array property_ndo_get_tick(const nd::array &n)
{
  return n.replace_dtype(ndt::make_property(n.get_dtype(), "tick"));
}

void ndt::time_type::get_dynamic_array_properties(
    const std::pair<std::string, gfunc::callable> **out_properties,
    size_t *out_count) const
{
  static pair<string, gfunc::callable> time_array_properties[] = {
      pair<string, gfunc::callable>(
          "hour", gfunc::make_callable(&property_ndo_get_hour, "self")),
      pair<string, gfunc::callable>(
          "minute", gfunc::make_callable(&property_ndo_get_minute, "self")),
      pair<string, gfunc::callable>(
          "second", gfunc::make_callable(&property_ndo_get_second, "self")),
      pair<string, gfunc::callable>(
          "microsecond",
          gfunc::make_callable(&property_ndo_get_microsecond, "self")),
      pair<string, gfunc::callable>(
          "tick", gfunc::make_callable(&property_ndo_get_tick, "self"))};

  *out_properties = time_array_properties;
  *out_count = sizeof(time_array_properties) / sizeof(time_array_properties[0]);
}

///////// functions on the nd::array

static nd::array function_ndo_to_struct(const nd::array &n)
{
  return n.replace_dtype(ndt::make_property(n.get_dtype(), "struct"));
}

void ndt::time_type::get_dynamic_array_functions(
    const std::pair<std::string, gfunc::callable> **out_functions,
    size_t *out_count) const
{
  static pair<string, gfunc::callable> time_array_functions[] = {
      pair<string, gfunc::callable>(
          "to_struct", gfunc::make_callable(&function_ndo_to_struct, "self")),
  };

  *out_functions = time_array_functions;
  *out_count = sizeof(time_array_functions) / sizeof(time_array_functions[0]);
}

///////// property accessor kernels (used by property_type)

namespace {

struct time_get_hour_kernel
    : nd::base_kernel<time_get_hour_kernel, kernel_request_host, 1> {
  void single(char *dst, char *const *src)
  {
    int64_t ticks = **reinterpret_cast<int64_t *const *>(src);
    *reinterpret_cast<int32_t *>(dst) =
        static_cast<int32_t>(ticks / DYND_TICKS_PER_HOUR);
  }
};

struct time_get_minute_kernel
    : nd::base_kernel<time_get_minute_kernel, kernel_request_host, 1> {
  void single(char *dst, char *const *src)
  {
    int64_t ticks = **reinterpret_cast<int64_t *const *>(src);
    *reinterpret_cast<int32_t *>(dst) =
        static_cast<int32_t>((ticks / DYND_TICKS_PER_MINUTE) % 60);
  }
};

struct time_get_second_kernel
    : nd::base_kernel<time_get_second_kernel, kernel_request_host, 1> {
  void single(char *dst, char *const *src)
  {
    int64_t ticks = **reinterpret_cast<int64_t *const *>(src);
    *reinterpret_cast<int32_t *>(dst) =
        static_cast<int32_t>((ticks / DYND_TICKS_PER_SECOND) % 60);
  }
};

struct time_get_microsecond_kernel
    : nd::base_kernel<time_get_microsecond_kernel, kernel_request_host, 1> {
  void single(char *dst, char *const *src)
  {
    int64_t ticks = **reinterpret_cast<int64_t *const *>(src);
    *reinterpret_cast<int32_t *>(dst) =
        static_cast<int32_t>((ticks / DYND_TICKS_PER_MICROSECOND) % 1000000);
  }
};

struct time_get_tick_kernel
    : nd::base_kernel<time_get_tick_kernel, kernel_request_host, 1> {
  void single(char *dst, char *const *src)
  {
    int64_t ticks = **reinterpret_cast<int64_t *const *>(src);
    *reinterpret_cast<int32_t *>(dst) = static_cast<int32_t>(ticks % 10000000);
  }
};

struct time_get_struct_kernel
    : nd::base_kernel<time_get_struct_kernel, kernel_request_host, 1> {
  void single(char *dst, char *const *src)
  {
    time_hmst *dst_struct = reinterpret_cast<time_hmst *>(dst);
    dst_struct->set_from_ticks(**reinterpret_cast<int64_t *const *>(src));
  }
};

struct time_set_struct_kernel
    : nd::base_kernel<time_set_struct_kernel, kernel_request_host, 1> {
  void single(char *dst, char *const *src)
  {
    time_hmst *src_struct = *reinterpret_cast<time_hmst *const *>(src);
    *reinterpret_cast<int64_t *>(dst) = src_struct->to_ticks();
  }
};

} // anonymous namespace

namespace {
enum time_properties_t {
  timeprop_hour,
  timeprop_minute,
  timeprop_second,
  timeprop_microsecond,
  timeprop_tick,
  timeprop_struct
};
}

size_t ndt::time_type::get_elwise_property_index(
    const std::string &property_name) const
{
  if (property_name == "hour") {
    return timeprop_hour;
  } else if (property_name == "minute") {
    return timeprop_minute;
  } else if (property_name == "second") {
    return timeprop_second;
  } else if (property_name == "microsecond") {
    return timeprop_microsecond;
  } else if (property_name == "tick") {
    return timeprop_tick;
  } else if (property_name == "struct") {
    // A read/write property for accessing a time as a struct
    return timeprop_struct;
  } else {
    stringstream ss;
    ss << "dynd time type does not have a kernel for property "
       << property_name;
    throw runtime_error(ss.str());
  }
}

ndt::type ndt::time_type::get_elwise_property_type(size_t property_index,
                                                   bool &out_readable,
                                                   bool &out_writable) const
{
  switch (property_index) {
  case timeprop_hour:
  case timeprop_minute:
  case timeprop_second:
  case timeprop_microsecond:
  case timeprop_tick:
    out_readable = true;
    out_writable = false;
    return make_type<int32_t>();
  case timeprop_struct:
    out_readable = true;
    out_writable = true;
    return time_hmst::type();
  default:
    out_readable = false;
    out_writable = false;
    return make_type<void>();
  }
}

size_t ndt::time_type::make_elwise_property_getter_kernel(
    void *ckb, intptr_t ckb_offset, const char *DYND_UNUSED(dst_arrmeta),
    const char *DYND_UNUSED(src_arrmeta), size_t src_property_index,
    kernel_request_t kernreq, const eval::eval_context *DYND_UNUSED(ectx)) const
{
  switch (src_property_index) {
  case timeprop_hour:
    time_get_hour_kernel::make(ckb, kernreq, ckb_offset);
    return ckb_offset;
  case timeprop_minute:
    time_get_minute_kernel::make(ckb, kernreq, ckb_offset);
    return ckb_offset;
  case timeprop_second:
    time_get_second_kernel::make(ckb, kernreq, ckb_offset);
    return ckb_offset;
  case timeprop_microsecond:
    time_get_microsecond_kernel::make(ckb, kernreq, ckb_offset);
    return ckb_offset;
  case timeprop_tick:
    time_get_tick_kernel::make(ckb, kernreq, ckb_offset);
    return ckb_offset;
  case timeprop_struct:
    time_get_struct_kernel::make(ckb, kernreq, ckb_offset);
    return ckb_offset;
  default:
    stringstream ss;
    ss << "dynd time type given an invalid property index"
       << src_property_index;
    throw runtime_error(ss.str());
  }
}

size_t ndt::time_type::make_elwise_property_setter_kernel(
    void *ckb, intptr_t ckb_offset, const char *DYND_UNUSED(dst_arrmeta),
    size_t dst_property_index, const char *DYND_UNUSED(src_arrmeta),
    kernel_request_t kernreq, const eval::eval_context *DYND_UNUSED(ectx)) const
{
  switch (dst_property_index) {
  case timeprop_struct:
    time_set_struct_kernel::make(ckb, kernreq, ckb_offset);
    return ckb_offset;
  default:
    stringstream ss;
    ss << "dynd time type given an invalid property index"
       << dst_property_index;
    throw runtime_error(ss.str());
  }
}

namespace {
struct time_is_avail_ck
    : nd::base_kernel<time_is_avail_ck, kernel_request_host, 1> {
  void single(char *dst, char *const *src)
  {
    int64_t v = **reinterpret_cast<int64_t *const *>(src);
    *dst = v != DYND_TIME_NA;
  }

  void strided(char *dst, intptr_t dst_stride, char *const *src,
               const intptr_t *src_stride, size_t count)
  {
    char *src0 = src[0];
    intptr_t src0_stride = src_stride[0];
    for (size_t i = 0; i != count; ++i) {
      int64_t v = *reinterpret_cast<int64_t *>(src0);
      *dst = v != DYND_TIME_NA;
      dst += dst_stride;
      src0 += src0_stride;
    }
  }
};

struct time_assign_na_ck
    : nd::base_kernel<time_assign_na_ck, kernel_request_host, 1> {
  void single(char *dst, char *const *DYND_UNUSED(src))
  {
    *reinterpret_cast<int64_t *>(dst) = DYND_TIME_NA;
  }

  void strided(char *dst, intptr_t dst_stride, char *const *DYND_UNUSED(src),
               const intptr_t *DYND_UNUSED(src_stride), size_t count)
  {
    for (size_t i = 0; i != count; ++i, dst += dst_stride) {
      *reinterpret_cast<int64_t *>(dst) = DYND_TIME_NA;
    }
  }
};
} // anonymous namespace

nd::array ndt::time_type::get_option_nafunc() const
{
  nd::array naf = nd::empty(option_type::make_nafunc_type());
  arrfunc_type_data *is_avail =
      reinterpret_cast<arrfunc_type_data *>(naf.get_ndo()->m_data_pointer);
  arrfunc_type_data *assign_na = is_avail + 1;

  new (is_avail)
      arrfunc_type_data(0, &time_is_avail_ck::instantiate, NULL, NULL);
  new (assign_na)
      arrfunc_type_data(0, &time_assign_na_ck::instantiate, NULL, NULL);
  return naf;
}
