#define LAVACAKE_WINDOW_MANAGER_GLFW
#include <LavaCake/Framework/Framework.h> 
#include <LavaCake/Helpers/ABBox.h> 

#include "utils/trackball.h"
#include "utils/Tetmesh.h"

using namespace LavaCake;
using namespace LavaCake::Geometry;
using namespace LavaCake::Framework;
#include <chrono>



vec2d mousepos = vec2d({0,0});

vec2d get_mouse_mouvement(GLFWwindow* window){
  
  vec2d new_mousepos;
  glfwGetCursorPos(window, &new_mousepos[0], &new_mousepos[1]);
  auto delta = new_mousepos - mousepos;
  mousepos = vec2d(new_mousepos.data);
  return delta;
}

double scroll = 0;
bool scrolled = false;
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
{
  if(yoffset!=0){
    scroll = -yoffset * 10.0f;
    scrolled = true;
  }

}


vec3f get_movement(GLFWwindow* window){
    vec3f translation;
    int state = glfwGetKey(window, GLFW_KEY_UP);
    if (state == GLFW_PRESS)
    {
        translation = translation + vec3f({0.0f,0.0f,-1.0});
    }
    state = glfwGetKey(window, GLFW_KEY_DOWN);
    if (state == GLFW_PRESS)
    {
        translation = translation + vec3f({0.0f,0.0f,1.0});
    }

    state = glfwGetKey(window, GLFW_KEY_LEFT);
    if (state == GLFW_PRESS)
    {
        translation = translation + vec3f({-1.0f,0.0f,0.0});
    }
    state = glfwGetKey(window, GLFW_KEY_RIGHT);
    if (state == GLFW_PRESS)
    {
       translation = translation +  vec3f({1.0f,0.0f,0.0});
    }

    state = glfwGetKey(window, GLFW_KEY_SPACE);
    if (state == GLFW_PRESS)
    {
       translation = translation +  vec3f({0.0f,-1.0f,0.0});
    }

    state = glfwGetKey(window, GLFW_KEY_RIGHT_CONTROL);
    if (state == GLFW_PRESS)
    {
       translation = translation +  vec3f({0.0f,1.0f,0.0});
    }

    return translation;
}

std::string root = PROJECT_ROOT;

