#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <LavaCake/GLFWSupport.hpp>
#include <LavaCake/CommandBuffer.hpp>
#include <LavaCake/DynamicRendering.hpp>
#include <LavaCake/GraphicPipeline.hpp>
#include <LavaCake/Buffer.hpp>
#include <LavaCake/UniformBuffer.hpp>
#include <LavaCake/DescriptorSet.hpp>
#include <LavaCake/DescriptorPool.hpp>
#include <LavaCake/Image.hpp>
#include <LavaCake/ImGui.hpp>

#include <imgui_impl_glfw.h>

#include "utils/trackball.h"
#include "utils/Tetmesh.h"

#include <chrono>
#include <iostream>


glm::dvec2 mousepos = glm::dvec2(0.0, 0.0);

glm::dvec2 get_mouse_mouvement(GLFWwindow* window) {
    glm::dvec2 new_mousepos;
    glfwGetCursorPos(window, &new_mousepos.x, &new_mousepos.y);
    auto delta = new_mousepos - mousepos;
    mousepos = new_mousepos;
    return delta;
}

double scroll = 0;
bool scrolled = false;
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset) {
    (void)window; (void)xoffset;
    if (yoffset != 0) {
        scroll = -yoffset * 10.0;
        scrolled = true;
    }
}

glm::vec3 get_movement(GLFWwindow* window) {
    glm::vec3 translation(0.0f);
    if (glfwGetKey(window, GLFW_KEY_UP)            == GLFW_PRESS) translation += glm::vec3( 0.0f,  0.0f, -1.0f);
    if (glfwGetKey(window, GLFW_KEY_DOWN)          == GLFW_PRESS) translation += glm::vec3( 0.0f,  0.0f,  1.0f);
    if (glfwGetKey(window, GLFW_KEY_LEFT)          == GLFW_PRESS) translation += glm::vec3(-1.0f,  0.0f,  0.0f);
    if (glfwGetKey(window, GLFW_KEY_RIGHT)         == GLFW_PRESS) translation += glm::vec3( 1.0f,  0.0f,  0.0f);
    if (glfwGetKey(window, GLFW_KEY_SPACE)         == GLFW_PRESS) translation += glm::vec3( 0.0f, -1.0f,  0.0f);
    if (glfwGetKey(window, GLFW_KEY_RIGHT_CONTROL) == GLFW_PRESS) translation += glm::vec3( 0.0f,  1.0f,  0.0f);
    return translation;
}

std::string root = PROJECT_ROOT;

