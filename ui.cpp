#include "ui.h"

#include <iostream>

#include "external/imgui/imgui.h"
#include "external/imgui/imgui_impl_glfw.h"
#include "external/imgui/imgui_impl_opengl3.h"

#include "glad/glad.h"

#include <GLFW/glfw3.h>

// imgui setup code from https://decovar.dev/blog/2019/08/04/glfw-dear-imgui/

std::string programName = "GLFW window";
int windowWidth = 1200,
windowHeight = 800;
float backgroundR = 0.1f,
backgroundG = 0.3f,
backgroundB = 0.2f;

GLFWwindow *window = NULL;

static void framebuffer_size_callback(GLFWwindow *window, int width, int height)
{
	glViewport(0, 0, width, height);
}

static void teardown(GLFWwindow *window)
{
	if (window != NULL) { glfwDestroyWindow(window); }
	glfwTerminate();
}

static int
initGui()
{
	if (!glfwInit())
	{
		std::cerr << "[ERROR] Couldn't initialize GLFW\n";
		return -1;
	}
	else
	{
		std::cout << "[INFO] GLFW initialized\n";
	}

	// setup GLFW window

	glfwWindowHint(GLFW_DOUBLEBUFFER , 1);
	glfwWindowHint(GLFW_DEPTH_BITS, 24);
	glfwWindowHint(GLFW_STENCIL_BITS, 8);

	glfwWindowHint(
		GLFW_OPENGL_PROFILE,
		GLFW_OPENGL_CORE_PROFILE
	);

	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);

	window = glfwCreateWindow(
		windowWidth,
		windowHeight,
		programName.c_str(), NULL, NULL
	);
	if (!window)
	{
		std::cerr << "[ERROR] Couldn't create a GLFW window\n";
		teardown(NULL);
		return -1;
	}
	// watch window resizing
	glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
	glfwMakeContextCurrent(window);
	// VSync
	glfwSwapInterval(1);

	if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
	{
		std::cerr << "[ERROR] Couldn't initialize GLAD" << std::endl;
		teardown(window);
		return -1;
	}
	else
	{
		std::cout << "[INFO] GLAD initialized\n";
	}

	std::cout << "[INFO] OpenGL from glad "
	<< GLVersion.major << "." << GLVersion.minor
	<< std::endl;

	int actualWindowWidth, actualWindowHeight;
	glfwGetWindowSize(window, &actualWindowWidth, &actualWindowHeight);
	glViewport(0, 0, actualWindowWidth, actualWindowHeight);

	glClearColor(backgroundR, backgroundG, backgroundB, 1.0f);


	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGui::StyleColorsDark();

	// Setup Platform/Renderer backends
	ImGui_ImplGlfw_InitForOpenGL(window, true);
	ImGui_ImplOpenGL3_Init("#version 130");

	ImGui::GetIO().Fonts->AddFontDefault();
	unsigned char* tex_pixels = NULL;
	int tex_width, tex_height;
	ImGui::GetIO().Fonts->GetTexDataAsRGBA32(&tex_pixels, &tex_width, &tex_height);
	ImGui::GetIO().DisplaySize.x = windowWidth;
	ImGui::GetIO().DisplaySize.y = windowHeight;

	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	return 0;
}


bool
createModifiedBindings(XrInstance instance,
                       XrInstanceCreateInfo *instanceInfo,

                       std::map<XrActionSet, XrActionSetCreateInfo> actionSetInfos,
                       std::map<XrAction, XrActionCreateInfo> actionInfos,
                       std::map<XrAction, XrActionSpaceCreateInfo> actionSpaceInfos,
                       std::map<XrPath, std::string> paths,
                       std::map<XrPath, XrInteractionProfileSuggestedBinding *> *bindings,
                       std::map<XrPath, XrInteractionProfileSuggestedBinding *> *modifiedBindings)
{
	initGui();

	// --- rendering loop
	while (!glfwWindowShouldClose(window))
	{
		glfwPollEvents();

		ImGui_ImplOpenGL3_NewFrame();
		ImGui_ImplGlfw_NewFrame();
		ImGui::NewFrame();

		ImGui::Begin("Binding editor!");
		ImGui::Text("Modify the bindings that the application suggests to the runtime, then click done.");
		if (ImGui::Button("Done")) {
			break;
		}
		ImGui::End();

		for (const auto &profileBindings : *bindings) {
			XrInteractionProfileSuggestedBinding *b = profileBindings.second;

			ImGui::Begin(paths[b->interactionProfile].c_str());
			ImGui::Text("Sugggested by app");
			for (uint32_t i = 0; i < b->countSuggestedBindings; i++) {
				XrAction action = b->suggestedBindings[i].action;
				std::string s = std::string(actionInfos[action].actionName);
				s += " -> ";
				s += paths[b->suggestedBindings[i].binding];
				ImGui::Text(s.c_str());
			}
			ImGui::End();

			// TODO modify something
			(*modifiedBindings)[profileBindings.first] = b;
		}

		ImGui::Render();

		glClear(GL_COLOR_BUFFER_BIT);

		ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
		glfwSwapBuffers(window);
	}

	teardown(window);
	return true;
}
