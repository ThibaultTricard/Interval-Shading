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

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

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
    GLFWwindow* window = glfwCreateWindow(1920, 1080, "Interval Shading - Crystal", nullptr, nullptr);
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
        Tetmesh mesh = load_msh(root + "objects/crystal.vtk");
        std::cout << mesh.m_indices.size() << std::endl;

        // Trackball setup
        Trackball trackball(
            glm::vec3(0.0f, -0.8f, 0.0f),
            glm::vec3(0.0f,  0.0f, 0.0f),
            glm::vec3(0.0f,  0.0f, -1.0f)
        );
        trackball.m_moveSpeed    = 0.001f;
        trackball.m_rotationSpeed = 1.0f;

        auto view = trackball.getview();

        float nearPlane = 0.1f;
        vk::Extent2D swapExtent = device.getSwapchainExtent();
        // Must use LH perspective to match lookAtLH: ensures NDC Z is in [0,1]
        // for visible objects (positive view-space Z), so the depth stored in the
        // alpha channel via MIN blend satisfies d.w != 1.0 in the mode-3 depth check.
        glm::mat4 perspective = glm::perspectiveLH(glm::radians(80.0f),
            float(swapExtent.width) / float(swapExtent.height), nearPlane, 1000.0f);

        int step = 0;

        // Mesh uniform buffer
        LavaCake::UniformBuffer viewproj(device);
        glm::mat4 model = glm::identity<glm::mat4>();
        viewproj.addVariable("model",     model);
        viewproj.addVariable("view",      view);
        viewproj.addVariable("perpective", perspective);
        viewproj.addVariable("nearPlane", nearPlane);
        viewproj.addVariable("mode",      step);
        viewproj.end();

        // Mesh storage buffers
        LavaCake::Buffer vertexBuffer(device, mesh.m_vertices, vk::BufferUsageFlagBits::eStorageBuffer);
        LavaCake::Buffer indicesBuffer(device, mesh.m_indices,  vk::BufferUsageFlagBits::eStorageBuffer);

        // Offscreen image
        constexpr uint32_t resWidth  = 3840;
        constexpr uint32_t resHeight = 2160;
        vk::Format offscreenFormat = vk::Format::eR32G32B32A32Sfloat;

        LavaCake::Image     offscreenImage(device, resWidth, resHeight, 1, offscreenFormat,
                                           vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eSampled);
        LavaCake::ImageView offscreenView(offscreenImage);
        LavaCake::Sampler   offscreenSampler(device);

        // Transition offscreen image to ShaderReadOnly initially
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

        // Load envmap PNG with stb_image
        int envWidth, envHeight, envChannels;
        stbi_uc* pixels = stbi_load((root + "envmaps/dancing_hall_4k.png").c_str(),
                                     &envWidth, &envHeight, &envChannels, STBI_rgb_alpha);
        std::vector<uint8_t> envData(pixels, pixels + envWidth * envHeight * 4);
        stbi_image_free(pixels);

        // Envmap image (constructor with data uploads to GPU and transitions to ShaderReadOnly)
        LavaCake::Image     envmapImage(device, envData,
                                        uint32_t(envWidth), uint32_t(envHeight), 1,
                                        vk::Format::eR8G8B8A8Unorm,
                                        vk::ImageUsageFlagBits::eSampled);
        LavaCake::ImageView envmapView(envmapImage);
        LavaCake::Sampler   envmapSampler(device);

        // Mesh descriptor layout / pool / set
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

        // Mesh blend: ONE/ONE/ADD color + ONE/ONE/MIN alpha
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
            .addShaderFromFile(root + "src/shader/crystal/mesh.mesh",
                               vk::ShaderStageFlagBits::eMeshEXT,
                               LavaCake::ShadingLanguage::eGLSL)
            .addShaderFromFile(root + "src/shader/crystal/interval.frag",
                               vk::ShaderStageFlagBits::eFragment,
                               LavaCake::ShadingLanguage::eGLSL)
            .addColorAttachmentFormat(offscreenFormat)
            .setColorBlendAttachments({meshBlend})
            .setCullMode(vk::CullModeFlagBits::eNone)
            .setDepthTest(false, false)
            .addDescriptorSetLayout(meshDescLayout)
            .build();

        // Post-process quad buffers
        std::vector<float>    quadVerts   = { -1.0f,-1.0f,0.0f,0.0f,0.0f,
                                              -1.0f, 1.0f,0.0f,0.0f,1.0f,
                                               1.0f, 1.0f,0.0f,1.0f,1.0f,
                                               1.0f,-1.0f,0.0f,1.0f,0.0f };
        std::vector<uint32_t> quadIndices = { 0,1,2, 2,3,0 };

        LavaCake::Buffer quadVertexBuffer(device, quadVerts,   vk::BufferUsageFlagBits::eVertexBuffer);
        LavaCake::Buffer quadIndexBuffer (device, quadIndices, vk::BufferUsageFlagBits::eIndexBuffer);

        // Post-process uniform buffer
        glm::vec4  ppColor      = glm::vec4(255.0f/255.0f, 174.0f/255.0f, 26.0f/255.0f, 17.8f);
        glm::vec4  ppReflection = glm::vec4(1.0f, 1.0f, 1.0f, 0.3f);
        int        ppState      = step;
        glm::uvec2 ppRes        = glm::uvec2(swapExtent.width, swapExtent.height);
        glm::uvec2 ppResFirst   = glm::uvec2(resWidth, resHeight);

        LavaCake::UniformBuffer ub(device);
        ub.addVariable("color",        ppColor);
        ub.addVariable("proj",         perspective);
        ub.addVariable("camera",       view);
        ub.addVariable("camPos",       trackball.m_pos);
        ub.addVariable("padding",      1.0f);
        ub.addVariable("res",          ppRes);
        ub.addVariable("resFirstPass", ppResFirst);
        ub.addVariable("reflection",   ppReflection);
        ub.addVariable("State",        ppState);
        ub.end();

        // Post-process descriptor layout / pool / set
        // binding 0: offscreen sampler, binding 1: UB, binding 2: envmap sampler
        LavaCake::DescriptorSetLayout postDescLayout = LavaCake::DescriptorSetLayout::Builder(device)
            .addCombinedImageSampler(0, vk::ShaderStageFlagBits::eFragment)
            .addUniformBuffer(1, vk::ShaderStageFlagBits::eFragment)
            .addCombinedImageSampler(2, vk::ShaderStageFlagBits::eFragment)
            .build();

        LavaCake::DescriptorPool postDescPool = LavaCake::DescriptorPool::Builder(device.getDevice())
            .addCombinedImageSamplers(2)
            .addUniformBuffers(1)
            .setMaxSets(1)
            .build();

        vk::DescriptorSet postDescSet = postDescPool.allocate(postDescLayout);
        LavaCake::DescriptorSetUpdater(device, postDescSet)
            .bindImage(0, offscreenView, offscreenSampler)
            .bindUniformBuffer(1, ub, 0, ub.size())
            .bindImage(2, envmapView, envmapSampler)
            .update();

        // Vertex input for fullscreen quad: position (xyz) + uv
        LavaCake::GraphicsPipeline::VertexInputInfo quadVertexInput;
        quadVertexInput.bindings   = {{ 0, 5 * sizeof(float), vk::VertexInputRate::eVertex }};
        quadVertexInput.attributes = {
            { 0, 0, vk::Format::eR32G32B32Sfloat, 0 },
            { 1, 0, vk::Format::eR32G32Sfloat,    3 * sizeof(float) }
        };

        // Post-process pipeline
        LavaCake::GraphicsPipeline postProcessPipeline = LavaCake::GraphicsPipeline::Builder(device)
            .addShaderFromFile(root + "src/shader/crystal/postProcess.vert",
                               vk::ShaderStageFlagBits::eVertex,
                               LavaCake::ShadingLanguage::eGLSL)
            .addShaderFromFile(root + "src/shader/crystal/postProcess.frag",
                               vk::ShaderStageFlagBits::eFragment,
                               LavaCake::ShadingLanguage::eGLSL)
            .addColorAttachmentFormat(device.getSwapchainFormat())
            .setVertexInput(quadVertexInput)
            .setDepthTest(false, false)
            .addDescriptorSetLayout(postDescLayout)
            .build();

        // ImGui
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
            ImGui::SliderInt("Mode",          &step, 0, 3);
            ImGui::ColorEdit3("color",         &ppColor.x);
            ImGui::SliderFloat("Opacity",      &ppColor.w, 0.0f, 100.0f);
            ImGui::ColorEdit3("Reflection color", &ppReflection.x);
            ImGui::SliderFloat("reflectivity", &ppReflection.w, 0.0f, 1.0f);
            ImGui::End();

            // Camera input (skip when ImGui captures mouse)
            ImGuiIO& io = ImGui::GetIO();
            if (!io.WantCaptureMouse) {
                trackball.move(get_movement(window));

                auto dmov = get_mouse_mouvement(window);
                if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS) {
                    float dx = float(dmov.x) / float(swapExtent.width)  * 2.0f * 3.14159265f;
                    float dy = float(dmov.y) / float(swapExtent.width) * 2.0f * 3.14159265f;
                    trackball.rotate(glm::vec2(dx, dy));
                }

                if (scrolled) {
                    scrolled = false;
                    trackball.zoom(float(scroll));
                }
            }

            view = trackball.getview();
            viewproj.setVariable("view", view);
            viewproj.setVariable("mode", step);

            ub.setVariable("camera",     view);
            ub.setVariable("camPos",     trackball.m_pos);
            ub.setVariable("color",      ppColor);
            ub.setVariable("reflection", ppReflection);
            ub.setVariable("State",      step);

            // Acquire swapchain image
            LavaCake::SwapChainImage& swapchainImage =
                device.aquireSwapChainImage(imageAvailableSemaphores[currentFrame]);

            auto frameStart = std::chrono::high_resolution_clock::now();

            // Begin recording
            cmdBuffer.begin(vk::CommandBufferUsageFlagBits::eOneTimeSubmit);
            vk::CommandBuffer cmd = cmdBuffer.getCommandBuffer();

            viewproj.update(cmd);
            ub.update(cmd);

            // Offscreen mesh pass
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

            // Post-process + ImGui pass
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
