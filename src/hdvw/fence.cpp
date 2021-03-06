#include <hdvw/fence.hpp>
using namespace hd;

Fence_t::Fence_t(FenceCreateInfo ci) {
    _device = ci.device->raw();
    
    vk::FenceCreateInfo fi = {};
    if (ci.state == FenceState::eSignaled)
        fi.flags = vk::FenceCreateFlagBits::eSignaled;

    _fence = _device.createFence(fi);
}

void Fence_t::wait() {
    _device.waitForFences(_fence, VK_TRUE, UINT64_MAX);
}

void Fence_t::reset() {
    _device.resetFences(_fence);
}

vk::Fence Fence_t::raw() {
    return _fence;
}

Fence_t::~Fence_t() {
    _device.destroy(_fence);
}
