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
    GLFWwindow* window = glfwCreateWindow(1920, 1080, "Interval Shading - Single Tetrahedron", nullptr, nullptr);
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

        // Hardcoded single tetrahedron
        std::vector<glm::vec4> vertices;
        vertices.push_back(glm::vec4( sqrtf(8.0f/9.0f), 0.0f,              -1.0f/3.0f, 1.0f));
        vertices.push_back(glm::vec4(-sqrtf(2.0f/9.0f), sqrtf(2.0f/3.0f), -1.0f/3.0f, 1.0f));
        vertices.push_back(glm::vec4(-sqrtf(2.0f/9.0f),-sqrtf(2.0f/3.0f), -1.0f/3.0f, 1.0f));
        vertices.push_back(glm::vec4( 0.0f,              0.0f,              1.0f,       1.0f));

        std::vector<glm::uvec4> indices;
        indices.push_back(glm::uvec4(0, 1, 2, 3));
        Tetmesh mesh(vertices, indices);

        // Trackball setup
        float freq = 2.5f;
        Trackball trackball(
            glm::vec3(0.0f, -2.0f, 0.0f),
            glm::vec3(0.0f,  0.0f, 0.0f),
            glm::vec3(0.0f,  0.0f, -1.0f)
        );
        trackball.m_moveSpeed = 0.02f;
        trackball.m_rotationSpeed = 3.14159265f * 2.0f / freq;

        auto view = trackball.getview();

        // Uniform buffer
        LavaCake::UniformBuffer viewproj(device);
        glm::mat4 model = glm::identity<glm::mat4>();
        viewproj.addVariable("model", model);
        viewproj.addVariable("view", view);

        float nearPlane = 0.1f;
        vk::Extent2D swapExtent = device.getSwapchainExtent();
        glm::mat4 perspective = glm::perspectiveLH(glm::radians(80.0f),
            float(swapExtent.width) / float(swapExtent.height), nearPlane, 10.0f);
        perspective[1][1] *= -1.0f;
        viewproj.addVariable("perpective", perspective);
        viewproj.addVariable("nearPlane", nearPlane);
        viewproj.addVariable("tetnumber", int(mesh.m_indices.size()));
        viewproj.end();

        // Storage buffers
        LavaCake::Buffer vertexBuffer(device, mesh.m_vertices, vk::BufferUsageFlagBits::eStorageBuffer);
        LavaCake::Buffer indicesBuffer(device, mesh.m_indices,  vk::BufferUsageFlagBits::eStorageBuffer);

        // Descriptor layout / pool / set
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

        // Additive blend (ADD for both color and alpha)
        vk::PipelineColorBlendAttachmentState meshBlend{};
        meshBlend.blendEnable         = VK_TRUE;
        meshBlend.srcColorBlendFactor = vk::BlendFactor::eOne;
        meshBlend.dstColorBlendFactor = vk::BlendFactor::eOne;
        meshBlend.colorBlendOp        = vk::BlendOp::eAdd;
        meshBlend.srcAlphaBlendFactor = vk::BlendFactor::eOne;
        meshBlend.dstAlphaBlendFactor = vk::BlendFactor::eOne;
        meshBlend.alphaBlendOp        = vk::BlendOp::eAdd;
        meshBlend.colorWriteMask      = vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG
                                      | vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA;

        // Mesh pipeline rendering directly to swapchain
        LavaCake::GraphicsPipeline graphicPipeline = LavaCake::GraphicsPipeline::Builder(device)
            .addShaderFromFile(root + "src/shader/singleTet/mesh.mesh",
                               vk::ShaderStageFlagBits::eMeshEXT,
                               LavaCake::ShadingLanguage::eGLSL)
            .addShaderFromFile(root + "src/shader/singleTet/interval.frag",
                               vk::ShaderStageFlagBits::eFragment,
                               LavaCake::ShadingLanguage::eGLSL)
            .addColorAttachmentFormat(device.getSwapchainFormat())
            .setColorBlendAttachments({meshBlend})
            .setCullMode(vk::CullModeFlagBits::eNone)
            .setDepthTest(false, false)
            .addDescriptorSetLayout(meshDescLayout)
            .build();

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

        // Main render loop
        while (!glfwWindowShouldClose(window)) {
            cmdBuffer.waitForCompletion();
            cmdBuffer.reset();
            glfwPollEvents();

            // Camera input
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

            view = trackball.getview();
            viewproj.setVariable("view", view);

            // Acquire swapchain image
            LavaCake::SwapChainImage& swapchainImage =
                device.aquireSwapChainImage(imageAvailableSemaphores[currentFrame]);

            // Begin recording
            cmdBuffer.begin(vk::CommandBufferUsageFlagBits::eOneTimeSubmit);
            vk::CommandBuffer cmd = cmdBuffer.getCommandBuffer();

            viewproj.update(cmd);

            // Render mesh directly to swapchain
            swapchainImage.prepareForAttachementBarrier(cmd);
            {
                auto ctx = LavaCake::DynamicRenderingContext::Builder()
                    .setRenderArea(device.getSwapchainExtent())
                    .addColorAttachment(swapchainImage,
                        vk::ClearColorValue(std::array<float,4>{0.0f, 0.0f, 0.0f, 1.0f}))
                    .begin(cmd);

                ctx.setDefaultViewportScissor(cmd);
                graphicPipeline.bind(cmd);
                graphicPipeline.bindDescriptorSets(cmd, {meshDescSet});
                cmd.drawMeshTasksEXT(uint32_t(mesh.m_indices.size()), 1, 1);
                ctx.end(cmd);
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
        }

        device.waitForAllCommands();

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
