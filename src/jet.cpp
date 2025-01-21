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
	GLFWwindow* window = glfwCreateWindow(1920, 1080, "Mesh Shading", nullptr, nullptr);
	glfwSetScrollCallback(window, scroll_callback);
	ErrorCheck::printError(true, 5);
	
	GLFWSurfaceInitialisator surfaceInitialisator(window);

	Device* device = Device::getDevice();
	device->enableMeshShader();
	VkPhysicalDeviceFeatures feature;
	device->initDevices(1, 1, surfaceInitialisator);

	SwapChain* swapChain = SwapChain::getSwapChain();
	swapChain->init();

	GraphicQueue graphicQueue = device->getGraphicQueue(0);
	PresentationQueue presentQueue = device->getPresentQueue();

	CommandBuffer  commandBuffer;
	std::shared_ptr<Semaphore> semaphore = std::make_shared<Semaphore>();

	std::vector<std::string> names = {
		"../objects/jet/airbus_simplifiedBackCover.vtk",
		"../objects/jet/airbus_simplifiedBackCover_001.vtk",
		"../objects/jet/airbus_simplifiedBackCover_002.vtk",
		"../objects/jet/airbus_simplifiedBackCover_003.vtk",
		"../objects/jet/airbus_simplifiedBackCover_004.vtk",
		"../objects/jet/airbus_simplifiedBackCover_005.vtk",
		"../objects/jet/airbus_simplifiedBackCover_006.vtk",
		"../objects/jet/airbus_simplifiedBackCover_007.vtk",
		"../objects/jet/airbus_simplifiedBackCover_008.vtk",
		"../objects/jet/airbus_simplifiedBackCover_009.vtk",
		"../objects/jet/airbus_simplifiedBackCover_010.vtk",
		"../objects/jet/airbus_simplifiedBackCover_011.vtk",
		"../objects/jet/airbus_simplifiedBackCover_012.vtk",
		"../objects/jet/airbus_simplifiedBackCover_013.vtk",
	};

	std::vector<Tetmesh> meshes;
	for(auto s : names){
		auto m = load_msh(s ,false);
		//meshes.push_back();
		if(m.m_indices.size() > 0){
			meshes.push_back(m);
			//std::cout<<meshes[meshes.size()-1].m_indices.size() << std::endl;
		}

	}
	
	std::vector<Buffer> vertices;
	std::vector<Buffer> indices;
	for (auto& m:meshes){
		vertices.push_back( Buffer(graphicQueue, commandBuffer, m.m_vertices, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,VK_PIPELINE_STAGE_TRANSFER_BIT,VK_FORMAT_UNDEFINED));
		indices.push_back( Buffer(graphicQueue, commandBuffer, m.m_indices, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,VK_PIPELINE_STAGE_TRANSFER_BIT,VK_FORMAT_UNDEFINED));
	} 


	

	

	MeshShaderModule meshShader(root + "/shaders/jet/mesh.mesh.spv");
	FragmentShaderModule fragmentShader(root +"/shaders/jet/interval.frag.spv");
	

	VkExtent2D size = swapChain->size();
	
	

	Trackball trackball(vec3f({0.0f,-50.0f,0.0f}),vec3f({0.0f,0.0f,0.0f}),vec3f({0.0f,0.0f,-1.0f}));
	trackball.m_moveSpeed = 0.02f;

	float freq = 10.0f;
	trackball.m_rotationSpeed = 3.14159265f*2.0 / freq;
	
	
	auto view = trackball.getview();
	float nearPlane = 0.1f;
	auto perspective = PreparePerspectiveProjectionMatrix(float(size.width)/float(size.height),80.0f,nearPlane,1000.0f);
	perspective = transpose(perspective);

	std::vector<UniformBuffer> viewprojs(meshes.size());
	
	
	for(int i = 0; i< meshes.size(); i++){
		auto model = Identity();
		model = PrepareRotationMatrix(90,vec3f({1,0,0})) * PrepareRotationMatrix(180,vec3f({0,0,1}));
		viewprojs[i].addVariable("model", transpose(model));
		viewprojs[i].addVariable("view", transpose(view));

		
		
		viewprojs[i].addVariable("perpective", perspective );
		viewprojs[i].addVariable("nearPlane", nearPlane );

		viewprojs[i].end();
	}

	std::vector<vec3f> pos;
	std::srand(42);
	for(int i = 0; i< meshes.size(); i++){
		float x = 0.0f;
		float y = (float(std::rand())/float(RAND_MAX)) * 2.0 - 1.0;
		float z = (float(std::rand())/float(RAND_MAX)) * 2.0 - 1.0;

		pos.push_back(normalize(vec3f({x,y,z}))*20.0f);
	}

	vec2u res = vec2u({3840,2160});

	

	std::vector<DescriptorSet> sets(meshes.size());
	for (int i = 0; i < sets.size(); i++){
		sets[i].addUniformBuffer(viewprojs[i], VK_SHADER_STAGE_MESH_BIT_EXT | VK_SHADER_STAGE_FRAGMENT_BIT, 0);
		sets[i].addBuffer(vertices[i], VK_SHADER_STAGE_MESH_BIT_EXT , 1);
		sets[i].addBuffer(indices[i], VK_SHADER_STAGE_MESH_BIT_EXT , 2);
		sets[i].compile();
	}

	GraphicPipeline graphicPipeline(
		vec3f({ 0,0,0 }), 
		vec3f({ float(res[0]),float(res[1]),1.0f }), 
		vec2f({ 0,0 }), 
		vec2f({ float(res[0]),float(res[1]) })
	);

	std::vector<vec4f> colors(meshes.size());
	for(int i = 0; i< meshes.size(); i++){
		colors[i]= vec4f({0,0,0,0.1});
	}

	colors[1] = vec4f({22.0f/255.0f,0,1.0,0.403});
	colors[2] = vec4f({1.0f,0,1.0,0.696});
	colors[7] = vec4f({1.0f,0.0f,0.0f,0.659});
	colors[8] = vec4f({55.0f/255.0f,1.0f,0.0f,0.403f});

	graphicPipeline.setMeshModule(meshShader);
	graphicPipeline.setFragmentModule(fragmentShader);
	graphicPipeline.setDescriptorLayout(sets[0].getLayout());
	//graphicPipeline.setPolygonMode(VK_POLYGON_MODE_LINE);
	graphicPipeline.setLineWidth(3.0f);
	graphicPipeline.setCullMode(VK_CULL_MODE_NONE);
	graphicPipeline.setVerticesInfo({},{},VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);
	

	std::shared_ptr<PushConstant> constant = std::make_shared<PushConstant>();
	constant->addVariable("color", colors[0]);

	VkPushConstantRange range;
	range.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
	range.offset = 0;
	range.size = sizeof(colors[0]);
	graphicPipeline.setPushContantInfo({range});

	RenderPass renderPass;

	SubPassAttachments SA;
	//SA.setDepthFormat(VK_FORMAT_D16_UNORM);
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

	

	vec4f color;
	vec4f lightColor;
	color = vec4f({255/255.0,51/255.0,51/255.0f,11.0f});
	vec4f reflection = vec4f({1,1,1,0.25});

	int step = 0.0;

	UniformBuffer ub;
	ub.addVariable("color",color);
	ub.addVariable("proj",perspective);

	ub.addVariable("camera",transpose(view));
	ub.addVariable("camPos", trackball.m_pos);
	ub.addVariable("padding", 1.0f);
	ub.addVariable("res", vec2u({size.width,size.height}));
	ub.addVariable("resFirstPass", res);
	ub.addVariable("reflection", reflection);
	ub.addVariable("State", step);
	ub.end();

    DescriptorSet postProcessDescriptor;
	postProcessDescriptor.addFrameBuffer(frameBuffer,VK_SHADER_STAGE_FRAGMENT_BIT,0);
	postProcessDescriptor.addUniformBuffer(ub,VK_SHADER_STAGE_FRAGMENT_BIT,1);
    postProcessDescriptor.compile();


	

    GraphicPipeline postProcessPipeline(
		vec3f({ 0,0,0 }), 
		vec3f({ float(size.width),float(size.height),1.0f }), 
		vec2f({ 0,0 }), 
		vec2f({ float(size.width),float(size.height) })
	);

    VertexShaderModule possProcessVert(root + "/shaders/jet/postProcess.vert.spv");
	FragmentShaderModule possProcessFrag(root + "/shaders/jet/postProcess.frag.spv");
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

	trackball.rotate(vec2f({1.75,0.0f}));
	trackball.move(vec3f({0.0f,-10.0f,0.0f}));
	float dt = 0.0;
	float elapsed = freq*0.0f;
	float elapsed2 = 0.0f;
    float cost = 0;

	bool rotate = false;
	bool pressed = false;

	float avgCost = 0.0;
	

	bool pause = true;
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

		for(int i = 0; i< meshes.size(); i ++){
			std::string name = "Color "+std::to_string(i) ;
			ImGui::ColorEdit3(name.data(), &colors[i].data[0]);
			name = "Opacity "+std::to_string(i);
			ImGui::SliderFloat(name.data(), &colors[i].data[3],0,10.0);
		}

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

			state = glfwGetKey(window, GLFW_KEY_SPACE);
    		if (state == GLFW_PRESS)
    		{
				if( pressed != state){
					//std::cout << state << std::endl;
      				rotate = !rotate;
					pressed = true;
				}
    		}
			else{
				pressed = false;
			}
			//std::cout << pressed << std::endl;

			if(rotate){
				//trackball.rotate(vec2f({dt,0.0f}));
				//trackball.move(vec3f({0.0f,0.0f,-dt*10.0f}));
			}
			

			state = glfwGetKey(window, GLFW_KEY_P);
			if(state == GLFW_PRESS){
				pause = true;
			}
			state = glfwGetKey(window, GLFW_KEY_O);
			if(state == GLFW_PRESS){
				pause = false;
			}

			if(scrolled){
				scrolled = false;
				trackball.zoom(scroll);
			}
		
		}
		
		
		view = trackball.getview();

		float tmp = (sinf((elapsed)*2.0f - 3.14159265f/2.0)+1);
		//tmp = 1.5;

		for(int i = 0; i< meshes.size(); i++){
			viewprojs[i].setVariable("view",transpose(view));
			mat4f model = PrepareRotationMatrix(90,vec3f({1,0,0})) * PrepareRotationMatrix(180,vec3f({0,0,1})) * PrepareTranslationMatrix( 0.0, pos[i][1] * tmp, pos[i][2] * tmp);
			viewprojs[i].setVariable("model",transpose(model));
		}
    	mat4f rotation =  PrepareRotationMatrix(elapsed2*100.0f, vec3f({0.0f,0.0f,1.0f}))*PrepareRotationMatrix(90,vec3f({1,0,0})) * PrepareRotationMatrix(180,vec3f({0,0,1}));
		viewprojs[7].setVariable("model",transpose(rotation));

		ub.setVariable("camera",transpose(view));
		ub.setVariable("camPos",trackball.m_pos);

		ub.setVariable("color",color);
		ub.setVariable("reflection",reflection);
		ub.setVariable("State", step);

		

		commandBuffer.beginRecord();

		for(int i = 0; i< meshes.size(); i++){
			viewprojs[i].update(commandBuffer);
		}
		ub.update(commandBuffer);
		renderPass.begin(commandBuffer, 
			frameBuffer, 
			vec2u({ 0,0 }), 
			vec2u({ uint32_t (res[0]), uint32_t (res[1]) }), 
			{ { 0.0f, 0.0f, 0.0f, 1.0f } });


		
		graphicPipeline.bindPipeline(commandBuffer);

		for (int i = 0; i < meshes.size(); i++){
			graphicPipeline.bindDescriptorSet(commandBuffer, sets[i]);
			constant->setVariable("color",colors[i]);
			constant->push(commandBuffer,graphicPipeline.getPipelineLayout(),range);
			drawMeshTasks(commandBuffer, meshes[i].m_indices.size() ,1,1);
		}
		//drawMeshTasks(commandBuffer, 10 ,1,1);
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

		//std::cout<<cost<<std::endl; 
		
		dt = cost/1000.0f;
		elapsed2 += dt;
		if(!pause){
			elapsed += dt;
		}
		
		step = int(elapsed/freq);

		if(step> 2){
			//dt = dt / 2.0;
		}

		avgCost = avgCost*0.99 + cost * 0.01;
		//std::cout<<avgCost<<std::endl; 
	}
	device->waitForAllCommands();
}