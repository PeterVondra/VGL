#pragma once

#include "VkContext.h"
#include "../Definitions.h"

namespace vgl
{
	namespace vk
	{
		// Pre-declarations
		class Renderer;
		class g_Pipeline;
		class RenderPass;

		class Shader
		{
			public:

				Shader();

				Shader(std::string p_ComputeShaderPath);
				Shader(std::string p_VertexShaderPath, std::string p_FragmentShaderPath, std::string p_GeometryShaderPath);
				Shader(std::string p_VertexShaderPath, std::string p_FragmentShaderPath);
				~Shader();

				void destroy();

				void setShader(std::string p_ComputeShaderPath);
				void setShader(std::string p_VertexShaderPath, std::string p_FragmentShaderPath, std::string p_GeometryShaderPath);
				void setShader(std::string p_VertexShaderPath, std::string p_FragmentShaderPath);

				// Element 0 is vertex shader
				// Element 1 is fragment shader
				// Element 2 is geometry shader
				void compile(std::vector<std::string> p_Shader);
				void compile(std::string p_Shader, glslType p_Type);

			protected:
			private:
				friend class vk::Renderer;
				friend class vk::g_Pipeline;
				friend class vk::RenderPass;

				Context* m_ContextPtr;

				std::vector<unsigned int> glslToSpirv(std::string& p_Code, glslType p_Type);

				// Shader modules
				VkShaderModule    m_ComputeShaderModule;
				VkShaderModule    m_VertShaderModule;
				VkShaderModule    m_GeometryShaderModule;
				VkShaderModule    m_FragShaderModule;

				VkPipelineShaderStageCreateInfo    m_ComputeShaderStageInfo;
				VkPipelineShaderStageCreateInfo    m_VertShaderStageInfo;
				VkPipelineShaderStageCreateInfo    m_GeometryShaderStageInfo;
				VkPipelineShaderStageCreateInfo    m_FragShaderStageInfo;

				VkShaderModule createShaderModule(const std::vector<uint32_t>& p_Code);
				VkShaderModule createShaderModule(const std::vector<char>& p_Code);
				std::vector<char> readFile(const std::string& p_Filename);

				bool m_Valid = false;
			
		};
	}
}
