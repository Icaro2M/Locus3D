/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

/**
 * @file IO.h
 * @brief Aggregator header for implemented mesh import and export support.
 *
 * This header collects importer/exporter interfaces, STL and OBJ format
 * implementations, and format registry utilities.
 */

/*
 * Mesh import and export interfaces.
 */
#include "kernel/io/IExporter.h"
#include "kernel/io/IImporter.h"

/*
 * Implemented mesh formats.
 */
#include "kernel/io/StlExporter.h"
#include "kernel/io/StlImporter.h"
#include "kernel/io/ObjExporter.h"
#include "kernel/io/ObjImporter.h"

/*
 * Format registration.
 */
#include "kernel/io/FormatRegistry.h"
