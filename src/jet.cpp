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
    GLFWwindow* window = glfwCreateWindow(1920, 1080, "Interval Shading - Jet", nullptr, nullptr);
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

        // Load meshes (skip empty ones)
        std::vector<std::string> names = {
            root + "objects/jet/airbus_simplifiedBackCover.vtk",
            root + "objects/jet/airbus_simplifiedBackCover_001.vtk",
            root + "objects/jet/airbus_simplifiedBackCover_002.vtk",
            root + "objects/jet/airbus_simplifiedBackCover_003.vtk",
            root + "objects/jet/airbus_simplifiedBackCover_004.vtk",
            root + "objects/jet/airbus_simplifiedBackCover_005.vtk",
            root + "objects/jet/airbus_simplifiedBackCover_006.vtk",
            root + "objects/jet/airbus_simplifiedBackCover_007.vtk",
            root + "objects/jet/airbus_simplifiedBackCover_008.vtk",
            root + "objects/jet/airbus_simplifiedBackCover_009.vtk",
            root + "objects/jet/airbus_simplifiedBackCover_010.vtk",
            root + "objects/jet/airbus_simplifiedBackCover_011.vtk",
            root + "objects/jet/airbus_simplifiedBackCover_012.vtk",
            root + "objects/jet/airbus_simplifiedBackCover_013.vtk",
        };

        std::vector<Tetmesh> meshes;
        for (auto& s : names) {
            auto m = load_msh(s, false);
            if (m.m_indices.size() > 0) {
                meshes.push_back(std::move(m));
            }
        }

        int N = int(meshes.size());

        // Trackball setup
        Trackball trackball(
            glm::vec3(0.0f, -50.0f, 0.0f),
            glm::vec3(0.0f,   0.0f, 0.0f),
            glm::vec3(0.0f,   0.0f, -1.0f)
        );
        trackball.m_moveSpeed = 0.02f;
        float freq = 10.0f;
        trackball.m_rotationSpeed = 3.14159265f * 2.0f / freq;
        trackball.rotate(glm::vec2(1.75f, 0.0f));
        trackball.move(glm::vec3(0.0f, -10.0f, 0.0f));

        auto view = trackball.getview();

        float nearPlane = 0.1f;
        vk::Extent2D swapExtent = device.getSwapchainExtent();
        glm::mat4 perspective = glm::perspectiveLH(glm::radians(80.0f),
            float(swapExtent.width) / float(swapExtent.height), nearPlane, 1000.0f);

        // Random per-mesh positions (srand(42) for reproducibility)
        std::vector<glm::vec3> pos;
        std::srand(42);
        for (int i = 0; i < N; i++) {
            float x = 0.0f;
            float y = (float(std::rand()) / float(RAND_MAX)) * 2.0f - 1.0f;
            float z = (float(std::rand()) / float(RAND_MAX)) * 2.0f - 1.0f;
            pos.push_back(glm::normalize(glm::vec3(x, y, z)) * 20.0f);
        }

        // Base model: 90° around X then 180° around Z
        glm::mat4 baseModel = glm::rotate(glm::mat4(1.0f), glm::radians(90.0f),  glm::vec3(1, 0, 0))
                            * glm::rotate(glm::mat4(1.0f), glm::radians(180.0f), glm::vec3(0, 0, 1));

        // Per-mesh UniformBuffers
        std::vector<LavaCake::UniformBuffer> viewprojs;
        viewprojs.reserve(N);
        for (int i = 0; i < N; i++) {
            viewprojs.emplace_back(device);
            viewprojs[i].addVariable("model",     baseModel);
            viewprojs[i].addVariable("view",      view);
            viewprojs[i].addVariable("perpective", perspective);
            viewprojs[i].addVariable("nearPlane", nearPlane);
            viewprojs[i].end();
        }

        // Per-mesh storage buffers
        std::vector<LavaCake::Buffer> vertexBuffers;
        std::vector<LavaCake::Buffer> indexBuffers;
        vertexBuffers.reserve(N);
        indexBuffers.reserve(N);
        for (int i = 0; i < N; i++) {
            vertexBuffers.emplace_back(device, meshes[i].m_vertices, vk::BufferUsageFlagBits::eStorageBuffer);
            indexBuffers.emplace_back(device, meshes[i].m_indices,   vk::BufferUsageFlagBits::eStorageBuffer);
        }

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

        // Mesh descriptor layout (shared by all N sets)
        LavaCake::DescriptorSetLayout meshDescLayout = LavaCake::DescriptorSetLayout::Builder(device)
            .addUniformBuffer(0, vk::ShaderStageFlagBits::eMeshEXT | vk::ShaderStageFlagBits::eFragment)
            .addStorageBuffer(1, vk::ShaderStageFlagBits::eMeshEXT)
            .addStorageBuffer(2, vk::ShaderStageFlagBits::eMeshEXT)
            .build();

        // Mesh descriptor pool for all N sets
        LavaCake::DescriptorPool meshDescPool = LavaCake::DescriptorPool::Builder(device.getDevice())
            .addUniformBuffers(N)
            .addStorageBuffers(2 * N)
            .setMaxSets(N)
            .build();

        std::vector<vk::DescriptorSet> meshDescSets(N);
        for (int i = 0; i < N; i++) {
            meshDescSets[i] = meshDescPool.allocate(meshDescLayout);
            LavaCake::DescriptorSetUpdater(device, meshDescSets[i])
                .bindUniformBuffer(0, viewprojs[i], 0, viewprojs[i].size())
                .bindStorageBuffer(1, vertexBuffers[i])
                .bindStorageBuffer(2, indexBuffers[i])
                .update();
        }

        // Colors (rgba): defaults + specific mesh overrides
        std::vector<glm::vec4> colors(N, glm::vec4(0.0f, 0.0f, 0.0f, 0.1f));
        if (N > 1) colors[1] = glm::vec4(22.0f/255.0f, 0.0f, 1.0f, 0.403f);
        if (N > 2) colors[2] = glm::vec4(1.0f, 0.0f, 1.0f, 0.696f);
        if (N > 7) colors[7] = glm::vec4(1.0f, 0.0f, 0.0f, 0.659f);
        if (N > 8) colors[8] = glm::vec4(55.0f/255.0f, 1.0f, 0.0f, 0.403f);

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

        // Mesh pipeline with push constant for per-mesh color
        LavaCake::GraphicsPipeline graphicPipeline = LavaCake::GraphicsPipeline::Builder(device)
            .addShaderFromFile(root + "src/shader/jet/mesh.mesh",
                               vk::ShaderStageFlagBits::eMeshEXT,
                               LavaCake::ShadingLanguage::eGLSL)
            .addShaderFromFile(root + "src/shader/jet/interval.frag",
                               vk::ShaderStageFlagBits::eFragment,
                               LavaCake::ShadingLanguage::eGLSL)
            .addColorAttachmentFormat(offscreenFormat)
            .setColorBlendAttachments({meshBlend})
            .setCullMode(vk::CullModeFlagBits::eNone)
            .setDepthTest(false, false)
            .addDescriptorSetLayout(meshDescLayout)
            .addPushConstant<glm::vec4>(vk::ShaderStageFlagBits::eFragment, 0)
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
        glm::vec4  ppColor      = glm::vec4(255.0f/255.0f, 51.0f/255.0f, 51.0f/255.0f, 11.0f);
        glm::vec4  ppReflection = glm::vec4(1.0f, 1.0f, 1.0f, 0.25f);
        int        ppState      = 0;
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
        LavaCake::DescriptorSetLayout postDescLayout = LavaCake::DescriptorSetLayout::Builder(device)
            .addCombinedImageSampler(0, vk::ShaderStageFlagBits::eFragment)
            .addUniformBuffer(1, vk::ShaderStageFlagBits::eFragment)
            .build();

        LavaCake::DescriptorPool postDescPool = LavaCake::DescriptorPool::Builder(device.getDevice())
            .addCombinedImageSamplers(1)
            .addUniformBuffers(1)
            .setMaxSets(1)
            .build();

        vk::DescriptorSet postDescSet = postDescPool.allocate(postDescLayout);
        LavaCake::DescriptorSetUpdater(device, postDescSet)
            .bindImage(0, offscreenView, offscreenSampler)
            .bindUniformBuffer(1, ub, 0, ub.size())
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
            .addShaderFromFile(root + "src/shader/jet/postProcess.vert",
                               vk::ShaderStageFlagBits::eVertex,
                               LavaCake::ShadingLanguage::eGLSL)
            .addShaderFromFile(root + "src/shader/jet/postProcess.frag",
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
        float    dt           = 0.0f;
        float    elapsed      = 0.0f;
        float    elapsed2     = 0.0f;
        bool     pause        = false;

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
            for (int i = 0; i < N; i++) {
                std::string colorName   = "Color "   + std::to_string(i);
                std::string opacityName = "Opacity " + std::to_string(i);
                ImGui::ColorEdit3(colorName.c_str(),    &colors[i].x);
                ImGui::SliderFloat(opacityName.c_str(), &colors[i].w, 0.0f, 10.0f);
            }
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

                if (glfwGetKey(window, GLFW_KEY_P) == GLFW_PRESS) pause = true;
                if (glfwGetKey(window, GLFW_KEY_O) == GLFW_PRESS) pause = false;

                if (scrolled) {
                    scrolled = false;
                    trackball.zoom(float(scroll));
                }
            }

            view = trackball.getview();

            // Update per-mesh model matrices (animated)
            float tmp = (sinf((elapsed * 10.0f) - 3.14159265f / 2.0f) + 1.0f);
            for (int i = 0; i < N; i++) {
                glm::mat4 model = glm::rotate(glm::mat4(1.0f), glm::radians(90.0f),  glm::vec3(1, 0, 0))
                                * glm::rotate(glm::mat4(1.0f), glm::radians(180.0f), glm::vec3(0, 0, 1))
                                * glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, pos[i].y * tmp, pos[i].z * tmp));
                viewprojs[i].setVariable("view",  view);
                viewprojs[i].setVariable("model", model);
            }

            // Mesh 7: spinning rotation (overrides the translation-based model above)
            if (N > 7) {
                glm::mat4 rotation = glm::rotate(glm::mat4(1.0f), glm::radians(elapsed2 * 1000.0f), glm::vec3(0, 1, 0))
                                   * glm::rotate(glm::mat4(1.0f), glm::radians(90.0f),             glm::vec3(1, 0, 0))
                                   * glm::rotate(glm::mat4(1.0f), glm::radians(180.0f),            glm::vec3(0, 0, 1));
                viewprojs[7].setVariable("model", rotation);
            }

            // Update post-process UB
            ub.setVariable("camera",     view);
            ub.setVariable("camPos",     trackball.m_pos);
            ub.setVariable("color",      ppColor);
            ub.setVariable("reflection", ppReflection);
            ub.setVariable("State",      ppState);

            // Acquire swapchain image
            LavaCake::SwapChainImage& swapchainImage =
                device.aquireSwapChainImage(imageAvailableSemaphores[currentFrame]);

            auto frameStart = std::chrono::high_resolution_clock::now();

            // Begin recording
            cmdBuffer.begin(vk::CommandBufferUsageFlagBits::eOneTimeSubmit);
            vk::CommandBuffer cmd = cmdBuffer.getCommandBuffer();

            for (int i = 0; i < N; i++) {
                viewprojs[i].update(cmd);
            }
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
                for (int i = 0; i < N; i++) {
                    graphicPipeline.bindDescriptorSets(cmd, {meshDescSets[i]});
                    graphicPipeline.pushConstants<glm::vec4>(cmd, vk::ShaderStageFlagBits::eFragment, 0, colors[i]);
                    cmd.drawMeshTasksEXT(uint32_t(meshes[i].m_indices.size()), 1, 1);
                }
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
            dt = cost / 1000.0f;
            elapsed2 += dt;
            if (!pause) elapsed += dt;
            ppState = int(elapsed / freq);
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
