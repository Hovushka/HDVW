#pragma once

#include <external/vk_mem_alloc.h>

#include <vulkan/vulkan.hpp>

#include <hdvw/instance.hpp>
#include <hdvw/device.hpp>

#include <memory>

namespace hd {
    struct AllocatorCreateInfo {
        Instance instance;
        Device device;
    };

    struct ReturnImage {
        vk::Image image;
        VmaAllocation allocation;
    };

    struct ReturnBuffer {
        vk::Buffer buffer;
        VmaAllocation allocation;
    };

    class Allocator_t;
    typedef std::shared_ptr<Allocator_t> Allocator;

    class Allocator_t {
        private:
            VmaAllocator _allocator;

        public:
            static Allocator conjure(AllocatorCreateInfo ci) {
                return std::make_shared<Allocator_t>(ci);
            }

            Allocator_t(AllocatorCreateInfo ci);

            ReturnImage create(vk::ImageCreateInfo ici, VmaMemoryUsage flag);

            ReturnBuffer create(vk::BufferCreateInfo ici, VmaMemoryUsage flag);

            void map(VmaAllocation alloc, void* &data);

            void unmap(VmaAllocation alloc);

            void destroy(vk::Image img, VmaAllocation alloc);

            void destroy(vk::Buffer buff, VmaAllocation alloc);

            ~Allocator_t();
    };
}
