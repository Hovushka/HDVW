#pragma once

#include <vulkan/vulkan.hpp>

#include <external/vk_mem_alloc.h>

#include <iostream>
#include <vector>

#include <hdvw/window.hpp>
#include <hdvw/instance.hpp>
#include <hdvw/surface.hpp>
#include <hdvw/device.hpp>
#include <hdvw/allocator.hpp>
#include <hdvw/queue.hpp>
#include <hdvw/commandpool.hpp>
#include <hdvw/commandbuffer.hpp>
#include <hdvw/swapchain.hpp>
#include <hdvw/renderpass.hpp>
#include <hdvw/framebuffer.hpp>
#include <hdvw/shader.hpp>
#include <hdvw/pipelinelayout.hpp>
#include <hdvw/pipeline.hpp>
#include <hdvw/semaphore.hpp>
#include <hdvw/fence.hpp>

#define MAX_FRAMES_IN_FLIGHT 3

class App {
    private:
        bool framebufferResized = false;

        hd::Window window;
        hd::Instance instance;
        hd::Surface surface;
        hd::Device device;
        hd::Allocator allocator;
        hd::Queue graphicsQueue;
        hd::Queue presentQueue;
        hd::CommandPool graphicsPool;

        std::vector<hd::Semaphore> imageAvailable;
        std::vector<hd::Semaphore> renderFinished;
        std::vector<hd::Fence> inFlightFences;

        void init() {
            window = hd::Window_t::conjure({
                    .width = 1280,
                    .height = 720,
                    .title = "Neo Water",
                    .cursorVisible = true,
                    .windowUser = this,
                    .framebufferSizeCallback = framebufferResizeCallback,
                    });

            instance = hd::Instance_t::conjure({
                    .applicationName = "Neo Water",
                    .applicationVersion = VK_MAKE_VERSION(1, 0, 0),
                    .engineName = "Hova's Engine",
                    .engineVersion = VK_MAKE_VERSION(2, 0, 0),
                    .apiVersion = VK_API_VERSION_1_1,
                    .validationLayers = { "VK_LAYER_KHRONOS_validation" },
                    .extensions = window->getRequiredExtensions(),
                    });

            surface = hd::Surface_t::conjure({
                    .window = window,
                    .instance = instance,
                    });

            device = hd::Device_t::conjure({
                    .instance = instance,
                    .surface = surface,
                    .findQueueFamilies = customFindQueueFamilies,
                    .extensions = { VK_KHR_SWAPCHAIN_EXTENSION_NAME },
                    .features = vk::PhysicalDeviceFeatures({ .samplerAnisotropy = VK_TRUE }),
                    .validationLayers = { "VK_LAYER_KHRONOS_validation" },
                    });

            allocator = hd::Allocator_t::conjure({
                    .instance = instance,
                    .device = device,
                    });

            graphicsQueue = hd::Queue_t::conjure({
                    .device = device,
                    .type = hd::QueueType::eGraphics,
                    });

            presentQueue = hd::Queue_t::conjure({
                    .device = device,
                    .type = hd::QueueType::ePresent,
                    });

            graphicsPool = hd::CommandPool_t::conjure({
                    .device = device,
                    .family = hd::PoolFamily::eGraphics,
                    });

            imageAvailable.resize(MAX_FRAMES_IN_FLIGHT);
            renderFinished.resize(MAX_FRAMES_IN_FLIGHT);
            inFlightFences.resize(MAX_FRAMES_IN_FLIGHT);

            for (uint32_t iter = 0; iter < MAX_FRAMES_IN_FLIGHT; iter++) {
                imageAvailable[iter] = hd::Semaphore_t::conjure({.device = device});
                renderFinished[iter] = hd::Semaphore_t::conjure({.device = device});
                inFlightFences[iter] = hd::Fence_t::conjure({.device = device});
            }
        }

