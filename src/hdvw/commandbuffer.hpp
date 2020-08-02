#pragma once

#include <vulkan/vulkan.hpp>

#include <hdvw/renderpass.hpp>
#include <hdvw/framebuffer.hpp>

#include <memory>

namespace hd {
    struct CommandBufferCreateInfo {
        vk::CommandBuffer commandBuffer;
        vk::CommandPool commandPool;
        vk::Device device;
    };

    struct BarrierCreateInfo {
        vk::AccessFlags srcAccess;
        vk::AccessFlags dstAccess;
        vk::PipelineStageFlags srcStage;
        vk::PipelineStageFlags dstStage;
    };

    class CommandBuffer_t;
    typedef std::shared_ptr<CommandBuffer_t> CommandBuffer;

    struct RenderPassBeginInfo {
        RenderPass renderPass;
        Framebuffer framebuffer;
        vk::Offset2D offset{ 0, 0 };
        vk::Extent2D extent;
    };

    class CommandBuffer_t {
        private:
            vk::CommandBuffer _buffer;
            vk::CommandPool _cmdpool;
            vk::Device _device;

        public:
            static CommandBuffer conjure(CommandBufferCreateInfo ci) {
                return std::make_shared<CommandBuffer_t>(ci);
            }

            CommandBuffer_t(CommandBufferCreateInfo ci);

            void barrier(BarrierCreateInfo ci);

            void begin();

            void begin(vk::CommandBufferUsageFlags flags);

            void end();

            void reset(bool release = true);

            void beginRenderPass(RenderPassBeginInfo bi);

            void endRenderPass(CommandBuffer buffer);

            vk::CommandBuffer& raw();

            ~CommandBuffer_t();
    };
}
