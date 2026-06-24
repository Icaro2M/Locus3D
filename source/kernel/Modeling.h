/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

/**
 * @file Modeling.h
 * @brief Aggregator header for the kernel modeling system.
 *
 * This header collects operation core types, concrete modeling operations, and
 * non-destructive preview infrastructure.
 */

/*
 * Operation core.
 */
#include "kernel/modeling/core/IOperation.h"
#include "kernel/modeling/core/OperationContext.h"
#include "kernel/modeling/core/OperationResult.h"

/*
 * Concrete operations and preview support.
 */
#include "kernel/Operations.h"
#include "kernel/Preview.h"
