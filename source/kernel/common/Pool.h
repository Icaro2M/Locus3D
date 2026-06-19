/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "kernel/common/Id.h"

#include <cassert>
#include <cstddef>
#include <utility>
#include <vector>

namespace locus::kernel {

    /**
     * @brief Sparse handle-addressed storage for kernel objects.
     *
     * Pool preserves slot indices after removal so existing handles remain stable
     * for validity checks. Removed slots are not reused by this implementation.
     *
     * @tparam T Stored value type.
     * @tparam HandleT Typed handle that wraps an Id.
     */
    template <typename T, typename HandleT>
    class Pool {
    public:
        /**
         * @brief Stored value type.
         */
        using ValueType = T;
        /**
         * @brief Handle type used to address values.
         */
        using HandleType = HandleT;

        Pool() = default;

        /**
         * @brief Adds a copied value to the pool.
         *
         * @param value Value to store.
         * @return Handle referencing the created slot.
         */
        [[nodiscard]] HandleT add(const T& value)
        {
            const HandleT handle = make_handle(slots_.size());
            slots_.push_back(Slot{ value, true });
            ++activeCount_;
            return handle;
        }

        /**
         * @brief Adds a moved value to the pool.
         *
         * @param value Value to store.
         * @return Handle referencing the created slot.
         */
        [[nodiscard]] HandleT add(T&& value)
        {
            const HandleT handle = make_handle(slots_.size());
            slots_.push_back(Slot{ std::move(value), true });
            ++activeCount_;
            return handle;
        }

        /**
         * @brief Constructs a value directly in a new pool slot.
         *
         * @tparam Args Constructor argument types.
         * @param args Arguments forwarded to the value constructor.
         * @return Handle referencing the created slot.
         */
        template <typename... Args>
        [[nodiscard]] HandleT emplace(Args&&... args)
        {
            const HandleT handle = make_handle(slots_.size());
            slots_.push_back(Slot{ T(std::forward<Args>(args)...), true });
            ++activeCount_;
            return handle;
        }

        /**
         * @brief Marks a slot as inactive.
         *
         * @param handle Handle to remove.
         * @return True when the handle referenced an active slot.
         */
        [[nodiscard]] bool remove(HandleT handle)
        {
            if (!is_valid(handle)) {
                return false;
            }

            slots_[handle.id.value].active = false;
            --activeCount_;
            return true;
        }

        /**
         * @brief Checks whether a handle references an active slot.
         *
         * @param handle Handle to test.
         * @return True when the handle is in range and active.
         */
        [[nodiscard]] bool is_valid(HandleT handle) const
        {
            return handle.is_valid()
                && handle.id.value < slots_.size()
                && slots_[handle.id.value].active;
        }

        /**
         * @brief Returns mutable access to a stored value.
         *
         * @param handle Handle referencing an active slot.
         * @return Mutable stored value.
         */
        [[nodiscard]] T& get(HandleT handle)
        {
            assert(is_valid(handle));
            return slots_[handle.id.value].value;
        }

        /**
         * @brief Returns read-only access to a stored value.
         *
         * @param handle Handle referencing an active slot.
         * @return Read-only stored value.
         */
        [[nodiscard]] const T& get(HandleT handle) const
        {
            assert(is_valid(handle));
            return slots_[handle.id.value].value;
        }

        /**
         * @brief Returns mutable access to a stored value.
         *
         * @param handle Handle referencing an active slot.
         * @return Mutable stored value.
         */
        [[nodiscard]] T& operator[](HandleT handle)
        {
            return get(handle);
        }

        /**
         * @brief Returns read-only access to a stored value.
         *
         * @param handle Handle referencing an active slot.
         * @return Read-only stored value.
         */
        [[nodiscard]] const T& operator[](HandleT handle) const
        {
            return get(handle);
        }

        /**
         * @brief Returns the number of active values.
         *
         * @return Active slot count.
         */
        [[nodiscard]] std::size_t count() const
        {
            return activeCount_;
        }

        /**
         * @brief Returns the total number of allocated slots.
         *
         * @return Slot count including inactive slots.
         */
        [[nodiscard]] std::size_t slot_count() const
        {
            return slots_.size();
        }

        /**
         * @brief Checks whether the pool contains no active values.
         *
         * @return True when activeCount is zero.
         */
        [[nodiscard]] bool empty() const
        {
            return activeCount_ == 0;
        }

        /**
         * @brief Removes all slots and invalidates every handle.
         */
        void clear()
        {
            slots_.clear();
            activeCount_ = 0;
        }

        /**
         * @brief Returns handles for all active slots.
         *
         * @return Active handles in slot order.
         */
        [[nodiscard]] std::vector<HandleT> handles() const
        {
            std::vector<HandleT> result;
            result.reserve(activeCount_);

            for (std::size_t index = 0; index < slots_.size(); ++index) {
                if (slots_[index].active) {
                    result.push_back(make_handle(index));
                }
            }

            return result;
        }

    private:
        /**
         * @brief Stored slot payload and active flag.
         */
        struct Slot {
            T value{};
            bool active = false;
        };

        /**
         * @brief Converts a slot index to a typed handle.
         *
         * @param index Slot index.
         * @return Typed handle for the slot.
         */
        [[nodiscard]] static HandleT make_handle(std::size_t index)
        {
            return HandleT(static_cast<IdValue>(index));
        }

        std::vector<Slot> slots_{};
        std::size_t activeCount_ = 0;
    };

}
