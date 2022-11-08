#pragma once

#include "VkImage.h"
#include "VkBuffers.h"
#include "VkRenderPass.h"
#include "../../Math/Transform.h"
#include "VkFramebufferAttachment.h"
#include "../../GDefinitions.h"
#include <vector>

namespace vgl
{
	namespace vk
	{
		class GraphicsContext
		{
			public:
				GraphicsContext() : m_DShadowMapRenderPass(RenderPassType::Graphics) {}

				void init();

				void createSkyboxPipelines(RenderPass& p_RenderPass);
				// Pass GBuffer RenderPass
				void createShapesPipelines(RenderPass& p_RenderPass);


				ImageCube	generateAtmosphericScatteringCubeMap(Vector2i p_Size, AtmosphericScatteringInfo& p_Info);
				ImageCube	getCubeMapFromHDR(Image& p_HDRImage, Vector2i p_Size);
				ImageCube	generateIrradianceMap(ImageCube& p_CubeMapImage, Vector2i p_Size);
				ImageCube	generatePreFilteredMaps(ImageCube& p_CubeMapImage, Vector2i p_InitialSize);
				//static Image	generateBRDFLut(Vector2f p_Size);

				const VertexArray& getRecVao();
				VertexBuffer& getRecVertexBuffer();
				IndexBuffer& getRecIndexBuffer();

			private:
				g_Pipeline&				getAtmosphericScatteringPipeline();
				g_Pipeline&				getSkyBoxPipeline();
				Shader&					getSkyBoxShader();
				Shader&					getAtmosphericScatteringShader();
				Shader&					getDShadowMapShader();
				Shader&					getDShadowMapAlbedoShader();
				g_Pipeline&				getDShadowMapPipeline();
				g_Pipeline&				getDShadowMapAlbedoPipeline();
				RenderPass&				getDShadowMapRenderPass();
				VkDescriptorSetLayout&	getDShadowMapDescriptorSetLayout();
				VkDescriptorSetLayout&	getDShadowMapAlbedoDescriptorSetLayout();

			private:
				friend class Renderer;
				friend struct D_ShadowMap;

				g_Pipeline m_TextBoxPipeline;
				g_Pipeline m_Shape2DPipeline;
				g_Pipeline m_Circle2DPipeline;
				g_Pipeline m_ParticlePipeline;

				Shader m_TextBoxShader;
				Shader m_Shape2DShader;
				Shader m_Circle2DShader;
				Shader m_ParticleShader;

				Shader m_SkyBoxShader;
				g_Pipeline m_SkyBoxPipeline;

				Shader m_AtmosphericScatteringShader;
				g_Pipeline m_AtmosphericScatteringPipeline;

				g_Pipeline m_DShadowMapPipeline;
				RenderPass m_DShadowMapRenderPass;
				g_Pipeline m_DShadowMapAlbedoPipeline;

				Shader m_DShadowMapShader;
				Shader m_DShadowMapAlbedoShader;
				VkDescriptorSetLayout m_DShadowMapDescriptorSetLayout;
				VkDescriptorSetLayout m_DShadowMapAlbedoDescriptorSetLayout;

				::std::vector<Vector2f> m_RecRawVertices;
				VertexBuffer m_RecVertices;

				::std::vector<uint32_t> m_RecRawIndices;
				IndexBuffer m_RecIndices;

				BufferLayout m_RecLayout;

				VertexArray m_RecVao;
		};

		class GraphicsContextSingleton
		{
			public:
				inline static GraphicsContext& getInstance()
				{
					static GraphicsContext instance;
					return instance;
				}
			public:
				GraphicsContextSingleton(GraphicsContextSingleton const&) = delete;
				GraphicsContextSingleton(GraphicsContextSingleton&&) = delete;
				void operator=(GraphicsContextSingleton const&) = delete;
				void operator=(GraphicsContextSingleton&&) = delete;
		};
	}
}