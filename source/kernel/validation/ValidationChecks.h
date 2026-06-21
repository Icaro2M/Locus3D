/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

 /**
  * @file ValidationChecks.h
  * @brief Aggregates the built-in validation checks provided by the kernel.
  *
  * This header exposes concrete validation checks grouped under the validation
  * module. Include it when a caller needs to instantiate or register individual
  * checks directly.
  */

#include "kernel/validation/checks/geometry/DegenerateEditableFaceCheck.h"
#include "kernel/validation/checks/geometry/InvalidPositionCheck.h"

#include "kernel/validation/checks/lem/ElementReferenceCheck.h"
#include "kernel/validation/checks/lem/FaceCycleCheck.h"
#include "kernel/validation/checks/lem/HandleValidityCheck.h"
#include "kernel/validation/checks/lem/RadialCycleCheck.h"

#include "kernel/validation/checks/topology/ConnectivityConsistencyCheck.h"