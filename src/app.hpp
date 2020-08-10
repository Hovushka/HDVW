#pragma once

#define GLM_FORCE_DEPTH_ZERO_TO_ONE

#include <vulkan/vulkan.hpp>
#include <glm/glm.hpp>

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
#include <hdvw/vertex.hpp>
#include <hdvw/databuffer.hpp>
#include <hdvw/texture.hpp>
#include <hdvw/descriptorlayout.hpp>
#include <hdvw/descriptorpool.hpp>
#include <hdvw/descriptorset.hpp>

#define MAX_FRAMES_IN_FLIGHT 3

struct MVP {
    glm::mat4 model;
    glm::mat4 view;
    glm::mat4 proj;
};

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

        hd::DataBuffer<hd::Vertex> vertexBuffer;
        hd::DataBuffer<uint32_t> indexBuffer;

        hd::Texture texture;
        hd::DataBuffer<MVP> unibuffer;
        hd::DescriptorLayout descriptorLayout;

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

            const std::vector<hd::Vertex> vertices = {
                {{-0.5f, -0.5f,  0.0f}, {1.0f, 0.0f, 0.0f}, {0.0f, 0.0f}},
                {{ 0.5f, -0.5f,  0.0f}, {0.0f, 1.0f, 0.0f}, {1.0f, 0.0f}},
                {{ 0.5f,  0.5f,  0.0f}, {0.0f, 0.0f, 1.0f}, {1.0f, 1.0f}},
                {{-0.5f,  0.5f,  0.0f}, {1.0f, 1.0f, 1.0f}, {0.0f, 1.0f}},
            };

            vertexBuffer = hd::DataBuffer_t<hd::Vertex>::conjure({
                    .commandPool = graphicsPool,
                    .queue = graphicsQueue,
                    .allocator = allocator,
                    .data = vertices,
                    .usage = vk::BufferUsageFlagBits::eVertexBuffer,
                    });

            const std::vector<uint32_t> indices = {
                0, 1, 2, 2, 3, 0
            };

            indexBuffer = hd::DataBuffer_t<uint32_t>::conjure({
                    .commandPool = graphicsPool,
                    .queue = graphicsQueue,
                    .allocator = allocator,
                    .data = indices,
                    .usage = vk::BufferUsageFlagBits::eIndexBuffer,
                    });
            
            texture = hd::Texture_t::conjure({
                    .filename = "lizard.jpg",
                    .commandPool = graphicsPool,
                    .queue = graphicsQueue,
                    .allocator = allocator,
                    .device = device,
                    });

            vk::DescriptorSetLayoutBinding textureBinding{
                0, // binding
                vk::DescriptorType::eCombinedImageSampler, // descriptorType
                1, // descriptorCount
                vk::ShaderStageFlagBits::eFragment, // stageFlags
                nullptr, // pImmutableSamplers
            };

            MVP transform {
                glm::mat4(1.0f),
                glm::mat4(1.0f),
                glm::mat4(1.0f),
            };

            unibuffer = hd::DataBuffer_t<MVP>::conjure({
                    .commandPool = graphicsPool,
                    .queue = graphicsQueue,
                    .allocator = allocator,
                    .data = {transform},
                    .usage = vk::BufferUsageFlagBits::eUniformBuffer,
                    .memoryUsage = VMA_MEMORY_USAGE_CPU_TO_GPU,
                    });

            vk::DescriptorSetLayoutBinding uniformBinding{
                1,
                vk::DescriptorType::eUniformBuffer,
                1,
                vk::ShaderStageFlagBits::eVertex,
                nullptr,
            };

            descriptorLayout = hd::DescriptorLayout_t::conjure({
                    .device = device,
                    .bindings = {textureBinding, uniformBinding},
                    });
        }

        hd::SwapChain swapChain;
        hd::DescriptorPool descriptorPool;
        std::vector<hd::DescriptorSet> descriptorSets;
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
                    .allocator = allocator,
                    .device = device,
                    .presentMode = vk::PresentModeKHR::eMailbox,
                    });

            descriptorPool = hd::DescriptorPool_t::conjure({
                    .device = device,
                    .layouts = {{descriptorLayout, 1}},
                    .instances = swapChain->length(),
                    });

            descriptorSets = descriptorPool->allocate(1, descriptorLayout);

            for (auto& set: descriptorSets) {
                vk::DescriptorImageInfo ii = {};
                ii.imageLayout = vk::ImageLayout::eShaderReadOnlyOptimal;
                ii.imageView = texture->view();
                ii.sampler = texture->sampler();

                vk::WriteDescriptorSet ws = {};
                ws.dstBinding = 0;
                ws.dstArrayElement = 0;
                ws.descriptorType = vk::DescriptorType::eCombinedImageSampler;
                ws.descriptorCount = 1;

                set->update({ .writeSet = ws, .imageInfo = ii, });

                vk::DescriptorBufferInfo bi = {};
                bi.buffer = unibuffer->raw();
                bi.offset = 0;
                bi.range = sizeof(MVP);

                vk::WriteDescriptorSet ws2 = {};
                ws2.dstBinding = 1;
                ws2.dstArrayElement = 0;
                ws2.descriptorType = vk::DescriptorType::eUniformBuffer;
                ws2.descriptorCount = 1;

                set->update({ .writeSet = ws2, .bufferInfo = bi, });
            }

            float aspect = (float) swapChain->extent().height / (float) swapChain->extent().width;
            
            MVP orthoProj {
                glm::mat4(1.0f),
                glm::lookAt(glm::vec3(0.0f, 0.0f, 3.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f)),
                glm::ortho(-1.0f, 1.0f, -1.0f * aspect, 1.0f * aspect, 0.1f, 100.0f),
            };

            void* data;
            allocator->map(unibuffer->memory(), data);
            memcpy(data, &orthoProj, sizeof(MVP));
            allocator->unmap(unibuffer->memory());

            inFlightImages.resize(swapChain->length(), nullptr);

            renderPass = hd::SwapChainRenderPass_t::conjure({
                        .swapChain = swapChain,
                        .device = device,
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
                    .descriptorLayouts = {descriptorLayout->raw()},
                    });

            pipeline = hd::DefaultPipeline_t::conjure({
                    .pipelineLayout = pipelineLayout,
                    .renderPass = renderPass,
                    .device = device,
                    .shaderInfo = { triangleVertex->info(), triangleFragment->info() },
                    .extent = swapChain->extent(),
                    .cullMode = vk::CullModeFlagBits::eBack,
                    .frontFace = vk::FrontFace::eClockwise,
                    .checkDepth = true,
                    });

            commandBuffers = graphicsPool->allocate(framebuffers.size());

            // Record
            std::vector<vk::DeviceSize> offsets = { 0 };
            for (uint32_t iter = 0; iter < framebuffers.size(); iter++) {
                commandBuffers[iter]->begin();
                commandBuffers[iter]->beginRenderPass({
                        .renderPass = renderPass,
                        .framebuffer = framebuffers[iter],
                        .extent = swapChain->extent(),
                        });

                commandBuffers[iter]->raw().bindDescriptorSets(vk::PipelineBindPoint::eGraphics, pipelineLayout->raw(), 0, descriptorSets[iter]->raw(), nullptr);
                commandBuffers[iter]->raw().bindVertexBuffers(0, vertexBuffer->raw(), offsets);
                commandBuffers[iter]->raw().bindIndexBuffer(indexBuffer->raw(), 0, vk::IndexType::eUint32);
                commandBuffers[iter]->raw().bindPipeline(vk::PipelineBindPoint::eGraphics, pipeline->raw());
                commandBuffers[iter]->raw().drawIndexed(indexBuffer->count(), 1, 0, 0, 0);

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
            descriptorSets.clear();
            descriptorPool.reset();
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

                auto raw = commandBuffers[imageIndex]->raw();

                vk::SubmitInfo submitInfo = {};
                submitInfo.waitSemaphoreCount = 1;
                submitInfo.pWaitSemaphores = waitSemaphores;
                submitInfo.pWaitDstStageMask = waitStages;
                submitInfo.commandBufferCount = 1;
                submitInfo.pCommandBuffers = &raw;
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
