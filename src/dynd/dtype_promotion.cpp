//
// Copyright (C) 2011-12, Dynamic NDArray Developers
// BSD 2-Clause License, see LICENSE.txt
//

#include <stdexcept>

#include <dynd/dtype_promotion.hpp>
#include <dynd/dtypes/string_dtype.hpp>

using namespace std;
using namespace dynd;

/*
static intptr_t min_strlen_for_builtin_kind(dtype_kind_t kind)
{
    switch (kind) {
        case bool_kind:
            return 1;
        case int_kind:
        case uint_kind:
            return 24;
        case real_kind:
            return 32;
        case complex_kind:
            return 64;
        default:
            throw runtime_error("cannot get minimum string length for specified kind");
    }
}
*/

dtype dynd::promote_dtypes_arithmetic(const dtype& dt0, const dtype& dt1)
{
    // Use the value dtypes
    const dtype& dt0_val = dt0.value_dtype();
    const dtype& dt1_val = dt1.value_dtype();

    const extended_dtype *dt0_ext, *dt1_ext;

    dt0_ext = dt0_val.extended();
    dt1_ext = dt1_val.extended();

    //cout << "Doing type promotion with value types " << dt0_val << " and " << dt1_val << endl;

    if (dt0_ext == NULL && dt1_ext == NULL) {
        const size_t int_size = sizeof(int);
        switch (dt0_val.kind()) {
            case bool_kind:
                switch (dt1_val.kind()) {
                    case bool_kind:
                        return make_dtype<int>();
                    case int_kind:
                    case uint_kind:
                        return (dt1_val.element_size() >= int_size) ? dt1_val
                                                               : make_dtype<int>();
                    case void_kind:
                        return dt0_val;
                    default:
                        return dt1_val;
                }
            case int_kind:
                switch (dt1_val.kind()) {
                    case bool_kind:
                        return (dt0_val.element_size() >= int_size) ? dt0_val
                                                               : make_dtype<int>();
                    case int_kind:
                        if (dt0_val.element_size() < int_size && dt1_val.element_size() < int_size) {
                            return make_dtype<int>();
                        } else {
                            return (dt0_val.element_size() >= dt1_val.element_size()) ? dt0_val
                                                                              : dt1_val;
                        }
                    case uint_kind:
                        if (dt0_val.element_size() < int_size && dt1_val.element_size() < int_size) {
                            return make_dtype<int>();
                        } else {
                            // When the element_sizes are equal, the uint kind wins
                            return (dt0_val.element_size() > dt1_val.element_size()) ? dt0_val
                                                                             : dt1_val;
                        }
                    case real_kind:
                        // Integer type sizes don't affect float type sizes
                        return dt1_val;
                    case complex_kind:
                        // Integer type sizes don't affect complex type sizes
                        return dt1_val;
                    case void_kind:
                        return dt0_val;
                    default:
                        break;
                }
                break;
            case uint_kind:
                switch (dt1_val.kind()) {
                    case bool_kind:
                        return (dt0_val.element_size() >= int_size) ? dt0_val
                                                               : make_dtype<int>();
                    case int_kind:
                        if (dt0_val.element_size() < int_size && dt1_val.element_size() < int_size) {
                            return make_dtype<int>();
                        } else {
                            // When the element_sizes are equal, the uint kind wins
                            return (dt0_val.element_size() >= dt1_val.element_size()) ? dt0_val
                                                                              : dt1_val;
                        }
                    case uint_kind:
                        if (dt0_val.element_size() < int_size && dt1_val.element_size() < int_size) {
                            return make_dtype<int>();
                        } else {
                            return (dt0_val.element_size() >= dt1_val.element_size()) ? dt0_val
                                                                              : dt1_val;
                        }
                    case real_kind:
                        // Integer type sizes don't affect float type sizes
                        return dt1_val;
                    case complex_kind:
                        // Integer type sizes don't affect complex type sizes
                        return dt1_val;
                    case void_kind:
                        return dt0_val;
                    default:
                        break;
                }
                break;
            case real_kind:
                switch (dt1_val.kind()) {
                    // Integer type sizes don't affect float type sizes
                    case bool_kind:
                    case int_kind:
                    case uint_kind:
                        return dt0_val;
                    case real_kind:
                        return (dt0_val.element_size() >= dt1_val.element_size()) ? dt0_val
                                                                          : dt1_val;
                    case complex_kind:
                        if (dt0_val.type_id() == float64_type_id && dt1_val.type_id() == complex_float32_type_id) {
                            return dtype(complex_float64_type_id);
                        } else {
                            return dt1_val;
                        }
                    case void_kind:
                        return dt0_val;
                    default:
                        break;
                }
                break;
            case complex_kind:
                switch (dt1_val.kind()) {
                    // Integer and float type sizes don't affect complex type sizes
                    case bool_kind:
                    case int_kind:
                    case uint_kind:
                    case real_kind:
                        if (dt0_val.type_id() == complex_float32_type_id && dt1_val.type_id() == float64_type_id) {
                            return dtype(complex_float64_type_id);
                        } else {
                            return dt0_val;
                        }
                    case complex_kind:
                        return (dt0_val.element_size() >= dt1_val.element_size()) ? dt0_val
                                                                          : dt1_val;
                    case void_kind:
                        return dt0_val;
                    default:
                        break;
                }
                break;
            case void_kind:
                return dt1_val;
            default:
                break;
        }

        stringstream ss;
        ss << "internal error in built-in dtype promotion of " << dt0_val << " and " << dt1_val;
        throw std::runtime_error(ss.str());
    }

    // HACK for getting simple string dtype promotions.
    // TODO: Do this properly in a pluggable manner.
    if (dt0_val.type_id() == string_type_id && dt1_val.type_id() == string_type_id) {
        const extended_string_dtype *ext0 = static_cast<const extended_string_dtype *>(
                        dt0_val.extended());
        const extended_string_dtype *ext1 = static_cast<const extended_string_dtype *>(
                        dt1_val.extended());
        if (ext0->encoding() > ext1->encoding()) {
            return dt0_val;
        } else {
            return dt1_val;
        }
    }

    // In general, if one type is void, just return the other type
    if (dt0_val.type_id() == void_type_id) {
        return dt1_val;
    } else if (dt1_val.type_id() == void_type_id) {
        return dt0_val;
    }

    throw std::runtime_error("type promotion of custom dtypes is not yet supported");
}