        hd::SwapChain swapChain;
        std::vector<hd::Fence> inFlightImages;
        hd::RenderPass renderPass;
        std::vector<hd::Framebuffer> framebuffers;
        hd::PipelineLayout pipelineLayout;
        hd::Pipeline pipeline;
        std::vector<hd::CommandBuffer> commandBuffers;

        void setup() {
            swapChain = hd::SwapChain_t::conjure(hd::SwapChainCreateInfo{
                    .window = window,
                    .surface = surface,
                    .device = device,
                    .allocator = allocator,
                    .presentMode = vk::PresentModeKHR::eMailbox,
                    });

            inFlightImages.resize(swapChain->length(), nullptr);

            renderPass = hd::SwapChainRenderPass_t::conjure({
                        .device = device,
                        .swapChain = swapChain,
                        });

            framebuffers.reserve(swapChain->length());
            for (uint32_t index = 0; index < swapChain->length(); index++)
                framebuffers.push_back(hd::Framebuffer_t::conjure({
                            .renderPass = renderPass,
                            .device = device,
                            .attachments = {
                                swapChain->colorAttachment(index)->view(),
                                swapChain->depthAttachment(index)->view(),
                            },
                            .extent = swapChain->extent(),
                            }));

            hd::Shader triangleVertex = hd::Shader_t::conjure({
                    .device = device,
                    .filename = "shaders/triangle.vert.spv",
                    .stage = vk::ShaderStageFlagBits::eVertex,
                    });

            hd::Shader triangleFragment = hd::Shader_t::conjure({
                    .device = device,
                    .filename = "shaders/triangle.frag.spv",
                    .stage = vk::ShaderStageFlagBits::eFragment,
                    });

            pipelineLayout = hd::PipelineLayout_t::conjure({
                    .device = device,
                    });

            pipeline = hd::DefaultPipeline_t::conjure({
                    .pipelineLayout = pipelineLayout,
                    .renderPass = renderPass,
                    .device = device,
                    .shaderInfo = { triangleVertex->info(), triangleFragment->info() },
                    .extent = swapChain->extent(),
                    .checkDepth = false,
                    });

            commandBuffers = graphicsPool->allocate(framebuffers.size());

            // Record
            for (uint32_t iter = 0; iter < framebuffers.size(); iter++) {
                commandBuffers[iter]->begin();
                commandBuffers[iter]->beginRenderPass({
                        .renderPass = renderPass,
                        .framebuffer = framebuffers[iter],
                        .extent = swapChain->extent(),
                        });

                commandBuffers[iter]->raw().bindPipeline(vk::PipelineBindPoint::eGraphics, pipeline->raw());
                commandBuffers[iter]->raw().draw(3, 1, 0, 0);

                commandBuffers[iter]->endRenderPass(commandBuffers[iter]);
                commandBuffers[iter]->end();
            }
            // End
        }

        void cleanupRender() {
            device->waitIdle();

            commandBuffers.clear();
            pipeline.reset();
            pipelineLayout.reset();
            framebuffers.clear();
            renderPass.reset();
            inFlightImages.clear();
            swapChain.reset();

            device->updateSurfaceInfo();
        }

        void loop() {
            while (!window->shouldClose()) {
                window->pollEvents();
                update();
            }

            device->waitIdle();
        }

        uint32_t currentFrame = 0;

