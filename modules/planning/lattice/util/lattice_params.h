/******************************************************************************
 * Copyright 2017 The Apollo Authors. All Rights Reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *****************************************************************************/

/**
 * @file
 **/

#ifndef MODULES_PLANNING_LATTICE_LATTICE_PARAMS_H_
#define MODULES_PLANNING_LATTICE_LATTICE_PARAMS_H_

namespace apollo {
namespace planning {

// TODO(all) move the fake FLAG variable to planning_gflags
static const double decision_horizon = 200.0;
static const double lateral_enter_lane_thred = 2.0;

}  // namespace planning
}  // namespace apollo

#endif /* MODULES_PLANNING_LATTICE_LATTICE_PARAMS_H_ */