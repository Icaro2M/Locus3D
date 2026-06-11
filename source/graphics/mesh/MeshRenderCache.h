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
    struct MeshRenderCacheKey
    {
        u64 ownerId = 0;
        u64 revision = 0;

        [[nodiscard]] bool is_valid() const;
    };

    [[nodiscard]] bool operator==(const MeshRenderCacheKey& lhs, const MeshRenderCacheKey& rhs);
    [[nodiscard]] bool operator!=(const MeshRenderCacheKey& lhs, const MeshRenderCacheKey& rhs);

    struct MeshRenderCacheKeyHash
    {
        [[nodiscard]] std::size_t operator()(const MeshRenderCacheKey& key) const;
    };

    class MeshRenderCache
    {
    public:
        MeshRenderCache() = default;
        ~MeshRenderCache();

        MeshRenderCache(const MeshRenderCache&) = delete;
        MeshRenderCache& operator=(const MeshRenderCache&) = delete;

        MeshRenderCache(MeshRenderCache&& other) noexcept;
        MeshRenderCache& operator=(MeshRenderCache&& other) noexcept;

        void clear();

        void begin_frame();
        void set_current_frame(u64 frameIndex);

        [[nodiscard]] GraphicsResult<void> upload_or_replace(
            const MeshRenderCacheKey& key,
            const MeshUploadData& uploadData,
            const MeshUploader& uploader
        );

        [[nodiscard]] GraphicsResult<GpuMesh*> get_or_upload(
            const MeshRenderCacheKey& key,
            const MeshUploadData& uploadData,
            const MeshUploader& uploader
        );

        [[nodiscard]] GpuMesh* find(const MeshRenderCacheKey& key);
        [[nodiscard]] const GpuMesh* find(const MeshRenderCacheKey& key) const;

        [[nodiscard]] bool contains(const MeshRenderCacheKey& key) const;
        [[nodiscard]] bool remove(const MeshRenderCacheKey& key);

        void touch(const MeshRenderCacheKey& key);
        void prune_unused(u64 maxUnusedFrames);

        [[nodiscard]] std::size_t size() const;
        [[nodiscard]] bool empty() const;
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