int main() {
    glfwInit();
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
    GLFWwindow* window = glfwCreateWindow(1920, 1080, "Interval Shading - Bunny", nullptr, nullptr);
    glfwSetScrollCallback(window, scroll_callback);

    auto surfaceConfig = LavaCake::GLFW::createSurfaceConfig(window);

    vk::PhysicalDeviceVulkan12Features vulkan12{};
    vulkan12.scalarBlockLayout                                    = VK_TRUE;
    vulkan12.descriptorIndexing                                   = VK_TRUE;
    vulkan12.shaderSampledImageArrayNonUniformIndexing            = VK_TRUE;
    vulkan12.shaderStorageBufferArrayNonUniformIndexing           = VK_TRUE;
    vulkan12.shaderUniformBufferArrayNonUniformIndexing           = VK_TRUE;
    vulkan12.runtimeDescriptorArray                               = VK_TRUE;
    vulkan12.descriptorBindingPartiallyBound                      = VK_TRUE;
    vulkan12.descriptorBindingSampledImageUpdateAfterBind         = VK_TRUE;
    vulkan12.descriptorBindingStorageBufferUpdateAfterBind        = VK_TRUE;
    vulkan12.descriptorBindingUniformBufferUpdateAfterBind        = VK_TRUE;
    vulkan12.descriptorBindingVariableDescriptorCount             = VK_TRUE;
    vulkan12.bufferDeviceAddress                                  = VK_TRUE;
    vulkan12.timelineSemaphore                                    = VK_TRUE;
    vulkan12.hostQueryReset                                       = VK_TRUE;

    vk::PhysicalDeviceMaintenance4Features maintenance4Features{};
    maintenance4Features.maintenance4 = VK_TRUE;

    vk::PhysicalDeviceMeshShaderFeaturesEXT meshShaderFeatures{};
    meshShaderFeatures.meshShader = VK_TRUE;
    meshShaderFeatures.taskShader = VK_TRUE;

    LavaCake::Device device = LavaCake::Device::Builder()
        .setGraphicQueueCount(1)
        .enableSamplerAnisotropy(true)
        .enableFillModeNonSolid(true)
        .addFeature(vulkan12)
        .addFeature(maintenance4Features)
        .addFeature(meshShaderFeatures)
        .addDeviceExtension(VK_EXT_MESH_SHADER_EXTENSION_NAME)
        .preferDiscreteGPU()
        .setSurface(surfaceConfig)
        .build();
    {
        LavaCake::CommandBuffer cmdBuffer(device, true);

        // Load mesh
        Tetmesh mesh = load_msh(root + "objects/bunny.vtk");
        std::cout << mesh.m_indices.size() << std::endl;

        // Trackball setup
        float freq = 10.0f;
        Trackball trackball(
            glm::vec3(0.0f, -1.1f, 0.0f),
            glm::vec3(0.0f,  0.0f, 0.0f),
            glm::vec3(0.0f,  0.0f, -1.0f)
        );
        trackball.m_moveSpeed = 0.02f;
        trackball.m_rotationSpeed = 3.14159265f * 2.0f / freq;
        trackball.rotate(glm::vec2(freq / 2.0f, 0.0f));

        auto view = trackball.getview();

        // Uniform buffer
        LavaCake::UniformBuffer viewproj(device);
        glm::mat4 model = glm::rotate(glm::mat4(1.0f), glm::radians(90.0f),  glm::vec3(1.0f, 0.0f, 0.0f))
                        * glm::rotate(glm::mat4(1.0f), glm::radians(180.0f), glm::vec3(0.0f, 0.0f, 1.0f));
        viewproj.addVariable("model", model);
        viewproj.addVariable("view", view);

        float nearPlane = 0.1f;
        glm::mat4 perspective = glm::perspectiveLH(glm::radians(80.0f), float(1920) / float(1080), nearPlane, 1000.0f);
        perspective[1][1] *= -1.0f;
        viewproj.addVariable("perpective", perspective);
        viewproj.addVariable("nearPlane", nearPlane);
        viewproj.addVariable("tetnumber", int(mesh.m_indices.size()));
        viewproj.end();

        // Offscreen resolution
        constexpr uint32_t resWidth  = 3840;
        constexpr uint32_t resHeight = 2160;
        vk::Format offscreenFormat = vk::Format::eR32G32B32A32Sfloat;

        // Storage buffers
        LavaCake::Buffer vertexBuffer(device, mesh.m_vertices, vk::BufferUsageFlagBits::eStorageBuffer);
        LavaCake::Buffer indicesBuffer(device, mesh.m_indices,  vk::BufferUsageFlagBits::eStorageBuffer);

        // Offscreen image + view + sampler
        LavaCake::Image     offscreenImage(device, resWidth, resHeight, 1, offscreenFormat,
                                           vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eSampled);
        LavaCake::ImageView offscreenView(offscreenImage);
        LavaCake::Sampler   offscreenSampler(device);

        // Initial layout transition
        {
            LavaCake::CommandBuffer initCmd(device, false);
            initCmd.begin();
            vk::CommandBuffer initRaw = initCmd.getCommandBuffer();
            offscreenImage.transitionLayout(initRaw,
                vk::ImageLayout::eUndefined,
                vk::ImageLayout::eShaderReadOnlyOptimal);
            initCmd.end();

            vk::SubmitInfo submitInfo{};
            submitInfo.commandBufferCount = 1;
            vk::CommandBuffer rawInit = initCmd.getCommandBuffer();
            submitInfo.pCommandBuffers = &rawInit;
            device.getGraphicQueue(0).submit(submitInfo);
            device.getGraphicQueue(0).waitIdle();
        }

        // ---- Mesh pass descriptor ----
        LavaCake::DescriptorSetLayout meshDescLayout = LavaCake::DescriptorSetLayout::Builder(device)
            .addUniformBuffer(0, vk::ShaderStageFlagBits::eMeshEXT | vk::ShaderStageFlagBits::eFragment)
            .addStorageBuffer(1, vk::ShaderStageFlagBits::eMeshEXT)
            .addStorageBuffer(2, vk::ShaderStageFlagBits::eMeshEXT)
            .build();

        LavaCake::DescriptorPool meshDescPool = LavaCake::DescriptorPool::Builder(device.getDevice())
            .addUniformBuffers(1)
            .addStorageBuffers(2)
            .setMaxSets(1)
            .build();

        vk::DescriptorSet meshDescSet = meshDescPool.allocate(meshDescLayout);

        LavaCake::DescriptorSetUpdater(device, meshDescSet)
            .bindUniformBuffer(0, viewproj, 0, viewproj.size())
            .bindStorageBuffer(1, vertexBuffer)
            .bindStorageBuffer(2, indicesBuffer)
            .update();

        // Additive blend + min alpha
        vk::PipelineColorBlendAttachmentState meshBlend{};
        meshBlend.blendEnable         = VK_TRUE;
        meshBlend.srcColorBlendFactor = vk::BlendFactor::eOne;
        meshBlend.dstColorBlendFactor = vk::BlendFactor::eOne;
        meshBlend.colorBlendOp        = vk::BlendOp::eAdd;
        meshBlend.srcAlphaBlendFactor = vk::BlendFactor::eOne;
        meshBlend.dstAlphaBlendFactor = vk::BlendFactor::eOne;
        meshBlend.alphaBlendOp        = vk::BlendOp::eMin;
        meshBlend.colorWriteMask      = vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG
                                      | vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA;

        // Mesh pipeline
        LavaCake::GraphicsPipeline graphicPipeline = LavaCake::GraphicsPipeline::Builder(device)
            .addShaderFromFile(root + "src/shader/bunny/mesh.mesh",
                               vk::ShaderStageFlagBits::eMeshEXT,
                               LavaCake::ShadingLanguage::eGLSL)
            .addShaderFromFile(root + "src/shader/bunny/interval.frag",
                               vk::ShaderStageFlagBits::eFragment,
                               LavaCake::ShadingLanguage::eGLSL)
            .addColorAttachmentFormat(offscreenFormat)
            .setColorBlendAttachments({meshBlend})
            .setCullMode(vk::CullModeFlagBits::eNone)
            .setDepthTest(false, false)
            .addDescriptorSetLayout(meshDescLayout)
            .build();

        // ---- Post-process quad ----
        std::vector<float>    quadVerts   = { -1.0f,-1.0f,0.0f,0.0f,0.0f,
                                              -1.0f, 1.0f,0.0f,0.0f,1.0f,
                                               1.0f, 1.0f,0.0f,1.0f,1.0f,
                                               1.0f,-1.0f,0.0f,1.0f,0.0f };
        std::vector<uint32_t> quadIndices = { 0,1,2, 2,3,0 };

        LavaCake::Buffer quadVertexBuffer(device, quadVerts,   vk::BufferUsageFlagBits::eVertexBuffer);
        LavaCake::Buffer quadIndexBuffer (device, quadIndices, vk::BufferUsageFlagBits::eIndexBuffer);

        // ---- Post-process descriptor ----
        LavaCake::DescriptorSetLayout postDescLayout = LavaCake::DescriptorSetLayout::Builder(device)
            .addCombinedImageSampler(0, vk::ShaderStageFlagBits::eFragment)
            .build();

        LavaCake::DescriptorPool postDescPool = LavaCake::DescriptorPool::Builder(device.getDevice())
            .addCombinedImageSamplers(1)
            .setMaxSets(1)
            .build();

        vk::DescriptorSet postDescSet = postDescPool.allocate(postDescLayout);

        LavaCake::DescriptorSetUpdater(device, postDescSet)
            .bindImage(0, offscreenView, offscreenSampler)
            .update();

        // Vertex input for post-process quad
        LavaCake::GraphicsPipeline::VertexInputInfo quadVertexInput;
        quadVertexInput.bindings   = {{ 0, 5 * sizeof(float), vk::VertexInputRate::eVertex }};
        quadVertexInput.attributes = {
            { 0, 0, vk::Format::eR32G32B32Sfloat, 0 },
            { 1, 0, vk::Format::eR32G32Sfloat,    3 * sizeof(float) }
        };

        // Post-process pipeline
        LavaCake::GraphicsPipeline postProcessPipeline = LavaCake::GraphicsPipeline::Builder(device)
            .addShaderFromFile(root + "src/shader/bunny/postProcess.vert",
                               vk::ShaderStageFlagBits::eVertex,
                               LavaCake::ShadingLanguage::eGLSL)
            .addShaderFromFile(root + "src/shader/bunny/postProcess.frag",
                               vk::ShaderStageFlagBits::eFragment,
                               LavaCake::ShadingLanguage::eGLSL)
            .addColorAttachmentFormat(device.getSwapchainFormat())
            .setVertexInput(quadVertexInput)
            .setDepthTest(false, false)
            .addDescriptorSetLayout(postDescLayout)
            .build();

        // ---- ImGui ----
        LavaCake::ImGuiRenderer imgui(device, device.getSwapchainFormat());
        ImGui::SetCurrentContext(imgui.getContext());
        ImGui_ImplGlfw_InitForVulkan(window, true);

        // Per-swapchain-image semaphores
        size_t swapchainImageCount = device.getSwapChainImagesNumber();
        vk::SemaphoreCreateInfo semaphoreInfo;
        std::vector<vk::Semaphore> imageAvailableSemaphores(swapchainImageCount);
        std::vector<vk::Semaphore> renderFinishedSemaphores(swapchainImageCount);
        for (size_t i = 0; i < swapchainImageCount; i++) {
            imageAvailableSemaphores[i] = device.getDevice().createSemaphore(semaphoreInfo);
            renderFinishedSemaphores[i] = device.getDevice().createSemaphore(semaphoreInfo);
        }

        uint32_t currentFrame = 0;
        float    cost         = 0.0f;

        // Main render loop
        while (!glfwWindowShouldClose(window)) {
            cmdBuffer.waitForCompletion();
            cmdBuffer.reset();
            glfwPollEvents();

            // ImGui new frame
            ImGui_ImplGlfw_NewFrame();
            imgui.newFrame();
            ImGui::Begin("Controls");
            ImGui::Text("cost: %.3f ms", cost);
            ImGui::End();

            // Camera input
            ImGuiIO& io = ImGui::GetIO();
            if (!io.WantCaptureMouse) {
                trackball.move(get_movement(window));

                auto dmov = get_mouse_mouvement(window);
                if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS) {
                    float dx = float(dmov.x) / float(1920) * 2.0f * 3.14159265f;
                    float dy = float(dmov.y) / float(1920) * 2.0f * 3.14159265f;
                    trackball.rotate(glm::vec2(dx, dy));
                }

                if (scrolled) {
                    scrolled = false;
                    trackball.zoom(float(scroll));
                }
            }

            view = trackball.getview();
            viewproj.setVariable("view", view);

            // Acquire swapchain image
            LavaCake::SwapChainImage& swapchainImage =
                device.aquireSwapChainImage(imageAvailableSemaphores[currentFrame]);

            auto frameStart = std::chrono::high_resolution_clock::now();

            // Begin recording
            cmdBuffer.begin(vk::CommandBufferUsageFlagBits::eOneTimeSubmit);
            vk::CommandBuffer cmd = cmdBuffer.getCommandBuffer();

            viewproj.update(cmd);

            // ---- Offscreen mesh pass ----
            offscreenImage.transitionLayout(cmd,
                vk::ImageLayout::eShaderReadOnlyOptimal,
                vk::ImageLayout::eColorAttachmentOptimal);
            {
                auto offscreenCtx = LavaCake::DynamicRenderingContext::Builder()
                    .setRenderArea(resWidth, resHeight)
                    .addColorAttachment(offscreenView,
                        vk::ClearColorValue(std::array<float,4>{0.0f, 0.0f, 0.0f, 1.0f}))
                    .begin(cmd);

                offscreenCtx.setDefaultViewportScissor(cmd);
                graphicPipeline.bind(cmd);
                graphicPipeline.bindDescriptorSets(cmd, {meshDescSet});
                cmd.drawMeshTasksEXT(uint32_t(mesh.m_indices.size()), 1, 1);
                offscreenCtx.end(cmd);
            }
            offscreenImage.transitionLayout(cmd,
                vk::ImageLayout::eColorAttachmentOptimal,
                vk::ImageLayout::eShaderReadOnlyOptimal);

            // ---- Post-process + ImGui pass ----
            swapchainImage.prepareForAttachementBarrier(cmd);
            {
                auto postCtx = LavaCake::DynamicRenderingContext::Builder()
                    .setRenderArea(device.getSwapchainExtent())
                    .addColorAttachment(swapchainImage,
                        vk::ClearColorValue(std::array<float,4>{0.0f, 0.0f, 0.0f, 1.0f}))
                    .begin(cmd);

                postCtx.setDefaultViewportScissor(cmd);

                postProcessPipeline.bind(cmd);
                postProcessPipeline.bindDescriptorSets(cmd, {postDescSet});
                vk::Buffer     vb     = quadVertexBuffer;
                vk::DeviceSize offset = 0;
                cmd.bindVertexBuffers(0, 1, &vb, &offset);
                cmd.bindIndexBuffer(quadIndexBuffer, 0, vk::IndexType::eUint32);
                postProcessPipeline.drawIndexed(cmd, 6);

                imgui.render(cmd);
                postCtx.end(cmd);
            }
            swapchainImage.prepareForPresentBarrier(cmd);
            cmdBuffer.end();

            // Submit
            vk::PipelineStageFlags waitStage = vk::PipelineStageFlagBits::eColorAttachmentOutput;
            vk::SubmitInfo submitInfo;
            submitInfo.waitSemaphoreCount   = 1;
            submitInfo.pWaitSemaphores      = &imageAvailableSemaphores[currentFrame];
            submitInfo.pWaitDstStageMask    = &waitStage;
            submitInfo.commandBufferCount   = 1;
            vk::CommandBuffer rawCmd        = cmdBuffer.getCommandBuffer();
            submitInfo.pCommandBuffers      = &rawCmd;
            submitInfo.signalSemaphoreCount = 1;
            submitInfo.pSignalSemaphores    = &renderFinishedSemaphores[currentFrame];

            device.getGraphicQueue(0).submit(submitInfo, cmdBuffer.getFence());
            cmdBuffer.markSubmitted();

            device.presentImage(swapchainImage, {renderFinishedSemaphores[currentFrame]});
            currentFrame = (currentFrame + 1) % swapchainImageCount;

            auto frameEnd = std::chrono::high_resolution_clock::now();
            cost = std::chrono::duration<float, std::milli>(frameEnd - frameStart).count();
        }

        device.waitForAllCommands();
        ImGui_ImplGlfw_Shutdown();

        for (size_t i = 0; i < swapchainImageCount; i++) {
            device.getDevice().destroySemaphore(imageAvailableSemaphores[i]);
            device.getDevice().destroySemaphore(renderFinishedSemaphores[i]);
        }
    }

    device.releaseDevice();
    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}
