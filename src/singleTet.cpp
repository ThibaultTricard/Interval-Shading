#define LAVACAKE_WINDOW_MANAGER_GLFW
#include <LavaCake/Framework/Framework.h> 
#include <LavaCake/Helpers/ABBox.h> 

#include "utils/trackball.h"
#include "utils/Tetmesh.h"

using namespace LavaCake;
using namespace LavaCake::Geometry;
using namespace LavaCake::Framework;
#include <chrono>
#undef near

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
	GLFWwindow* window = glfwCreateWindow(1920, 1080, "Interval Shading - Single Tetrahedron", nullptr, nullptr);
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

	MeshShaderModule meshShader(root+"/shaders/singleTet/mesh.mesh.spv");
	FragmentShaderModule fragmentShader(root+"/shaders/singleTet/interval.frag.spv");
	

	VkExtent2D size = swapChain->size();
	
	float freq = 2.5f;

	Trackball trackball(vec3f({0.0f,-2.f,0.0f}),vec3f({0.0f,0.0f,0.0f}),vec3f({0.0f,0.0f,-1.0f}));
	trackball.m_moveSpeed = 0.02f;


	trackball.m_rotationSpeed = 3.14159265f*2.0 / freq;
	
	auto view = trackball.getview();


	UniformBuffer viewproj;

	auto model = Identity();
	viewproj.addVariable("model", transpose(model));
	viewproj.addVariable("view", transpose(view));

	float near = 0.1f;
	auto perspective = PreparePerspectiveProjectionMatrix(float(size.width)/float(size.height),80.0f,near,10.0f);
  	perspective = transpose(perspective);
  	viewproj.addVariable("perpective", perspective );
	viewproj.addVariable("near", near );
	viewproj.addVariable("tetnumber", int(mesh.m_indices.size()) );

	viewproj.end();
	
	vec2u res = vec2u({1920, 1080});
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
	//graphicPipeline.setPolygonMode(VK_POLYGON_MODE_LINE);
	//graphicPipeline.setLineWidth(2.0f);
	graphicPipeline.setCullMode(VK_CULL_MODE_NONE);
	graphicPipeline.setVerticesInfo({},{},VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);


	RenderPass renderPass;

	SubPassAttachments SA;
	
	SA.addSwapChainImageAttachment(swapChain->imageFormat());
	SA.m_attachments[0].m_blendState = {
		VK_TRUE,                                       	// VkBool32                 blendEnable
		VK_BLEND_FACTOR_ONE,                      		// VkBlendFactor            srcColorBlendFactor
		VK_BLEND_FACTOR_ONE,            			    // VkBlendFactor            dstColorBlendFactor
		VK_BLEND_OP_ADD,                                // VkBlendOp                colorBlendOp
		VK_BLEND_FACTOR_ONE,           					// VkBlendFactor            srcAlphaBlendFactor
		VK_BLEND_FACTOR_ONE,                            // VkBlendFactor            dstAlphaBlendFactor
		VK_BLEND_OP_ADD,                                // VkBlendOp                colorBlendOp
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

	commandBuffer.wait();
	commandBuffer.resetFence();

	
	while (!glfwWindowShouldClose(window)) {
		
		commandBuffer.wait();
		glfwPollEvents();

		
		const SwapChainImage& image = swapChain->acquireImage();
		std::vector<waitSemaphoreInfo> waitSemaphoreInfos = {};
		waitSemaphoreInfos.push_back({
			image.getSemaphore(),                           // VkSemaphore            Semaphore
			VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT	  // VkPipelineStageFlags   WaitingStage
		});
		
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
	
		
		view = trackball.getview();
    	viewproj.setVariable("view",transpose(view));


		commandBuffer.beginRecord();

        renderPass.setSwapChainImage(frameBuffer, image);

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


		commandBuffer.endRecord();
		commandBuffer.submit(graphicQueue, waitSemaphoreInfos, { semaphore });

		swapChain->presentImage(presentQueue, image, { semaphore });

		commandBuffer.wait();
		commandBuffer.resetFence();


		
	}
	device->waitForAllCommands();
}