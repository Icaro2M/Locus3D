/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

/**
 * @file Kernel.h
 * @brief Main aggregator header for the Locus3D kernel stage 1 API.
 *
 * Include this header when an application needs the complete public kernel API
 * surface, including common types, math, geometry, modeling, IO, and validation.
 */

/*
 * Kernel subsystem aggregators.
 */
#include "kernel/Common.h"
#include "kernel/Math.h"
#include "kernel/Geometry.h"
#include "kernel/Modeling.h"
#include "kernel/IO.h"
#include "kernel/Validation.h"
