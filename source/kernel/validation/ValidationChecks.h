/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

 /**
  * @file ValidationChecks.h
  * @brief Aggregates built-in validation checks for editable mesh validation.
  */

#include "kernel/validation/checks/geometry/DegenerateEditableFaceCheck.h"
#include "kernel/validation/checks/geometry/InvalidPositionCheck.h"
#include "kernel/validation/checks/lem/ElementReferenceCheck.h"
#include "kernel/validation/checks/lem/FaceCycleCheck.h"
#include "kernel/validation/checks/lem/HandleValidityCheck.h"
#include "kernel/validation/checks/lem/RadialCycleCheck.h"
#include "kernel/validation/checks/topology/ConnectivityConsistencyCheck.h"