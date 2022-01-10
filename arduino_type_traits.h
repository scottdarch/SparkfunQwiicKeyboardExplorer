/*
 *                                                              [h][o][t][ ]
 *                                                              [k][e][y][s]
 * ----------------------------------------------------------------------------
 * Copyright (c) 2022 Scott A Dixon.
 * All Rights Reserved.
 *
 * This software is distributed under the terms of the MIT License.
 */

#ifndef GH_SCOTTDARCH_ARDUINO_TYPE_TRAITS_H
#define GH_SCOTTDARCH_ARDUINO_TYPE_TRAITS_H

namespace gh
{
namespace scottdarch
{
/**
 * From https://en.cppreference.com/w/cpp/types/aligned_storage, aligned_storage implementation
 * because Arduino doesn't include type_traits.
 */
template<size_t Len, size_t Align>
struct aligned_storage {
    struct type {
        alignas(Align) unsigned char data[Len];
    };
};
};  // namespace scottdarch
};  // namespace io

#endif  // GH_SCOTTDARCH_ARDUINO_TYPE_TRAITS_H
