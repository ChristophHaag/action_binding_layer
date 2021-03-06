#include "ui.h"
#include "util.h"

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

#define BUFLEN 1024

bool
createModifiedBindings(XrInstance instance,
                       XrInstanceCreateInfo *instanceInfo,

                       std::map<XrActionSet, XrActionSetCreateInfo> *actionSetInfos,
                       std::map<XrAction, XrActionCreateInfo> *actionInfos,
                       std::map<XrAction, XrActionSpaceCreateInfo> *actionSpaceInfos,
                       std::map<XrPath, std::string> *paths,
                       std::map<XrPath, XrInteractionProfileSuggestedBinding *> *bindings,
                       std::map<XrPath, XrInteractionProfileSuggestedBinding *> *modifiedBindings)
{
	std::map<const XrPath *, char *> modifications;

	for (const auto &profileBindings : *bindings) {
		XrInteractionProfileSuggestedBinding *b = profileBindings.second;
		for (uint32_t i = 0; i < b->countSuggestedBindings; i++) {
			const XrPath *binding_ptr = &b->suggestedBindings[i].binding;
			char *buf = new char[BUFLEN];
			snprintf(buf, BUFLEN, "%s", (*paths)[b->suggestedBindings[i].binding].c_str());
			modifications[binding_ptr] = buf;
		}
	}

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
		ImGui::Text("Application: %s v%d", instanceInfo->applicationInfo.applicationName,
		            instanceInfo->applicationInfo.applicationVersion);
		ImGui::Text("Engine: %s v%d", instanceInfo->applicationInfo.engineName,
		            instanceInfo->applicationInfo.engineVersion);
		if (ImGui::Button("Done")) {
			break;
		}
		ImGui::End();

		for (const auto &profileBindings : *bindings) {
			XrInteractionProfileSuggestedBinding *b = profileBindings.second;

			ImGui::Begin((*paths)[b->interactionProfile].c_str());

			ImGui::Separator();

			for (uint32_t i = 0; i < b->countSuggestedBindings; i++) {
				XrAction action = b->suggestedBindings[i].action;
				std::string s = (*actionInfos)[action].actionName;
				s += " (";
				s += actionTypesStr[(*actionInfos)[action].actionType];
				s += ")";
				ImGui::Text(s.c_str());

				const XrPath *binding_ptr = &b->suggestedBindings[i].binding;
				char *buf = modifications[binding_ptr];

				std::string label = std::to_string((uint64_t)binding_ptr);

				ImGui::InputText(label.c_str(), buf, BUFLEN);

				ImGui::Separator();
			}
			ImGui::End();
		}

		ImGui::Render();

		glClear(GL_COLOR_BUFFER_BIT);

		ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
		glfwSwapBuffers(window);
	}

	for (const auto &profileBindings : *bindings) {
		XrInteractionProfileSuggestedBinding *b = profileBindings.second;
		(*modifiedBindings)[profileBindings.first] = deepCopyBinding(b);

		XrInteractionProfileSuggestedBinding *modified_b = (*modifiedBindings)[profileBindings.first];

		XrActionSuggestedBinding *newBindings = new XrActionSuggestedBinding[b->countSuggestedBindings];
		for (uint32_t i = 0; i < b->countSuggestedBindings; i++) {
			const XrPath *binding_ptr = &b->suggestedBindings[i].binding;

			char *modifiedBinding = modifications[binding_ptr];

			bool found = false;

			XrPath key = 0;
			for (auto &path : (*paths)) {
				if (path.second == modifiedBinding) {
					key = path.first;
					found = true;
					break; // to stop searching
				}
			}

			newBindings[i] = b->suggestedBindings[i];
			if (found) {
				newBindings[i].binding = key;
				std::cout << _layerName << ": "
				          << "Using existing path for binding " << modifiedBinding << std::endl;
			} else {
				std::cout << _layerName << ": "
				          << "Creating new path for binding " << modifiedBinding << std::endl;

				XrPath p;
				XrResult result = _nextXrStringToPath(instance, modifiedBinding, &p);
				if (XR_SUCCEEDED(result)) {
					std::cout << _layerName << ": created path " << p << " for " << modifiedBinding
					          << std::endl;
					(*paths)[p] = std::string(modifiedBinding);


					newBindings[i].binding = p;
				} else {
					std::cout << _layerName << ": xrStringToPath failed for" << modifiedBinding
					          << std::endl;
				}
			}

			modified_b->suggestedBindings = newBindings;

			delete[] modifications[binding_ptr];
		}
	}

	teardown(window);
	return true;
}
