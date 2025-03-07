#pragma once

#include "VkContext.h"
#include "../Definitions.h"
#include "VkShader.h"

namespace vgl
{
	namespace vk
	{
		struct AttachmentInfo
		{
			VkFormat p_Format;
			uint8_t p_SampleCount;

			LoadOp p_LoadOp; // VkAttachmentLoadOp
			StoreOp p_StoreOp; // VkAttachmentStoreOp

			LoadOp p_StencilLoadOp;
			StoreOp p_StencilStoreOp;

			Layout p_InitialLayout; // VkAttachmentDescription
			Layout p_FinalLayout;

			AttachmentType p_AttachmentType;
		};

		// Pre-declarations
		class FramebufferAttachment;
		class Framebuffer;
		class g_Pipeline;
		class CommandBuffer;
		class Renderer;
		class GraphicsContext;
		class ImGuiContext;

		class RenderPass
		{
			public:
				RenderPass(RenderPassType p_Type);
				~RenderPass();

				void addAttachment(AttachmentInfo& p_AttachmentInfo);

				bool create(void* p_Next = nullptr);
				void destroy();

				const bool& isValid() { return m_IsValid; }

				std::vector<VkSubpassDependency> m_Dependencies;

				std::vector<AttachmentInfo>& getAttachmentInfo() { return m_AttachmentInfo; }

			protected:
			private:
				friend class FramebufferAttachment;
				friend class Framebuffer;
				friend class g_Pipeline;
				friend class CommandBuffer;
				friend class Renderer;
				friend class GraphicsContext;
				friend class ImGuiContext;

				Context* m_ContextPtr;

				RenderPassType m_Type;
				bool m_IsValid = false;
				uint32_t m_ClearCount = 0;

				VkRenderPass m_RenderPass;
				VkSubpassDescription m_Subpass;


				std::vector<VkAttachmentDescription>	m_Attachments;
				std::vector<VkAttachmentReference>		m_AttachmentRefs;
				std::vector<AttachmentInfo>				m_AttachmentInfo;

				std::vector<VkAttachmentReference> m_ColorAttachments;
				std::vector<VkAttachmentReference> m_ResolveAttachments;
				std::vector<VkAttachmentReference> m_InputAttachments;
			
		};

		struct g_PipelineInfo
		{
			g_PipelineInfo() :
				p_PolygonMode(PolygonMode::Fill),
				p_CullMode(CullMode::None),
				p_FrontFace(FrontFace::Default),
				p_IATopology(IATopoogy::TriList),
				p_Viewport({ 0, 0 }, { 0, 0 }),
				p_Scissor({ 0, 0 }, { 0, 0 }),
				p_RenderPass(nullptr),
				p_Shader(nullptr),
				p_SampleRateShading(true),
				p_AlphaBlending(false),
				p_DepthBuffering(true),
				p_DepthBias(false),
				p_UsePushConstants(false),
				p_PushConstantOffset(0),
				p_PushConstantSize(0),
				p_PushConstantShaderStage(0),
				p_DepthBiasConstantFactor(0.0f),
				p_DepthBiasClamp(0.0f),
				p_DepthBiasSlopeFactor(0.0f),
				p_MSAASamples(1)
			{ };

			bool					p_SampleRateShading;
			bool					p_AlphaBlending;
			bool					p_DepthBuffering;
			bool					p_DepthBias;
			PolygonMode				p_PolygonMode;
			CullMode				p_CullMode;
			FrontFace				p_FrontFace;
			IATopoogy				p_IATopology;
			Viewport				p_Viewport;
			Scissor					p_Scissor;
			Shader* p_Shader;
			RenderPass* p_RenderPass;
			uint8_t					p_MSAASamples;

			bool					p_UsePushConstants;
			uint32_t				p_PushConstantOffset;
			uint32_t				p_PushConstantSize;
			VkShaderStageFlags		p_PushConstantShaderStage;

			float p_DepthBiasConstantFactor;
			float p_DepthBiasClamp;
			float p_DepthBiasSlopeFactor;

			std::vector<VkDescriptorSetLayout>								p_DescriptorSetLayouts;
			std::vector<VkVertexInputBindingDescription>					p_BindingDescription;
			std::vector<std::vector<VkVertexInputAttributeDescription>> 	p_AttributeDescription;
		};

		// Compute pipeline
		class ComputePipeline
		{
			public:
				ComputePipeline(){}

