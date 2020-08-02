#include <hdvw/commandbuffer.hpp>
using namespace hd;

CommandBuffer_t::CommandBuffer_t(CommandBufferCreateInfo ci) {
    _device = ci.device;
    _buffer = ci.commandBuffer;
    _cmdpool = ci.commandPool;
}

void CommandBuffer_t::barrier(BarrierCreateInfo ci) {
    vk::MemoryBarrier midBarrier = {};
    midBarrier.pNext = NULL;
    midBarrier.srcAccessMask = ci.srcAccess;
    midBarrier.dstAccessMask = ci.dstAccess;

    _buffer.pipelineBarrier(
            ci.srcStage, ci.dstStage, vk::DependencyFlags{0},
            midBarrier, nullptr, nullptr
            );
}

void CommandBuffer_t::begin() {
    vk::CommandBufferBeginInfo bi = {};

    _buffer.begin(bi);
}

void CommandBuffer_t::begin(vk::CommandBufferUsageFlags flags) {
    vk::CommandBufferBeginInfo bi = {};
    bi.flags = flags;

    _buffer.begin(bi);
}

void CommandBuffer_t::end() {
    _buffer.end();
}

void CommandBuffer_t::reset(bool release) {
    if (!release) _buffer.reset(vk::CommandBufferResetFlags{0});
    else _buffer.reset(vk::CommandBufferResetFlagBits::eReleaseResources);
}

void CommandBuffer_t::beginRenderPass(RenderPassBeginInfo bi) {
    vk::RenderPassBeginInfo renderPassInfo = {};
    renderPassInfo.renderPass = bi.renderPass->raw();
    renderPassInfo.framebuffer = bi.framebuffer->raw();
    renderPassInfo.renderArea.offset = bi.offset;
    renderPassInfo.renderArea.extent = bi.extent;

    std::array<vk::ClearValue, 2> clearValues = {};
    clearValues[0].color = std::array<float, 4>{ 0.0f, 0.0f, 0.0f, 1.0f };
    clearValues[1].depthStencil = vk::ClearDepthStencilValue{ 1.0f, 0 };

    renderPassInfo.clearValueCount = clearValues.size();
    renderPassInfo.pClearValues = clearValues.data();

    _buffer.beginRenderPass(renderPassInfo, vk::SubpassContents::eInline);
}

void CommandBuffer_t::endRenderPass(CommandBuffer buffer) {
    buffer->raw().endRenderPass();
}

vk::CommandBuffer& CommandBuffer_t::raw() {
    return _buffer;
}

CommandBuffer_t::~CommandBuffer_t() {
    _device.free(_cmdpool, _buffer);
}
