#define LAVACAKE_WINDOW_MANAGER_GLFW
#include <LavaCake/Framework/Framework.h> 
#include <LavaCake/Helpers/ABBox.h> 

#include "utils/Tetmesh.h"
#include "utils/trackball.h"

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
    scroll = -yoffset *50;
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
	GLFWwindow* window = glfwCreateWindow(1920, 1080, "Interval Shading - Asteroids", nullptr, nullptr);
	glfwSetScrollCallback(window, scroll_callback);
	ErrorCheck::printError(true, 5);
	
	GLFWSurfaceInitialisator surfaceInitialisator(window);

	Device* device = Device::getDevice();
	device->enableMeshShader();
	VkPhysicalDeviceFeatures feature;
	device->initDevices(1, 1, surfaceInitialisator);

	SwapChain* swapChain = SwapChain::getSwapChain();
	swapChain->init();

	std::vector<vec4f> vertices;
	vertices.push_back(vec4f({sqrtf(8.0f/9.0f), 0.0f, -1.0f/3.0f,1.0}));
	vertices.push_back(vec4f({-sqrtf(2.0f/9.0f), sqrtf(2.0f/3.0f), -1.0f/3.0f,1.0}));
	vertices.push_back(vec4f({-sqrtf(2.0f/9.0f), -sqrtf(2.0f/3.0f), -1.0f/3.0f,1.0}));
	vertices.push_back(vec4f({0.0,0.0,1.0,1.0}));


	std::vector<vec4u> indices;
	indices.push_back(vec4u({0,1,2,3}));
	Tetmesh mesh = Tetmesh(vertices,indices);

	CommandBuffer  commandBuffer;
	std::shared_ptr<Semaphore> semaphore = std::make_shared<Semaphore>();

	GraphicQueue graphicQueue = device->getGraphicQueue(0);
	PresentationQueue presentQueue = device->getPresentQueue();

	MeshShaderModule meshShader(root+"/shaders/asteroid/mesh.mesh.spv");
	FragmentShaderModule fragmentShader(root+"/shaders/asteroid/interval.frag.spv");
	

	VkExtent2D size = swapChain->size();
	
	Trackball trackball(vec3f({0.0f,0.0f,-15.0f}),vec3f({0.0f,0.0f,0.0f}),vec3f({0.0f,1.0f,0.0f}));
	trackball.m_moveSpeed = 0.02f;
	trackball.m_rotationSpeed = 1.0f;
	auto view = trackball.getview();


	UniformBuffer viewproj;

    float scale = 1.0;
	viewproj.addVariable("view", transpose(view));

	float time = 20.0f;
	float nearPlane = 0.01f;
	float dt = 0.0;
	auto perspective = PreparePerspectiveProjectionMatrix(float(size.width)/float(size.height),80.0f,nearPlane,200.0f);
  	perspective = transpose(perspective);
  	viewproj.addVariable("perpective", perspective );
	viewproj.addVariable("nearPlane", nearPlane );
	viewproj.addVariable("time", time );
	viewproj.addVariable("dt", dt );
	viewproj.end();


	int instance = 10000;
	std::vector<mat4f> models;

	float range = 12.5f ;

	float scale_max = 3.0f;
	float scale_min = 1.0f;

	for(int i = 0; i < instance; i++){
		models.push_back(Identity());
	}

	vec2u res = vec2u({3840,2160});

	Buffer vertexBuffer(graphicQueue, commandBuffer, mesh.m_vertices, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,VK_PIPELINE_STAGE_TRANSFER_BIT,VK_FORMAT_UNDEFINED);
	Buffer indicesBuffer(graphicQueue, commandBuffer, mesh.m_indices, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,VK_PIPELINE_STAGE_TRANSFER_BIT,VK_FORMAT_UNDEFINED);
	Buffer modelsBuffer(graphicQueue, commandBuffer, models, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT|VK_PIPELINE_STAGE_MESH_SHADER_BIT_EXT|VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,VK_FORMAT_UNDEFINED,VK_ACCESS_SHADER_WRITE_BIT);

	DescriptorSet set;
	set.addUniformBuffer(viewproj, VK_SHADER_STAGE_MESH_BIT_EXT | VK_SHADER_STAGE_FRAGMENT_BIT| VK_SHADER_STAGE_COMPUTE_BIT, 0);
	set.addBuffer(vertexBuffer, VK_SHADER_STAGE_MESH_BIT_EXT , 1);
	set.addBuffer(indicesBuffer, VK_SHADER_STAGE_MESH_BIT_EXT , 2);
	set.addBuffer(modelsBuffer, VK_SHADER_STAGE_MESH_BIT_EXT | VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_COMPUTE_BIT, 3);
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

	
	RenderPass renderPass;

	SubPassAttachments SA;
	SA.setDepthFormat(VK_FORMAT_D16_UNORM);

	SA.addColorAttachment(VK_FORMAT_R32G32B32A32_SFLOAT);


	uint32_t passNumber = renderPass.addSubPass(SA);
	graphicPipeline.setSubPassNumber(passNumber);
	
	uint64_t start = 0; 
	renderPass.compile();
	
	graphicPipeline.compile(renderPass.getHandle(),SA);

	
	FrameBuffer frameBuffer =  FrameBuffer(res[0], res[1]);
	renderPass.prepareOutputFrameBuffer(graphicQueue, commandBuffer, frameBuffer);
	
	
	ComputePipeline modelPipeline;

	ComputeShaderModule modelShader(root+"/shaders/asteroid/model.comp.spv");
	modelPipeline.setComputeModule(modelShader);

	modelPipeline.setDescriptorLayout(set.getLayout());

  	modelPipeline.compile();
	

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

    VertexShaderModule possProcessVert(root+"/shaders/asteroid/postProcess.vert.spv");
	FragmentShaderModule possProcessFrag(root+"/shaders/asteroid/postProcess.frag.spv");
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

	float freq = 10.0f;
	trackball.m_rotationSpeed = 3.14159265f*2.0 / freq;

	time = 33.6981;
	trackball.rotate(vec2f({time,0.0f}));


	while (!glfwWindowShouldClose(window)) {
		
		commandBuffer.wait();
		glfwPollEvents();

		const SwapChainImage& image = swapChain->acquireImage();
		std::vector<waitSemaphoreInfo> waitSemaphoreInfos = {};
		waitSemaphoreInfos.push_back({
			image.getSemaphore(),                           // VkSemaphore            Semaphore
			VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT	  // VkPipelineStageFlags   WaitingStage
		});

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
		
		
		view = trackball.getview();
    	viewproj.setVariable("view",transpose(view));

		viewproj.setVariable("time",time);
		viewproj.setVariable("dt",dt);
		
		commandBuffer.beginRecord();

		viewproj.update(commandBuffer);
		modelPipeline.bindPipeline(commandBuffer);
		modelPipeline.bindDescriptorSet(commandBuffer,set);
		modelPipeline.compute(commandBuffer, instance / 32 +1, 1, 1);

		renderPass.begin(commandBuffer, 
			frameBuffer, 
			vec2u({ 0,0 }), 
			res, 
			{  { 0.0f, 0.0f, 0.0f, 0.0f }, { 1.0f, 0.0f},});

		graphicPipeline.bindPipeline(commandBuffer);
		graphicPipeline.bindDescriptorSet(commandBuffer, set);

		drawMeshTasks(commandBuffer, instance,1,1);
		
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
		{{ 0.0f, 0.0f, 0.0f, 0.0f }, { 1.0f, 0 }});

		postProcessPipeline.bindPipeline(commandBuffer);
		
		postProcessPipeline.bindDescriptorSet(commandBuffer,postProcessDescriptor);
		bindVertexBuffer(commandBuffer, *quad_vertex_buffer->getVertexBuffer());
		bindIndexBuffer(commandBuffer, *quad_vertex_buffer->getIndexBuffer());

		drawIndexed(commandBuffer, quad_vertex_buffer->getIndicesNumber());
		

		postProcessPass.end(commandBuffer);

		commandBuffer.endRecord();
		start = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
		commandBuffer.submit(graphicQueue, waitSemaphoreInfos, { semaphore });

		swapChain->presentImage(presentQueue, image, { semaphore });

		commandBuffer.wait();
		commandBuffer.resetFence();
		uint64_t stop = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
		
		dt = float(stop - start) / 1000000.0;
		time += dt;

	}
	device->waitForAllCommands();
}