#include <hdvw/attachment.hpp>
using namespace hd;

Attachment_t::Attachment_t(AttachmentCreateInfo ci) {
    _device = ci.device->raw();
    _allocator = ci.allocator;

    if (_allocator != nullptr) {
        vk::ImageCreateInfo ici = {};
        ici.imageType = vk::ImageType::e2D;
        ici.extent.width = ci.extent.width;
        ici.extent.height = ci.extent.height;
        ici.extent.depth = 1;
        ici.mipLevels = 1;
        ici.arrayLayers = 1;
        ici.format = ci.format;
        ici.tiling = vk::ImageTiling::eOptimal;
        ici.initialLayout = vk::ImageLayout::eUndefined;
        ici.usage = ci.usage;
        ici.samples = vk::SampleCountFlagBits::e1;
        ici.sharingMode = vk::SharingMode::eExclusive;

        auto res = _allocator->create(ici, VMA_MEMORY_USAGE_GPU_ONLY);
        _image = res.image;
        _imageMemory = res.allocation;
    } else _image = ci.image;

    {
        vk::ImageSubresourceRange sr = {};
        sr.aspectMask = ci.aspect;
        sr.baseMipLevel = 0;
        sr.levelCount = 1;
        sr.baseArrayLayer = 0;
        sr.layerCount = 1;

        vk::ImageViewCreateInfo ivci;
        ivci.image = _image;
        ivci.viewType = vk::ImageViewType::e2D;
        ivci.format = ci.format;
        ivci.subresourceRange = sr;

        _view = _device.createImageView(ivci);
    }
}

vk::Image& Attachment_t::raw() {
    return _image;
}

vk::ImageView& Attachment_t::view() {
    return _view;
}

Attachment_t::~Attachment_t() {
    _device.destroy(_view);

    if (_allocator != nullptr)
        _allocator->destroy(_image, _imageMemory);
}
