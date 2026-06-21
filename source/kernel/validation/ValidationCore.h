/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

 /**
  * @file ValidationCore.h
  * @brief Aggregates the core types shared by validation checks and validation pipelines.
  *
  * This header exposes the minimal validation API required to define checks,
  * configure validation execution, and inspect validation reports.
  */

#include "kernel/validation/core/IValidationCheck.h"
#include "kernel/validation/core/ValidationContext.h"
#include "kernel/validation/core/ValidationIssue.h"
#include "kernel/validation/core/ValidationReport.h"
#include "kernel/validation/core/ValidationSeverity.h"