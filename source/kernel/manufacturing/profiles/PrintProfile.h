/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "kernel/manufacturing/profiles/FDMProfile.h"
#include "kernel/manufacturing/profiles/SLAProfile.h"
#include "kernel/manufacturing/profiles/SLSProfile.h"
#include "kernel/manufacturing/profiles/PrintTechnology.h"

#include <string_view>
#include <type_traits>
#include <utility>
#include <variant>

namespace locus::kernel::manufacturing {

    /**
     * @brief Technology-specific manufacturing profile storage.
     */
    using PrintProfileData =
        std::variant<FDMProfile, SLAProfile, SLSProfile>;

    /**
     * @brief Value-type wrapper around a technology-specific manufacturing
     * analysis profile.
     *
     * PrintProfile provides a common interface without introducing virtual
     * ownership or runtime downcasting. Technology-specific data remains
     * available to analyzers through get_if().
     */
    class PrintProfile {
    public:
        /**
         * @brief Creates an FDM manufacturing profile.
         */
        explicit PrintProfile(FDMProfile profile)
            : data_(std::move(profile))
        {
        }

        /**
         * @brief Creates an SLA manufacturing profile.
         */
        explicit PrintProfile(SLAProfile profile)
            : data_(std::move(profile))
        {
        }

        /**
         * @brief Creates an SLS manufacturing profile.
         */
        explicit PrintProfile(SLSProfile profile)
            : data_(std::move(profile))
        {
        }

        /**
         * @brief Returns the represented manufacturing technology.
         */
        [[nodiscard]] PrintTechnology technology() const noexcept
        {
            return std::visit(
                [](const auto& profile) {
                    using ProfileType = std::decay_t<decltype(profile)>;
                    return ProfileType::technology;
                },
                data_);
        }

        /**
         * @brief Returns the profile's user-facing name.
         */
        [[nodiscard]] std::string_view name() const noexcept
        {
            return std::visit(
                [](const auto& profile) -> std::string_view {
                    return profile.name;
                },
                data_);
        }

        /**
         * @brief Returns generic manufacturing limits.
         */
        [[nodiscard]] const ManufacturingLimits& limits() const noexcept
        {
            return std::visit(
                [](const auto& profile)
                -> const ManufacturingLimits& {
                    return profile.limits;
                },
                data_);
        }

        /**
         * @brief Returns mutable generic manufacturing limits.
         */
        [[nodiscard]] ManufacturingLimits& limits() noexcept
        {
            return std::visit(
                [](auto& profile)
                -> ManufacturingLimits& {
                    return profile.limits;
                },
                data_);
        }

        /**
         * @brief Checks whether the profile contains a specific technology
         * profile type.
         *
         * @tparam ProfileType FDMProfile, SLAProfile, or SLSProfile.
         */
        template <typename ProfileType>
        [[nodiscard]] bool is() const noexcept
        {
            return std::holds_alternative<ProfileType>(data_);
        }

        /**
         * @brief Returns a technology-specific profile when it matches the
         * requested type.
         *
         * @tparam ProfileType FDMProfile, SLAProfile, or SLSProfile.
         * @return Profile pointer, or nullptr when the stored type differs.
         */
        template <typename ProfileType>
        [[nodiscard]] const ProfileType* get_if() const noexcept
        {
            return std::get_if<ProfileType>(&data_);
        }

        /**
         * @brief Returns a mutable technology-specific profile when it
         * matches the requested type.
         *
         * @tparam ProfileType FDMProfile, SLAProfile, or SLSProfile.
         * @return Profile pointer, or nullptr when the stored type differs.
         */
        template <typename ProfileType>
        [[nodiscard]] ProfileType* get_if() noexcept
        {
            return std::get_if<ProfileType>(&data_);
        }

        /**
         * @brief Returns the underlying technology-specific profile variant.
         */
        [[nodiscard]] const PrintProfileData& data() const noexcept
        {
            return data_;
        }

    private:
        PrintProfileData data_;
    };

}