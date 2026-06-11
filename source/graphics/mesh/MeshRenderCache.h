/*
 * SPDX-FileCopyrightText: 2026 Icaro
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "graphics/common/GraphicsResult.h"
#include "graphics/common/GraphicsTypes.h"
#include "graphics/mesh/GpuMesh.h"
#include "graphics/mesh/MeshUploadData.h"
#include "graphics/mesh/MeshUploader.h"

#include <cstddef>
#include <functional>
#include <unordered_map>

namespace locus::graphics
{
    /**
     * @brief Identifies a cached GPU mesh for one source mesh revision.
     */
    struct MeshRenderCacheKey
    {
        u64 ownerId = 0;
        u64 revision = 0;

        /**
         * @brief Checks whether the key can identify a cache entry.
         *
         * @return True when the owner id is non-zero.
         */
        [[nodiscard]] bool is_valid() const;
    };

    /**
     * @brief Compares two mesh cache keys.
     *
     * @param lhs First key.
     * @param rhs Second key.
     * @return True when both keys refer to the same owner and revision.
     */
    [[nodiscard]] bool operator==(const MeshRenderCacheKey& lhs, const MeshRenderCacheKey& rhs);

    /**
     * @brief Compares two mesh cache keys for inequality.
     *
     * @param lhs First key.
     * @param rhs Second key.
     * @return True when the keys differ.
     */
    [[nodiscard]] bool operator!=(const MeshRenderCacheKey& lhs, const MeshRenderCacheKey& rhs);

    /**
     * @brief Hash functor for MeshRenderCacheKey.
     */
    struct MeshRenderCacheKeyHash
    {
        /**
         * @brief Computes a hash value for a mesh cache key.
         *
         * @param key Cache key to hash.
         * @return Combined hash of owner id and revision.
         */
        [[nodiscard]] std::size_t operator()(const MeshRenderCacheKey& key) const;
    };

    /**
     * @brief Caches uploaded GPU meshes by source object and revision.
     *
     * MeshRenderCache avoids rebuilding GPU buffers when the source mesh has
     * not changed, while still allowing stale entries to be pruned by frame age.
     *
     * @note The cache owns every GpuMesh stored in it.
     */
    class MeshRenderCache
    {
    public:
        MeshRenderCache() = default;
        ~MeshRenderCache();

        MeshRenderCache(const MeshRenderCache&) = delete;
        MeshRenderCache& operator=(const MeshRenderCache&) = delete;

        MeshRenderCache(MeshRenderCache&& other) noexcept;
        MeshRenderCache& operator=(MeshRenderCache&& other) noexcept;

        /**
         * @brief Destroys every cached GPU mesh and resets frame tracking.
         */
        void clear();

        /**
         * @brief Advances the cache frame counter.
         */
        void begin_frame();

        /**
         * @brief Sets the current frame counter explicitly.
         *
         * @param frameIndex Frame index to store.
         */
        void set_current_frame(u64 frameIndex);

        /**
         * @brief Uploads mesh data and replaces any existing entry for the key.
         *
         * @param key Cache key for the uploaded mesh.
         * @param uploadData CPU mesh data to upload.
         * @param uploader Mesh uploader used to create the GPU mesh.
         * @return Empty result on success, or a graphics error on failure.
         */
        [[nodiscard]] GraphicsResult<void> upload_or_replace(
            const MeshRenderCacheKey& key,
            const MeshUploadData& uploadData,
            const MeshUploader& uploader
        );

        /**
         * @brief Finds a cached mesh or uploads it if missing.
         *
         * @param key Cache key for the requested mesh.
         * @param uploadData CPU mesh data used when the entry is missing.
         * @param uploader Mesh uploader used to create the GPU mesh.
         * @return Pointer to the cached GPU mesh, or a graphics error on failure.
         */
        [[nodiscard]] GraphicsResult<GpuMesh*> get_or_upload(
            const MeshRenderCacheKey& key,
            const MeshUploadData& uploadData,
            const MeshUploader& uploader
        );

        /**
         * @brief Finds a mutable cached mesh and marks it used this frame.
         *
         * @param key Cache key to find.
         * @return Pointer to the cached mesh, or nullptr when absent.
         */
        [[nodiscard]] GpuMesh* find(const MeshRenderCacheKey& key);

        /**
         * @brief Finds a cached mesh without updating frame usage.
         *
         * @param key Cache key to find.
         * @return Pointer to the cached mesh, or nullptr when absent.
         */
        [[nodiscard]] const GpuMesh* find(const MeshRenderCacheKey& key) const;

        /**
         * @brief Checks whether a key exists in the cache.
         *
         * @param key Cache key to test.
         * @return True when a record exists.
         */
        [[nodiscard]] bool contains(const MeshRenderCacheKey& key) const;

        /**
         * @brief Removes one cached mesh.
         *
         * @param key Cache key to remove.
         * @return True when an entry was removed.
         */
        [[nodiscard]] bool remove(const MeshRenderCacheKey& key);

        /**
         * @brief Marks an entry as used in the current frame.
         *
         * @param key Cache key to touch.
         */
        void touch(const MeshRenderCacheKey& key);

        /**
         * @brief Removes entries that have not been used recently.
         *
         * @param maxUnusedFrames Maximum number of frames an entry may remain unused.
         */
        void prune_unused(u64 maxUnusedFrames);

        /**
         * @brief Returns the number of cached meshes.
         *
         * @return Cache record count.
         */
        [[nodiscard]] std::size_t size() const;

        /**
         * @brief Checks whether the cache has no records.
         *
         * @return True when the cache is empty.
         */
        [[nodiscard]] bool empty() const;

        /**
         * @brief Returns the current frame index used for pruning.
         *
         * @return Current frame counter.
         */
        [[nodiscard]] u64 current_frame() const;

    private:
        struct Record
        {
            GpuMesh mesh;
            u64 lastUsedFrame = 0;
        };

    private:
        std::unordered_map<MeshRenderCacheKey, Record, MeshRenderCacheKeyHash> records_;
        u64 currentFrame_ = 0;
    };
}
