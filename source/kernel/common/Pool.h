#pragma once

#include "kernel/common/Id.h"

#include <cassert>
#include <cstddef>
#include <utility>
#include <vector>

namespace locus::kernel {

    template <typename T, typename HandleT>
    class Pool {
    public:
        using ValueType = T;
        using HandleType = HandleT;

        Pool() = default;

        [[nodiscard]] HandleT add(const T& value)
        {
            const HandleT handle = make_handle(slots_.size());
            slots_.push_back(Slot{ value, true });
            ++activeCount_;
            return handle;
        }

        [[nodiscard]] HandleT add(T&& value)
        {
            const HandleT handle = make_handle(slots_.size());
            slots_.push_back(Slot{ std::move(value), true });
            ++activeCount_;
            return handle;
        }

        template <typename... Args>
        [[nodiscard]] HandleT emplace(Args&&... args)
        {
            const HandleT handle = make_handle(slots_.size());
            slots_.push_back(Slot{ T(std::forward<Args>(args)...), true });
            ++activeCount_;
            return handle;
        }

        [[nodiscard]] bool remove(HandleT handle)
        {
            if (!is_valid(handle)) {
                return false;
            }

            slots_[handle.id.value].active = false;
            --activeCount_;
            return true;
        }

        [[nodiscard]] bool is_valid(HandleT handle) const
        {
            return handle.is_valid()
                && handle.id.value < slots_.size()
                && slots_[handle.id.value].active;
        }

        [[nodiscard]] T& get(HandleT handle)
        {
            assert(is_valid(handle));
            return slots_[handle.id.value].value;
        }

        [[nodiscard]] const T& get(HandleT handle) const
        {
            assert(is_valid(handle));
            return slots_[handle.id.value].value;
        }

        [[nodiscard]] T& operator[](HandleT handle)
        {
            return get(handle);
        }

        [[nodiscard]] const T& operator[](HandleT handle) const
        {
            return get(handle);
        }

        [[nodiscard]] std::size_t count() const
        {
            return activeCount_;
        }

        [[nodiscard]] std::size_t slot_count() const
        {
            return slots_.size();
        }

        [[nodiscard]] bool empty() const
        {
            return activeCount_ == 0;
        }

        void clear()
        {
            slots_.clear();
            activeCount_ = 0;
        }

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
        struct Slot {
            T value{};
            bool active = false;
        };

        [[nodiscard]] static HandleT make_handle(std::size_t index)
        {
            return HandleT(static_cast<IdValue>(index));
        }

        std::vector<Slot> slots_{};
        std::size_t activeCount_ = 0;
    };

}