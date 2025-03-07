#pragma once

#ifdef VGL_RENDER_API_VULKAN
#include "../Platform/Vulkan/VkDescriptor.h"
#include "../Platform/Vulkan/VkBuffers.h"

namespace vgl
{
	namespace vk {
		class Window;
		class Renderer;
		class Shader;
		class IndexBuffer;
		class VertexBuffer;
		class VertexArray;
		class RenderPipelineInfo;
		class FramebufferAttachment;
		class Image;
		class ImageCube;
		class ImageLoader;
		class ImGuiContext;
		class DShadowMap;
		class PShadowMap;
	}

	typedef vk::Window Window;
	typedef vk::Renderer Renderer;
	typedef vk::Shader Shader;
	template<typename ImageType> using SamplerData = vk::SamplerDescriptorData<ImageType>;
	typedef vk::Descriptor ShaderDescriptor;
	typedef vk::DescriptorSetInfo ShaderDescriptorInfo;
	typedef vk::UniformBuffer UniformBuffer;
	typedef vk::StorageBuffer StorageBuffer;
	typedef vk::IndexBuffer IndexBuffer;
	typedef vk::VertexBuffer VertexBuffer;
	typedef vk::VertexArray VertexArray;
	typedef vk::RenderPipelineInfo RenderPipelineInfo;
	typedef vk::FramebufferAttachment FramebufferAttachment;
	typedef vk::Image Image;
	typedef vk::ImageCube ImageCube;
	typedef vk::ImageLoader ImageLoader;
	typedef vk::ImGuiContext ImGuiContext;
	typedef vk::DShadowMap DShadowMap;
	typedef vk::PShadowMap PShadowMap;
}

#elif defined VGL_RENDER_API_OPENGL
#include "../Platform/OpenGL/GlWindow.h"
#include "../Platform/OpenGL/GlRenderer.h"
namespace vgl
{
	class Window : public gl::Window
	{
	public:
		Window() : gl::Window() {};
		Window(const Vector2i p_Size, const char* p_Title, const int p_MSAASamples) : gl::Window(p_Size, p_Title, p_MSAASamples) {};
	};
	class Renderer : public gl::Renderer
	{
	public:
		Renderer() : gl::Renderer() {}
		Renderer(Window* p_Window) : gl::Renderer(p_Window) {}
		Renderer(Window& p_Window) : gl::Renderer(p_Window) {}
		Renderer(Window& p_Window, Vector2i p_RenderResolution) : gl::Renderer(p_Window) {}
	};
	//class Image : public vk::Image {};
	//class Window : public vk::Window {};
	//class ImageLoader : public vk::ImageLoader {};
}
#endif