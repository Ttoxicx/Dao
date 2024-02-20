#include <iostream>
#include <assimp/Importer.hpp>
#include <GLFW/glfw3.h>
#include <vulkan/vulkan.h>

int main() {
	auto importer = Assimp::Importer();
	try {
		importer.ReadFile("/////////////////", 0);
	}
	catch (...) {
		std::cout << "exception occur" << std::endl;
	}

	auto res = importer.GetScene();
	if (res == nullptr) {
		std::cout << "invalide scene" << std::endl;
	}

	glfwInit();
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

	GLFWwindow* window = glfwCreateWindow(800, 600, "DaoEditor", NULL, NULL);
	if (!window) {
		std::cout << "Window Create Failed" << std::endl;
		return -1;
	}

	while (!glfwWindowShouldClose(window)) {
		glfwPollEvents();
	}

	glfwTerminate();
	return 0;
}