int main() {

	glfwInit();
	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
	GLFWwindow* window = glfwCreateWindow(1920, 1080, "Interval Shading - Armadillo", nullptr, nullptr);
	glfwSetScrollCallback(window, scroll_callback);
	ErrorCheck::printError(true, 5);
	
	GLFWSurfaceInitialisator surfaceInitialisator(window);

	Device* device = Device::getDevice();
	device->enableMeshShader();
	VkPhysicalDeviceFeatures feature;
	device->initDevices(1, 1, surfaceInitialisator);

	SwapChain* swapChain = SwapChain::getSwapChain();
	swapChain->init();

	
	
	Tetmesh mesh = load_msh(root+"/objects/Armadillo.vtk");
	std::cout<<mesh.m_indices.size()<<std::endl;

	CommandBuffer  commandBuffer;
	std::shared_ptr<Semaphore> semaphore = std::make_shared<Semaphore>();

	GraphicQueue graphicQueue = device->getGraphicQueue(0);
	PresentationQueue presentQueue = device->getPresentQueue();

	MeshShaderModule meshShader(root+"/shaders/armadillo/mesh.mesh.spv");
	FragmentShaderModule fragmentShader(root+"/shaders/armadillo/interval.frag.spv");
	

	VkExtent2D size = swapChain->size();
	
	

	Trackball trackball(vec3f({0.0f,-1.1f,0.0f}),vec3f({0.0f,0.0f,0.0f}),vec3f({0.0f,0.0f,-1.0f}));
	trackball.m_moveSpeed = 0.02f;

	float freq = 10.0f;
	trackball.m_rotationSpeed = 3.14159265f*2.0 / freq;
	
	auto view = trackball.getview();


	UniformBuffer viewproj;

	auto model = Identity();
	model = PrepareRotationMatrix(90,vec3f({1,0,0})) * PrepareRotationMatrix(180,vec3f({0,0,1}));
	viewproj.addVariable("model", transpose(model));
	viewproj.addVariable("view", transpose(view));

	float near = 0.1f;
	auto perspective = PreparePerspectiveProjectionMatrix(float(size.width)/float(size.height),80.0f,near,1000.0f);
  	perspective = transpose(perspective);
  	viewproj.addVariable("perpective", perspective );
	viewproj.addVariable("near", near );
	viewproj.addVariable("tetnumber", int(mesh.m_indices.size()) );

	viewproj.end();

	vec2u res = vec2u({3840,2160});

	Buffer vertexBuffer(graphicQueue, commandBuffer, mesh.m_vertices, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,VK_PIPELINE_STAGE_TRANSFER_BIT,VK_FORMAT_UNDEFINED);
	Buffer indicesBuffer(graphicQueue, commandBuffer, mesh.m_indices, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,VK_PIPELINE_STAGE_TRANSFER_BIT,VK_FORMAT_UNDEFINED);

	DescriptorSet set;
	set.addUniformBuffer(viewproj, VK_SHADER_STAGE_MESH_BIT_EXT | VK_SHADER_STAGE_FRAGMENT_BIT, 0);
	set.addBuffer(vertexBuffer, VK_SHADER_STAGE_MESH_BIT_EXT , 1);
	set.addBuffer(indicesBuffer, VK_SHADER_STAGE_MESH_BIT_EXT , 2);
	set.compile();

	GraphicPipeline graphicPipeline(
		vec3f({ 0,0,0 }), 
		vec3f({ float(res[0]),float(res[1]),1.0f }), 
		vec2f({ 0,0 }), 
		vec2f({ float(res[0]),float(res[1]) })
	);

	graphicPipeline.setMeshModule(meshShader);
	graphicPipeline.setFragmentModule(fragmentShader);
	graphicPipeline.setDescriptorLayout(set.getLayout());
	graphicPipeline.setCullMode(VK_CULL_MODE_NONE);
	graphicPipeline.setVerticesInfo({},{},VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);


	RenderPass renderPass;

	SubPassAttachments SA;
	SA.addColorAttachment(VK_FORMAT_R32G32B32A32_SFLOAT);
	SA.m_attachments[0].m_blendState = {
		VK_TRUE,                                       	// VkBool32                 blendEnable
		VK_BLEND_FACTOR_ONE,                      		// VkBlendFactor            srcColorBlendFactor
		VK_BLEND_FACTOR_ONE,            			    // VkBlendFactor            dstColorBlendFactor
		VK_BLEND_OP_ADD,                                // VkBlendOp                colorBlendOp
		VK_BLEND_FACTOR_ONE,           					// VkBlendFactor            srcAlphaBlendFactor
		VK_BLEND_FACTOR_ONE,                            // VkBlendFactor            dstAlphaBlendFactor
		VK_BLEND_OP_MIN,                                // VkBlendOp                alphaBlendOp
		VK_COLOR_COMPONENT_R_BIT |                      // VkColorComponentFlags    colorWriteMask
		VK_COLOR_COMPONENT_G_BIT |
		VK_COLOR_COMPONENT_B_BIT |
		VK_COLOR_COMPONENT_A_BIT
	};

	uint32_t passNumber = renderPass.addSubPass(SA);
	graphicPipeline.setSubPassNumber(passNumber);


	uint64_t start = 0; 
	renderPass.compile();

	graphicPipeline.compile(renderPass.getHandle(),SA);

	FrameBuffer frameBuffer =  FrameBuffer(res[0], res[1]);
	renderPass.prepareOutputFrameBuffer(graphicQueue, commandBuffer, frameBuffer);

	VkImageMemoryBarrier frameMemoryBarrier;
	frameMemoryBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	frameMemoryBarrier.pNext = nullptr;
	frameMemoryBarrier.srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
	frameMemoryBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
	frameMemoryBarrier.oldLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	frameMemoryBarrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	frameMemoryBarrier.srcQueueFamilyIndex = graphicQueue.getIndex();
	frameMemoryBarrier.dstQueueFamilyIndex = graphicQueue.getIndex();
	frameMemoryBarrier.image = frameBuffer.getImage(0)->getHandle();
	frameMemoryBarrier.subresourceRange = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 };

    std::shared_ptr<Mesh_t> quad =std::make_shared<IndexedMesh<Geometry::TRIANGLE>>(Geometry::P3UV);
	
	quad->appendVertex({ -1.0,-1.0,0.0,0.0,0.0 });
	quad->appendVertex({ -1.0, 1.0,0.0,0.0,1.0 });
	quad->appendVertex({  1.0, 1.0,0.0,1.0,1.0 });
	quad->appendVertex({  1.0,-1.0,0.0,1.0,0.0 });

	quad->appendIndex(0);
	quad->appendIndex(1);
	quad->appendIndex(2);

	quad->appendIndex(2);
	quad->appendIndex(3);
	quad->appendIndex(0);              
	
	std::shared_ptr<VertexBuffer> quad_vertex_buffer = std::make_shared<VertexBuffer>(graphicQueue, commandBuffer, std::vector<std::shared_ptr<Mesh_t>>({ quad }));


    DescriptorSet postProcessDescriptor;
	postProcessDescriptor.addFrameBuffer(frameBuffer,VK_SHADER_STAGE_FRAGMENT_BIT,0);
    postProcessDescriptor.compile();

    GraphicPipeline postProcessPipeline(
		vec3f({ 0,0,0 }), 
		vec3f({ float(size.width),float(size.height),1.0f }), 
		vec2f({ 0,0 }), 
		vec2f({ float(size.width),float(size.height) })
	);

    VertexShaderModule possProcessVert(root+"/shaders/armadillo/postProcess.vert.spv");
	FragmentShaderModule possProcessFrag(root+"/shaders/armadillo/postProcess.frag.spv");
	postProcessPipeline.setVertexModule(possProcessVert);
	postProcessPipeline.setFragmentModule(possProcessFrag);
	postProcessPipeline.setVerticesInfo(quad_vertex_buffer->getBindingDescriptions(), quad_vertex_buffer->getAttributeDescriptions(), quad_vertex_buffer->primitiveTopology());
	postProcessPipeline.setDescriptorLayout(postProcessDescriptor.getLayout());

	SubPassAttachments SA2;
  	SA2.addSwapChainImageAttachment(swapChain->imageFormat());
    SA2.m_attachments[0].m_blendState.blendEnable = VK_TRUE;
 
    RenderPass postProcessPass;

	postProcessPass.addSubPass(SA2);

	postProcessPass.compile();

	postProcessPipeline.compile(postProcessPass.getHandle(),SA2);

	ImGuiWrapper* gui = new ImGuiWrapper(graphicQueue, commandBuffer, vec2i({ (int)size.width ,(int)size.height }), vec2i({ (int)size.width ,(int)size.height }));
    
	prepareInputs(window);

    gui->getPipeline()->setSubPassNumber(0);
    gui->getPipeline()->compile(postProcessPass.getHandle(),SA2);

	FrameBuffer postProcessBuffer(swapChain->size().width, swapChain->size().height);
	postProcessPass.prepareOutputFrameBuffer(graphicQueue, commandBuffer, postProcessBuffer);

	
	commandBuffer.wait();
	commandBuffer.resetFence();

	trackball.rotate(vec2f({0.8,0.0f}));
	trackball.move(vec3f({0.0f,-10.0f,0.0f}));
	float dt = 0.0;
	float elapsed = freq*0.0f;
    float cost = 0;

	bool rotate = false;
	bool pressed = false;

	float avgCost = 0.0;

	while (!glfwWindowShouldClose(window)) {
		
		commandBuffer.wait();
		glfwPollEvents();

		{
			ImGuiIO& io = ImGui::GetIO();
			IM_ASSERT(io.Fonts->IsBuilt() && "Font atlas not built! It is generally built by the renderer back-end. Missing call to renderer _NewFrame() function? e.g. ImGui_ImplOpenGL3_NewFrame().");

			// Setup display size (every frame to accommodate for window resizing)
			int width, height;
			int display_w, display_h;
			glfwGetWindowSize(window, &width, &height);
			glfwGetFramebufferSize(window, &display_w, &display_h);
			io.DisplaySize = ImVec2((float)width, (float)height);
			if (width > 0 && height > 0)
				io.DisplayFramebufferScale = ImVec2((float)display_w / width, (float)display_h / height);
		}

		ImGui::NewFrame();
        ImGui::Begin("Controls");
		std::string text = "cost: "+std::to_string(cost) + "ms";
		ImGui::Text(text.data());
		ImGui::End();
        gui->prepareGui(graphicQueue, commandBuffer);

		const SwapChainImage& image = swapChain->acquireImage();
		std::vector<waitSemaphoreInfo> waitSemaphoreInfos = {};
		waitSemaphoreInfos.push_back({
			image.getSemaphore(),                           // VkSemaphore            Semaphore
			VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT	  // VkPipelineStageFlags   WaitingStage
		});

		
		ImGuiIO& io = ImGui::GetIO();

		if(!io.WantCaptureMouse){
			auto translation = get_movement(window);
			trackball.move(translation);

			
			int state = glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT);
		

			auto dmov = get_mouse_mouvement(window);
			if (state == GLFW_PRESS){

				dmov[0] = dmov[0]/float(size.width) *2.0 * 3.14159265;
				dmov[1] = dmov[1]/float(size.width) *2.0 * 3.14159265;
				trackball.rotate(vec2f({float(dmov[0]),float(dmov[1])}));
			}

			if(scrolled){
				scrolled = false;
				trackball.zoom(scroll);
			}
		
		}
		
		
		view = trackball.getview();
    	viewproj.setVariable("view",transpose(view));

		commandBuffer.beginRecord();

		viewproj.update(commandBuffer);
		renderPass.begin(commandBuffer, 
			frameBuffer, 
			vec2u({ 0,0 }), 
			vec2u({ uint32_t (res[0]), uint32_t (res[1]) }), 
			{ { 0.0f, 0.0f, 0.0f, 1.0f } });

		graphicPipeline.bindPipeline(commandBuffer);
		graphicPipeline.bindDescriptorSet(commandBuffer, set);

		drawMeshTasks(commandBuffer, mesh.m_indices.size() ,1,1);
		renderPass.end(commandBuffer);

		LavaCake::vkCmdPipelineBarrier(
        commandBuffer.getHandle(),
        VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
        VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
        0,
        0, nullptr,
        0, nullptr,
        1, &frameMemoryBarrier);

        postProcessPass.setSwapChainImage(postProcessBuffer, image);
		postProcessPass.begin(commandBuffer, postProcessBuffer, vec2u({ 0,0 }), vec2u({size.width, size.height}), 
		{{ 0.0f, 0.0f, 0.0f, 1.0f }, { 1.0f, 0 }});

		postProcessPipeline.bindPipeline(commandBuffer);
		
		postProcessPipeline.bindDescriptorSet(commandBuffer,postProcessDescriptor);
		bindVertexBuffer(commandBuffer, *quad_vertex_buffer->getVertexBuffer());
		bindIndexBuffer(commandBuffer, *quad_vertex_buffer->getIndexBuffer());

		drawIndexed(commandBuffer, quad_vertex_buffer->getIndicesNumber());
		

        gui->drawGui(commandBuffer);
		postProcessPass.end(commandBuffer);


		commandBuffer.endRecord();
		start = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
		commandBuffer.submit(graphicQueue, waitSemaphoreInfos, { semaphore });

		swapChain->presentImage(presentQueue, image, { semaphore });

		commandBuffer.wait();
		commandBuffer.resetFence();
		uint64_t stop = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::system_clock::now().time_since_epoch()).count();

		cost = float(stop - start)/1000.0f;
		
		dt = cost/1000.0f;
		
	}
	device->waitForAllCommands();
}