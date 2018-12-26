//*****************************************************************************
// Copyright 2017-2018 Intel Corporation
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//*****************************************************************************

#pragma once

#include <Eigen/Dense>
#include <cmath>
#include <omp.h>
#include <utility>

#include "ngraph/runtime/reference/broadcast.hpp"
#include "ngraph/shape_util.hpp"
#include "ngraph/util.hpp"

namespace ngraph
{
    namespace runtime
    {
        namespace rpi
        {
            namespace kernel
            {
                std::tuple<size_t, size_t> get_start_finish(size_t size)
                {
                    const size_t nthreads = omp_get_num_threads();
                    const size_t ithread = omp_get_thread_num();
                    const size_t start = ithread * size / nthreads;
                    const size_t finish = (ithread + 1) * size / nthreads;
                    return std::make_tuple(start, finish);
                }
                template <typename T>
                void broadcast_2d(const T* in,
                                  T* out,
                                  const Shape& in_shape,
                                  const Shape& out_shape,
                                  const AxisSet& broadcast_axes)
                {
                    size_t index[2];
                    size_t* out_index =
                        (broadcast_axes.find(0) == broadcast_axes.end() ? &index[0] : &index[1]);
                    for (index[0] = 0; index[0] < out_shape[0]; ++index[0])
                    {
                        for (index[1] = 0; index[1] < out_shape[1]; ++index[1])
                        {
                            out[index[0] * out_shape[1] + index[1]] = in[*out_index];
                        }
                    }
                }

                template <typename T>
                void broadcast_3d(const T* in,
                                  T* out,
                                  const Shape& in_shape,
                                  const Shape& out_shape,
                                  const AxisSet& broadcast_axes)
                {
#ifdef PARALLEL
#pragma omp parallel
#endif
                    {
                        size_t start;
                        size_t finish;
#ifdef PARALLEL
                        std::tie(start, finish) = get_start_finish(out_shape[0]);
#else
                        start = 0;
                        finish = out_shape[0];
#endif
                        if (start != finish)
                        {
                            NGRAPH_INFO << omp_get_thread_num() << " start=" << start
                                        << ", finish=" << finish;
                            size_t i0;
                            size_t i1;
                            size_t i2;
                            size_t* index[3] = {&i0, &i1, &i2};
                            size_t* out_index;
                            for (size_t i = 0; i < 3; i++)
                            {
                                if (broadcast_axes.count(i) == 0)
                                {
                                    out_index = index[i];
                                    NGRAPH_INFO << omp_get_thread_num() << ", " << i << ", "
                                                << out_index;
                                    break;
                                }
                            }
                            NGRAPH_INFO << omp_get_thread_num() << ", i0=" << &i0 << ", i1=" << &i1
                                        << ", i2=" << &i2 << " *out_index=" << out_index;
                            for (i0 = start; i0 < finish; ++i0)
                            {
                                for (i1 = 0; i1 < out_shape[1]; ++i1)
                                {
                                    for (i2 = 0; i2 < out_shape[2]; ++i2)
                                    {
                                        // out[i0 * out_shape[1] * out_shape[2] + i1 * out_shape[2] +
                                        //     i2] = in[*out_index];
                                        *out = in[*out_index];
                                        out++;
                                    }
                                }
                            }
                            NGRAPH_INFO;
                        }
                    }
                    NGRAPH_INFO;
                }

                template <typename T>
                void broadcast_4d(const T* in,
                                  T* out,
                                  const Shape& in_shape,
                                  const Shape& out_shape,
                                  const AxisSet& broadcast_axes)
                {
                    size_t index[4];
                    size_t* out_index;
                    for (size_t i = 0; i < 4; i++)
                    {
                        if (broadcast_axes.count(i) == 0)
                        {
                            out_index = &index[i];
                            break;
                        }
                    }
                    for (index[0] = 0; index[0] < out_shape[0]; ++index[0])
                    {
                        for (index[1] = 0; index[1] < out_shape[1]; ++index[1])
                        {
                            for (index[2] = 0; index[2] < out_shape[2]; ++index[2])
                            {
                                for (index[3] = 0; index[3] < out_shape[3]; ++index[3])
                                {
                                    out[index[0] * out_shape[1] * out_shape[2] * out_shape[3] +
                                        index[1] * out_shape[2] * out_shape[3] +
                                        index[2] * out_shape[3] + index[3]] = in[*out_index];
                                }
                            }
                        }
                    }
                }

                template <typename T>
                void broadcast(const T* in,
                               T* out,
                               const Shape& in_shape,
                               const Shape& out_shape,
                               const AxisSet& broadcast_axes)
                {
                    if (in_shape.size() == 0)
                    {
                        for (size_t i = 0; i < shape_size(out_shape); ++i)
                        {
                            out[i] = in[0];
                        }
                    }
                    else if (in_shape.size() == 1)
                    {
                        switch (out_shape.size())
                        {
                        case 2:
                            broadcast_2d<T>(in, out, in_shape, out_shape, broadcast_axes);
                            break;
                        case 3:
                            broadcast_3d<T>(in, out, in_shape, out_shape, broadcast_axes);
                            break;
                        case 4:
                            broadcast_4d<T>(in, out, in_shape, out_shape, broadcast_axes);
                            break;
                        }
                    }
                    else
                    {
                        NGRAPH_INFO << "reference Broadcast";
                        NGRAPH_INFO << "in_shape=" << in_shape << ", out_shape=" << out_shape
                                    << ",  broadcast_axes(" << broadcast_axes.size()
                                    << ")=" << join(broadcast_axes);
                        runtime::reference::broadcast<T>(
                            in, out, in_shape, out_shape, broadcast_axes);
                    }
                }
            }
        }
    }
}