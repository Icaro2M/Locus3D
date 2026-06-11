/*
 * SPDX-FileCopyrightText: 2026 Icaro
 * SPDX-License-Identifier: Apache-2.0
 */

#include "graphics/mesh/MeshRenderCache.h"

#include "graphics/common/GraphicsError.h"

#include <utility>

namespace locus::graphics
{
    bool MeshRenderCacheKey::is_valid() const
    {
        return ownerId != 0;
    }

    bool operator==(const MeshRenderCacheKey& lhs, const MeshRenderCacheKey& rhs)
    {
        return lhs.ownerId == rhs.ownerId && lhs.revision == rhs.revision;
    }

    bool operator!=(const MeshRenderCacheKey& lhs, const MeshRenderCacheKey& rhs)
    {
        return !(lhs == rhs);
    }

    std::size_t MeshRenderCacheKeyHash::operator()(const MeshRenderCacheKey& key) const
    {
        const std::size_t ownerHash = std::hash<u64>{}(key.ownerId);
        const std::size_t revisionHash = std::hash<u64>{}(key.revision);

        // Boost-style hash combine keeps owner and revision changes visible in one key.
        return ownerHash ^ (revisionHash + 0x9e3779b97f4a7c15ull + (ownerHash << 6) + (ownerHash >> 2));
    }

    MeshRenderCache::~MeshRenderCache()
    {
        clear();
    }

    MeshRenderCache::MeshRenderCache(MeshRenderCache&& other) noexcept
    {
        *this = std::move(other);
    }

    MeshRenderCache& MeshRenderCache::operator=(MeshRenderCache&& other) noexcept
    {
        if (this == &other)
        {
            return *this;
        }

        clear();

        records_ = std::move(other.records_);
        currentFrame_ = other.currentFrame_;

        other.currentFrame_ = 0;

        return *this;
    }

    void MeshRenderCache::clear()
    {
        for (auto& item : records_)
        {
            item.second.mesh.destroy();
        }

        records_.clear();
        currentFrame_ = 0;
    }

    void MeshRenderCache::begin_frame()
    {
        ++currentFrame_;
    }

    void MeshRenderCache::set_current_frame(u64 frameIndex)
    {
        currentFrame_ = frameIndex;
    }

    GraphicsResult<void> MeshRenderCache::upload_or_replace(
        const MeshRenderCacheKey& key,
        const MeshUploadData& uploadData,
        const MeshUploader& uploader
    )
    {
        if (!key.is_valid())
        {
            return GraphicsError::make(
                GraphicsErrorCode::InvalidArgument,
                "Cannot upload mesh to MeshRenderCache with an invalid key."
            );
        }

        if (uploadData.is_empty())
        {
            return GraphicsError::make(
                GraphicsErrorCode::InvalidArgument,
                "Cannot upload empty MeshUploadData to MeshRenderCache."
            );
        }

        auto meshResult = uploader.upload(uploadData);
        if (!meshResult)
        {
            return meshResult.error();
        }

        auto it = records_.find(key);
        if (it != records_.end())
        {
            // Replace in-place so external lookup semantics stay tied to the same cache key.
            it->second.mesh.destroy();
            it->second.mesh = meshResult.move_value();
            it->second.lastUsedFrame = currentFrame_;
            return {};
        }

        auto insertResult = records_.emplace(key, Record{});
        insertResult.first->second.mesh = meshResult.move_value();
        insertResult.first->second.lastUsedFrame = currentFrame_;

        return {};
    }

    GraphicsResult<GpuMesh*> MeshRenderCache::get_or_upload(
        const MeshRenderCacheKey& key,
        const MeshUploadData& uploadData,
        const MeshUploader& uploader
    )
    {
        GpuMesh* cachedMesh = find(key);
        if (cachedMesh != nullptr)
        {
            return cachedMesh;
        }

        auto uploadResult = upload_or_replace(key, uploadData, uploader);
        if (!uploadResult)
        {
            return uploadResult.error();
        }

        cachedMesh = find(key);
        if (cachedMesh == nullptr)
        {
            return GraphicsError::make(
                GraphicsErrorCode::ResourceNotFound,
                "MeshRenderCache failed to find mesh after upload."
            );
        }

        return cachedMesh;
    }

    GpuMesh* MeshRenderCache::find(const MeshRenderCacheKey& key)
    {
        auto it = records_.find(key);
        if (it == records_.end())
        {
            return nullptr;
        }

        it->second.lastUsedFrame = currentFrame_;
        return &it->second.mesh;
    }

    const GpuMesh* MeshRenderCache::find(const MeshRenderCacheKey& key) const
    {
        auto it = records_.find(key);
        if (it == records_.end())
        {
            return nullptr;
        }

        return &it->second.mesh;
    }

    bool MeshRenderCache::contains(const MeshRenderCacheKey& key) const
    {
        return records_.find(key) != records_.end();
    }

    bool MeshRenderCache::remove(const MeshRenderCacheKey& key)
    {
        auto it = records_.find(key);
        if (it == records_.end())
        {
            return false;
        }

        it->second.mesh.destroy();
        records_.erase(it);

        return true;
    }

    void MeshRenderCache::touch(const MeshRenderCacheKey& key)
    {
        auto it = records_.find(key);
        if (it == records_.end())
        {
            return;
        }

        it->second.lastUsedFrame = currentFrame_;
    }

    void MeshRenderCache::prune_unused(u64 maxUnusedFrames)
    {
        for (auto it = records_.begin(); it != records_.end();)
        {
            const u64 lastUsedFrame = it->second.lastUsedFrame;
            const bool shouldRemove =
                currentFrame_ > lastUsedFrame &&
                currentFrame_ - lastUsedFrame > maxUnusedFrames;

            if (shouldRemove)
            {
                it->second.mesh.destroy();
                it = records_.erase(it);
                continue;
            }

            ++it;
        }
    }

    std::size_t MeshRenderCache::size() const
    {
        return records_.size();
    }

    bool MeshRenderCache::empty() const
    {
        return records_.empty();
    }

    u64 MeshRenderCache::current_frame() const
    {
        return currentFrame_;
    }
}
