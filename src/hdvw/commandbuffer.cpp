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

void CommandBuffer_t::transitionImageLayout(TransitionImageLayoutInfo ci) {
    vk::ImageMemoryBarrier barrier = {};
    barrier.oldLayout = ci.image->layout();
    barrier.newLayout = ci.layout;
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.image = ci.image->raw();
    barrier.subresourceRange = ci.image->range();

    vk::PipelineStageFlags sourceStage;
    vk::PipelineStageFlags destinationStage;

    if (ci.image->layout() == vk::ImageLayout::eUndefined && ci.layout == vk::ImageLayout::eTransferDstOptimal) {
        barrier.srcAccessMask = vk::AccessFlags{0};
        barrier.dstAccessMask = vk::AccessFlagBits::eTransferWrite;

        sourceStage = vk::PipelineStageFlagBits::eTopOfPipe;
        destinationStage = vk::PipelineStageFlagBits::eTransfer;
    } else if (ci.image->layout() == vk::ImageLayout::eUndefined && ci.layout == vk::ImageLayout::eGeneral) {
        barrier.srcAccessMask = vk::AccessFlags{0};
        barrier.dstAccessMask = vk::AccessFlags{0};

        sourceStage = vk::PipelineStageFlagBits::eTopOfPipe;
        destinationStage = vk::PipelineStageFlagBits::eTopOfPipe;
    } else if (ci.image->layout() == vk::ImageLayout::eTransferDstOptimal && ci.layout == vk::ImageLayout::eGeneral) {
        barrier.srcAccessMask = vk::AccessFlagBits::eTransferWrite;
        barrier.dstAccessMask = vk::AccessFlags{0};

        sourceStage = vk::PipelineStageFlagBits::eTransfer;
        destinationStage = vk::PipelineStageFlagBits::eTopOfPipe;
    } else if (ci.image->layout() == vk::ImageLayout::eTransferDstOptimal && ci.layout == vk::ImageLayout::eTransferSrcOptimal) {
        barrier.srcAccessMask = vk::AccessFlagBits::eTransferWrite;
        barrier.dstAccessMask = vk::AccessFlagBits::eTransferRead;

        sourceStage = vk::PipelineStageFlagBits::eTransfer;
        destinationStage = vk::PipelineStageFlagBits::eTransfer;
    } else if (ci.image->layout() == vk::ImageLayout::eTransferDstOptimal && ci.layout == vk::ImageLayout::eShaderReadOnlyOptimal) {
        barrier.srcAccessMask = vk::AccessFlagBits::eTransferWrite;
        barrier.dstAccessMask = vk::AccessFlagBits::eShaderRead;

        sourceStage = vk::PipelineStageFlagBits::eTransfer;
        destinationStage = vk::PipelineStageFlagBits::eFragmentShader;
    } else {
        throw std::invalid_argument("unsupported layout transition!");
    }

    _buffer.pipelineBarrier(sourceStage, destinationStage, vk::DependencyFlags{0}, nullptr, nullptr, barrier);
    ci.image->setLayout(ci.layout);
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

void CommandBuffer_t::copy(CopyBufferToBufferInfo ci) {
    vk::BufferCopy copyRegion = {};
    copyRegion.srcOffset = ci.srcOffset;
    copyRegion.dstOffset = ci.dstOffset;

    if (ci.size == 0)
        copyRegion.size = std::min(ci.srcBuffer->size(), ci.dstBuffer->size());
    else copyRegion.size = ci.size;

    _buffer.copyBuffer(ci.srcBuffer->raw(), ci.dstBuffer->raw(), copyRegion);
}

void CommandBuffer_t::copy(CopyBufferToImageInfo ci) {
    vk::BufferImageCopy region = {};
    region.bufferOffset = 0;
    region.bufferRowLength = 0;
    region.bufferImageHeight = 0;

    region.imageSubresource.aspectMask = ci.image->range().aspectMask;
    region.imageSubresource.mipLevel = ci.image->range().baseMipLevel;
    region.imageSubresource.baseArrayLayer = ci.image->range().baseArrayLayer;
    region.imageSubresource.layerCount = ci.image->range().layerCount;

    region.imageOffset = vk::Offset3D{0, 0, 0};
    region.imageExtent = vk::Extent3D{ci.image->extent().width, ci.image->extent().height, 1};

    _buffer.copyBufferToImage(ci.buffer->raw(), ci.image->raw(), ci.image->layout(), region);
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

vk::CommandBuffer CommandBuffer_t::raw() {
    return _buffer;
}

CommandBuffer_t::~CommandBuffer_t() {
    _device.free(_cmdpool, _buffer);
}
