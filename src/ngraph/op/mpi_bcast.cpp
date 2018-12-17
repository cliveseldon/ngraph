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

#include "ngraph/op/mpi_bcast.hpp"

using namespace std;
using namespace ngraph;

op::MPI_Broadcast::MPI_Broadcast(const shared_ptr<Node>& arg)
    : Op("MPI_Broadcast", check_single_output_args({arg}))
{
    constructor_validate_and_infer_types();
}

void op::MPI_Broadcast::validate_and_infer_types()
{
    NODE_VALIDATION_ASSERT(this,
                           get_input_element_type(0).is_dynamic() ||
                               get_input_element_type(0) == element::f32 ||
                               get_input_element_type(0) == element::f64)
        << "Only element types f32 and f64 are supported (argument element type: "
        << get_input_element_type(0) << ").";

    set_output_type(0, get_input_element_type(0), get_input_partial_shape(0));
}

shared_ptr<Node> op::MPI_Broadcast::copy_with_new_args(const NodeVector& new_args) const
{
    check_new_args_count(this, new_args);
    return make_shared<MPI_Broadcast>(new_args.at(0));
}