				bool create(g_PipelineInfo p_PipelineInfo)
				{
					/*m_PipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
					m_PipelineLayoutInfo.setLayoutCount = 1;
					m_PipelineLayoutInfo.pSetLayouts = &computeDescriptorSetLayout;

					VkResult result = vkCreatePipelineLayout(m_ContextPtr->m_Device, &m_PipelineLayoutInfo, nullptr, &m_PipelineLayout);
					VGL_INTERNAL_ASSERT_ERROR(result == VK_SUCCESS, "[vk::g_Pipeline]Failed to create pipeline layout, VkResult: %i", (uint64_t)result);

					if (result != VK_SUCCESS) return false;

					m_PipelineInfo.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
					m_PipelineInfo.layout = m_PipelineLayout;
					m_PipelineInfo.stage = computeShaderStageInfo;

					VGL_INTERNAL_ASSERT_ERROR(p_PipelineInfo.p_RenderPass != nullptr, "[vk::ComputePipeline]Failed to create compute pipeline, p_RenderPass == nullptr");
					if (p_PipelineInfo.p_RenderPass == nullptr) return false;
					VkResult result = vkCreateComputePipelines(m_ContextPtr->m_Device, VK_NULL_HANDLE, 1, &m_PipelineInfo, nullptr, &m_ComputePipeline);
					VGL_INTERNAL_ASSERT_ERROR(result == VK_SUCCESS, "[vk::g_Pipeline]Failed to create graphics pipeline, VkResult: %i", (uint64_t)result);
					if (result != VK_SUCCESS) return false;

					VGL_INTERNAL_TRACE("[vk::g_Pipeline]Succesfully created graphics pipeline");

					m_IsValid = true;

					return true;*/
				}
				void destroy()
				{

				}

				const bool& isValid() { return m_IsValid; }

			private:
				friend class Renderer;

				Context* m_ContextPtr;

				bool m_IsValid = false;
				
				VkPipelineLayout m_PipelineLayout;
				VkPipeline m_ComputePipeline;
				VkComputePipelineCreateInfo m_PipelineInfo = {};
				VkPipelineLayoutCreateInfo m_PipelineLayoutInfo = {};
		};

		// Graphics Pipeline
		class g_Pipeline
		{
		public:
			g_Pipeline();
			~g_Pipeline();

			bool create(g_PipelineInfo p_PipelineInfo);
			void destroy();

			const bool& isValid() { return m_IsValid; }

		private:
			friend class CommandBuffer;
			friend class Renderer;
			friend class FramebufferAttachment;
			friend class GraphicsContext;
			friend class ImGuiContext;

			Context* m_ContextPtr;

			g_PipelineInfo m_Info;

			PipelineBindPoint m_BindPoint;

			std::vector<VkDynamicState> m_DynamicStates;

			VkViewport	m_Viewport;
			VkRect2D	m_Scissor;

			VkPipeline										m_Pipeline;
			VkPipelineLayout								m_PipelineLayout;
			VkDescriptorSetLayout							m_DescriptorSetLayout;
			VkVertexInputBindingDescription					m_BindingDescription;
			std::vector<VkVertexInputAttributeDescription>	m_AttributeDescription;

			// Info for graphics pipeline creation
			VkPipelineInputAssemblyStateCreateInfo	m_InputAssemblyInfo;
			VkPipelineViewportStateCreateInfo		m_ViewportInfo;
			VkPipelineRasterizationStateCreateInfo	m_RasterizerInfo;
			VkPipelineMultisampleStateCreateInfo	m_MultiSampling;
			std::vector<VkPipelineColorBlendAttachmentState>		m_ColorBlendAttachments;
			VkPipelineColorBlendStateCreateInfo		m_ColorBlending;
			VkPipelineDynamicStateCreateInfo		m_DynamicState;

			VkPipelineDepthStencilStateCreateInfo	m_DepthInfo;
			VkPipelineLayoutCreateInfo				m_PipelineLayoutInfo;

			VkGraphicsPipelineCreateInfo			m_PipelineInfo;
			VkPipelineVertexInputStateCreateInfo	m_VertexInputInfo;
		
			bool m_IsValid = false;
		};

		struct RenderPipelineInfo
		{
			bool p_CreateGraphicsPipeline = false;
			RenderPass* p_RenderPass = nullptr;
			g_Pipeline* p_Pipeline = nullptr; // Use another pipeline
			g_PipelineInfo p_GraphicsPipelineInfo;
		};
	}
}