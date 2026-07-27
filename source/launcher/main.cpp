/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#include "application/ApplicationResult.h"
#include "application/runtime/ApplicationRuntime.h"

#include <iostream>

int main()
{
    locus::application::ApplicationRuntime runtime{};

    const locus::application::ApplicationResult<void> initializeResult =
        runtime.initialize();

    if (!initializeResult) {
        std::cerr << initializeResult.error().message << '\n';
        return 1;
    }

    const locus::application::ApplicationResult<int> runResult =
        runtime.run();

    if (!runResult) {
        std::cerr << runResult.error().message << '\n';
        return 1;
    }

    return runResult.value();
}
