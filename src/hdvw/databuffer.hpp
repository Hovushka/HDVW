#pragma once

#include <vulkan/vulkan.hpp>

#include <hdvw/allocator.hpp>
#include <hdvw/commandpool.hpp>
#include <hdvw/queue.hpp>
#include <hdvw/buffer.hpp>

#include <memory>

namespace hd {
    template<class Data>
    struct DataBufferCreateInfo {
        CommandPool commandPool;
        Queue queue;
        Allocator allocator;
        std::vector<Data> data;
        vk::BufferUsageFlags usage;
        VmaMemoryUsage memoryUsage = VMA_MEMORY_USAGE_GPU_ONLY;
    };

    template<class Data>
    class DataBuffer_t;

    template<class Data>
    using DataBuffer = std::shared_ptr<DataBuffer_t<Data>>;

    template<class Data>
    class DataBuffer_t {
        private:
            Buffer _buffer;
            uint64_t _entities;

        public:
            static DataBuffer<Data> conjure(DataBufferCreateInfo<Data> ci) {
                return std::make_shared<DataBuffer_t>(ci);
            }

            DataBuffer_t(DataBufferCreateInfo<Data> ci) {
                _entities = ci.data.size();
                uint64_t _size = sizeof(ci.data[0]) * ci.data.size();

                Buffer stagingBuffer = Buffer_t::conjure({
                        .allocator = ci.allocator,
                        .size = _size,
                        .bufferUsage = vk::BufferUsageFlagBits::eTransferSrc,
                        .memoryUsage = VMA_MEMORY_USAGE_CPU_ONLY,
                        });

                void* data = nullptr;
                ci.allocator->map(stagingBuffer->memory(), data);
                memcpy(data, ci.data.data(), (size_t) _size);
                ci.allocator->unmap(stagingBuffer->memory());

                _buffer = Buffer_t::conjure({
                        .allocator = ci.allocator,
                        .size = _size,
                        .bufferUsage = vk::BufferUsageFlagBits::eTransferDst | ci.usage,
                        .memoryUsage = ci.memoryUsage,
                        });

                CommandBuffer cmd = ci.commandPool->singleTimeBegin();
                cmd->copy({
                        .srcBuffer = stagingBuffer,
                        .dstBuffer = _buffer,
                        });
                ci.commandPool->singleTimeEnd(cmd, ci.queue);
            }

            vk::DeviceSize size() {
                return _buffer->size();
            }

            uint64_t count() {
                return _entities;
            }

            vk::Buffer raw() {
                return _buffer->raw();
            }

            VmaAllocation memory() {
                return _buffer->memory();
            }
    };
}