        void update() {
            inFlightFences[currentFrame]->wait();

            uint32_t imageIndex;
            {
                auto result = device->acquireNextImage(swapChain->raw(), imageAvailable[currentFrame]->raw());

                if (result.result == vk::Result::eErrorOutOfDateKHR) {
                    cleanupRender();
                    setup();
                } else if (result.result != vk::Result::eSuccess && result.result != vk::Result::eSuboptimalKHR)
                    throw std::runtime_error("Failed to acquire the next image");
                imageIndex = result.value;
            }

            if (inFlightImages[imageIndex] != nullptr)
                inFlightImages[imageIndex]->wait();
            inFlightImages[imageIndex] = inFlightFences[currentFrame];

            {
                vk::Semaphore waitSemaphores[] = { imageAvailable[currentFrame]->raw() };
                vk::PipelineStageFlags waitStages[] = { vk::PipelineStageFlagBits::eColorAttachmentOutput };
                vk::Semaphore signalSemaphores[] = { renderFinished[currentFrame]->raw() };

                vk::SubmitInfo submitInfo = {};
                submitInfo.waitSemaphoreCount = 1;
                submitInfo.pWaitSemaphores = waitSemaphores;
                submitInfo.pWaitDstStageMask = waitStages;
                submitInfo.commandBufferCount = 1;
                submitInfo.pCommandBuffers = &commandBuffers[imageIndex]->raw();
                submitInfo.signalSemaphoreCount = 1;
                submitInfo.pSignalSemaphores = signalSemaphores;

                inFlightFences[currentFrame]->reset();
                graphicsQueue->submit(submitInfo, inFlightFences[currentFrame]);
            }

            {
                vk::SwapchainKHR swapChains[] = { swapChain->raw() };

                vk::Semaphore waitSemaphores[] = { renderFinished[currentFrame]->raw() };

                vk::PresentInfoKHR presentInfo{};
                presentInfo.waitSemaphoreCount = 1;
                presentInfo.pWaitSemaphores = waitSemaphores;
                presentInfo.swapchainCount = 1;
                presentInfo.pSwapchains = swapChains;
                presentInfo.pImageIndices = &imageIndex;
                presentInfo.pResults = nullptr;

                vk::Result result = presentQueue->present(presentInfo);

                if (result == vk::Result::eErrorOutOfDateKHR || result == vk::Result::eSuboptimalKHR || framebufferResized) {
                    framebufferResized = false;
                    cleanupRender();
                    setup();
                } else if (result != vk::Result::eSuccess)
                    throw std::runtime_error("Failed to present the image to the swapChain");
            }

            currentFrame = (currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
        }

        static void framebufferResizeCallback(GLFWwindow* window, int width, int height) {
            auto app = reinterpret_cast<App*>(glfwGetWindowUserPointer(window));
            app->framebufferResized = true;
        }


        static hd::QueueFamilyIndices customFindQueueFamilies(vk::PhysicalDevice physicalDevice, hd::Surface surface) {
            hd::QueueFamilyIndices indices;

            std::vector<vk::QueueFamilyProperties> queueFamilies = physicalDevice.getQueueFamilyProperties();

            int i = -1;
            for (const auto& queueFamily : queueFamilies) {
                i++;
                if (!indices.graphicsFamily.has_value())
                    if (queueFamily.queueFlags & vk::QueueFlagBits::eGraphics) {
                        indices.graphicsFamily = i;
                        indices.graphicsCount = queueFamily.queueCount;
                    }

                vk::Bool32 presentSupport;
                if (surface != NULL)
                    presentSupport = physicalDevice.getSurfaceSupportKHR(i, surface->raw());

                if (!indices.presentFamily.has_value())
                    if (presentSupport) {
                        indices.presentFamily = i;
                        indices.presentCount = queueFamily.queueCount;
                        continue;
                    }

                if (!indices.transferFamily.has_value())
                    if (queueFamily.queueFlags & vk::QueueFlagBits::eTransfer) {
                        indices.transferFamily = i;
                        indices.transferCount = queueFamily.queueCount;
                        continue;
                    }

                if (!indices.computeFamily.has_value())
                    if (queueFamily.queueFlags & vk::QueueFlagBits::eCompute) {
                        indices.computeFamily = i;
                        indices.computeCount = queueFamily.queueCount;
                        continue;
                    }

                if (indices.isComplete()) {
                    break;
                }
            }

            return indices;
        }

    public:
        void run() {
            init();
            setup();
            loop();
        }
};
