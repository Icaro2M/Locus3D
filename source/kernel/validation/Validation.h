/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

 /**
  * @file Validation.h
  * @brief Main aggregation header for the kernel validation module.
  *
  * This header exposes the public validation API, including core validation
  * types, built-in checks, and pipeline utilities.
  */

#include "kernel/validation/ValidationChecks.h"
#include "kernel/validation/ValidationCore.h"

#include "kernel/validation/pipeline/ValidationMode.h"
#include "kernel/validation/pipeline/ValidationPipeline.h"