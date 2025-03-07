#include "RenderPipelines.h"
#include "Platform/Definitions.h"
#include "Platform/Vulkan/VkGraphics_Internal.h"


namespace vgl
{
	RenderPipeline_Deferred::RenderPipeline_Deferred()
	{
	}
	RenderPipeline_Deferred::~RenderPipeline_Deferred()
	{
	}
	void RenderPipeline_Deferred::setup(Renderer* p_Renderer, Window* p_Window, Scene* p_Scene)
	{
		m_RendererPtr = p_Renderer;
		m_WindowPtr = p_Window;
		m_ScenePtr = p_Scene;


		//vk::GCS::getInstance().createShapesPipelines(*m_GBuffer.m_FramebufferAttachmentInfo.p_GraphicsPipelineInfo.p_RenderPass);

		m_SSAOShader.setShader("data/Shaders/SSAO/vert.spv", "data/Shaders/SSAO/frag.spv");
		m_SSAOBlurShader.setShader("data/Shaders/SSAO/Blur/vert.spv", "data/Shaders/SSAO/Blur/frag.spv");
		m_SSRShader.setShader("data/Shaders/SSR/vert.spv", "data/Shaders/SSR/frag.spv");
		m_HDRShader.setShader("data/Shaders/HDR/vert.spv", "data/Shaders/HDR/frag.spv");
		m_DOFShader.setShader("data/Shaders/BokehDOF/vert.spv", "data/Shaders/BokehDOF/frag.spv");
		m_FXAAShader.setShader("data/Shaders/FXAA/vert.spv", "data/Shaders/FXAA/frag.spv");
		m_DownsamplingShader.setShader("data/Shaders/Bloom/Downsampling/vert.spv", "data/Shaders/Bloom/Downsampling/frag.spv");
		m_UpsamplingShader.setShader("data/Shaders/Bloom/Upsampling/vert.spv", "data/Shaders/Bloom/Upsampling/frag.spv");
		m_PrefilterShader.setShader("data/Shaders/Bloom/Prefilter/vert.spv", "data/Shaders/Bloom/Prefilter/frag.spv");
		m_BloomShader.setShader("data/Shaders/Bloom/vert.spv", "data/Shaders/Bloom/frag.spv");
		m_VoxelShader.setShader("data/Shaders/VXGI/Voxelize/vert.spv", "data/Shaders/VXGI/Voxelize/frag.spv", "data/Shaders/VXGI/Voxelize/geom.spv");

		setupGBuffer();
		setupSSAOBuffers();
		setupLightPassBuffer();
		setupSSRBuffer();
		setupPostProcessBuffers();
		setupBloomBuffers();

		vk::GCS::getInstance().createSkyboxPipelines(*m_GBuffer.m_FramebufferAttachmentInfo.p_RenderPipelineInfo.p_RenderPass);
	}
	void RenderPipeline_Deferred::render(RenderInfo& p_RenderInfo)
	{
		for (auto& entity : m_ScenePtr->getEntities()) {
			// Render into directional shadow map
			auto directional_light = m_ScenePtr->getComponent<DirectionalLight3DComponent>(entity);
			auto shadow_map = m_ScenePtr->getComponent<DShadowMapComponent>(entity);
			if (shadow_map) {
				m_RendererPtr->beginRenderPass(p_RenderInfo, shadow_map->ShadowMap);
				shadow_map->ShadowMap.m_View = Matrix4f::lookAtRH(directional_light->Direction * 3000, { 0,0,0 }, { 0, 1, 0 });
				for (auto& entity : m_ScenePtr->getEntities()) {
					auto mesh = m_ScenePtr->getComponent<Mesh3DComponent>(entity);
					auto transform = m_ScenePtr->getComponent<Transform3DComponent>(entity);
					if (!mesh) continue;
					if (!transform) continue;
					if (mesh->mesh)
						m_RendererPtr->submit(*mesh->mesh, transform->transform);
				}
				m_RendererPtr->endRenderPass();
			}
			// Render into omni shadow map
			auto pshadow_map = m_ScenePtr->getComponent<PShadowMapComponent>(entity);
			auto point_light = m_ScenePtr->getComponent<PointLight3DComponent>(entity);
			if (pshadow_map && point_light) {
				pshadow_map->ShadowMap.m_Position = &point_light->Transform.getPosition();
				m_RendererPtr->beginRenderPass(p_RenderInfo, pshadow_map->ShadowMap);
				for (auto& entity : m_ScenePtr->getEntities()) {
					auto mesh = m_ScenePtr->getComponent<Mesh3DComponent>(entity);
					auto transform = m_ScenePtr->getComponent<Transform3DComponent>(entity);
					if (!mesh) continue;
					if (!transform) continue;
					if (mesh->mesh)
						m_RendererPtr->submit(*mesh->mesh, transform->transform);
				}
				m_RendererPtr->endRenderPass();
			}
		}

		m_RendererPtr->beginRenderPass(p_RenderInfo, m_GBuffer);

		// Submit meshes to Geometry Buffer
		for (auto entity : m_ScenePtr->getEntities()) {
			auto skybox = m_ScenePtr->getComponent<SkyboxComponent>(entity);
			if (skybox) {
				if (skybox->_AtmosphericScattering)
					m_RendererPtr->submit(*skybox->skybox, skybox->_AtmosphericScatteringInfo);
			}
			auto mesh = m_ScenePtr->getComponent<Mesh3DComponent>(entity);
			auto transform = m_ScenePtr->getComponent<Transform3DComponent>(entity);
			if (!mesh) continue;
			if (!transform) continue;
			if (mesh->mesh != nullptr)
				m_RendererPtr->submit(*mesh->mesh, transform->transform);

		}
		m_RendererPtr->endRenderPass();

		// SSAO
		transferLightPassData();
		transferSSAOData();
		m_RendererPtr->beginRenderPass(p_RenderInfo, m_SSAOFramebuffer);
		m_RendererPtr->endRenderPass();
		
		m_RendererPtr->beginRenderPass(p_RenderInfo, m_SSAOBlurFramebuffer);
		m_RendererPtr->endRenderPass();

		// Light pass
		m_RendererPtr->beginRenderPass(p_RenderInfo, m_LightPassFramebuffer);
		m_RendererPtr->endRenderPass();

		transferSSRData();
		
		m_RendererPtr->beginRenderPass(p_RenderInfo, m_SSRFramebuffer);
		m_RendererPtr->endRenderPass();
		
		transferPostProcessData();
		m_RendererPtr->beginRenderPass(p_RenderInfo, m_FXAAFramebuffer);
		m_RendererPtr->endRenderPass();

		// Depth of Field
		m_RendererPtr->beginRenderPass(p_RenderInfo, m_DOFFramebuffer);
		m_RendererPtr->endRenderPass();
		
		// Prefilter for bloom
		m_RendererPtr->beginRenderPass(p_RenderInfo, m_PrefilterFramebufferAttachment);
		m_RendererPtr->endRenderPass();

		for (auto& framebuffer : m_DownsamplingFramebuffers) {
			m_RendererPtr->beginRenderPass(p_RenderInfo, framebuffer);
			m_RendererPtr->endRenderPass();
		}
		for (int32_t mip = m_UpsampleMipLevels - 1; mip > -1; mip--) {
			m_RendererPtr->beginRenderPass(p_RenderInfo, m_UpsamplingFramebuffers[mip]);
			m_RendererPtr->endRenderPass();
		}

		m_RendererPtr->beginRenderPass(p_RenderInfo, m_BloomFramebufferAttachment);
		m_RendererPtr->endRenderPass();

		for (auto& img : m_BloomFramebufferAttachment.getImageAttachments())
			m_RendererPtr->blitImage(img[0].getImage());
		
		m_RendererPtr->beginRenderPass(p_RenderInfo, m_HDRFramebuffer);
		m_RendererPtr->endRenderPass();
	}
	void RenderPipeline_Deferred::setupGBuffer()
	{
		/////////////////////////////////////////////////////////////////////////////////////////////
		//				Geometry Buffer
		/////////////////////////////////////////////////////////////////////////////////////////////
		m_GBuffer.m_FramebufferAttachmentInfo.p_Size = m_WindowPtr->getWindowSize();
		m_GBuffer.m_FramebufferAttachmentInfo.p_RenderPipelineInfo.p_CreateGraphicsPipeline = false;

		// RGBA = Position
		m_GBuffer.addAttachment(m_WindowPtr->getWindowSize(), ImageFormat::C16SF_4C, Layout::ShaderR);
		// RGBA = Normal
		m_GBuffer.addAttachment(m_WindowPtr->getWindowSize(), ImageFormat::C16SF_4C, Layout::ShaderR);
		// RGBA = Albedo
		m_GBuffer.addAttachment(m_WindowPtr->getWindowSize(), ImageFormat::C16SF_4C, Layout::ShaderR);
		// RGBA = (R = Metallic) (G = Roughness) (B = Ambient Occlussion) (A = Depth)
		m_GBuffer.addAttachment(m_WindowPtr->getWindowSize(), ImageFormat::C16SF_4C, Layout::ShaderR);
		// Depth
		m_GBuffer.addAttachment(m_WindowPtr->getWindowSize(), ImageFormat::D32SF, Layout::DepthR);
		m_GBuffer.create();
	}
	void RenderPipeline_Deferred::setupSSAOBuffers()
	{
		/////////////////////////////////////////////////////////////////////////////////////////////
		//				Screen-Space-Ambient-Occlusion
		/////////////////////////////////////////////////////////////////////////////////////////////
		for (uint32_t i = 0; i < Sample_Count; i++) {
			Vector4f sample(
				Math::getRandomNumber(0.0f, 1.0f) * 2.0f - 1.0f,
				Math::getRandomNumber(0.0f, 1.0f) * 2.0f - 1.0f,
				Math::getRandomNumber(0.0f, 1.0f), 0.0f
			);
			sample = Math::normalize(sample);
			sample = sample * Math::getRandomNumber(0.0f, 1.0f);
			float scale = (float)i / Sample_Count;
			scale = Math::lerp(0.1f, 1.0f, scale * scale);
			sample = sample * scale;
			SSAO_Kernel.push_back(sample);
		}

		for (uint32_t i = 0; i < 16; i++) {
			SSAO_Noise.emplace_back(
				Math::getRandomNumber(0.0f, 1.0f) * 2.0f - 1.0f,
				Math::getRandomNumber(0.0f, 1.0f) * 2.0f - 1.0f,
				0.0f, 0.0f
			);
		}

		SSAO_NoiseImage.create({ 4, 4 }, (unsigned char*)SSAO_Noise.data(), ColorSpace::RGB, Channels::RGBA, SamplerMode::Repeat, false, Filter::Nearest, Filter::Nearest);

		static vk::DescriptorSetInfo infoSSAO;
		infoSSAO.p_FragmentUniformBuffer = UniformBuffer(sizeof(Vector4f) * Sample_Count + 2 * sizeof(Matrix4f) + sizeof(Vector4f), 0);
		for (int32_t i = 0; i < m_GBuffer.getImageAttachments().size(); i++) {
			infoSSAO.addImage(&m_GBuffer.getImageAttachments()[i][0].getImage(), 1, i);
			infoSSAO.addImage(&m_GBuffer.getImageAttachments()[i][1].getImage(), 2, i);
		}
		infoSSAO.addImage(&SSAO_NoiseImage, 3);

		m_SSAOFramebuffer.getDescriptors().create(infoSSAO);
		m_SSAOFramebuffer.getDescriptors().copy(ShaderStage::FragmentBit, SSAO_Kernel.data(), SSAO_Kernel.size() * sizeof(Vector4f), 2 * sizeof(Vector4f) + sizeof(Matrix4f));

		// Create Post-Processing framebuffer
		m_SSAOFramebuffer.m_FramebufferAttachmentInfo.p_Size = m_WindowPtr->getWindowSize();
		m_SSAOFramebuffer.m_FramebufferAttachmentInfo.p_Shader = &m_SSAOShader;
		m_SSAOFramebuffer.addAttachment(m_WindowPtr->getWindowSize(), ImageFormat::C16SF_1C, Layout::ShaderR);
		m_SSAOFramebuffer.create();

		static ShaderDescriptorInfo infoSSAOBlur;
		for (int32_t i = 0; i < m_SSAOFramebuffer.getImageAttachments().size(); i++)
			infoSSAOBlur.addImage(&m_SSAOFramebuffer.getImageAttachments()[i][0].getImage(), 0, i);

		m_SSAOBlurFramebuffer.getDescriptors().create(infoSSAOBlur);
		m_SSAOBlurFramebuffer.m_FramebufferAttachmentInfo.p_Size = m_WindowPtr->getWindowSize();
		m_SSAOBlurFramebuffer.m_FramebufferAttachmentInfo.p_Shader = &m_SSAOBlurShader;
		m_SSAOBlurFramebuffer.addAttachment(m_WindowPtr->getWindowSize(), ImageFormat::C16SF_1C, Layout::ShaderR);
		m_SSAOBlurFramebuffer.create();
	}
	void RenderPipeline_Deferred::setupLightPassBuffer()
	{
		/////////////////////////////////////////////////////////////////////////////////////////////
		//				Lighting Pass
		/////////////////////////////////////////////////////////////////////////////////////////////
		// Create main color framebuffer
		ShaderInfo shader_info;
		shader_info.p_LightingType = (uint32_t)LightingType::PBR;

		{
			P_Light plight;
			for (auto& entity : m_ScenePtr->getEntities()) {
				auto directional_light = m_ScenePtr->getComponent<DirectionalLight3DComponent>(entity);
				auto point_light = m_ScenePtr->getComponent<PointLight3DComponent>(entity);
				if (directional_light) {
					shader_info.p_Effects |= (uint32_t)Effects::ShadowMapping;
					shader_info.p_DirectionalLight = true;
				}
				if (point_light) {
					shader_info.p_Effects |= (uint32_t)Effects::ShadowMapping;
					plight.m_ShadowMapID = point_light->ShadowMapID;
					shader_info.p_PointLights.push_back(plight);
				}
			}
		}

		m_LightPassFramebuffer.m_FramebufferAttachmentInfo.p_Size = m_WindowPtr->getWindowSize();
		m_LightPassFramebuffer.m_FramebufferAttachmentInfo.p_Shader = &m_GBufferLightPassShader;
		m_LightPassFramebuffer.m_FramebufferAttachmentInfo.p_PushConstantShaderStage = ShaderStage::FragmentBit;
		m_LightPassFramebuffer.m_FramebufferAttachmentInfo.p_PushConstantData = &volumetric_light_mie_scattering;
		m_LightPassFramebuffer.m_FramebufferAttachmentInfo.p_PushConstantSize = sizeof(float);
		// RGBA = Position, A = Depth
		m_LightPassFramebuffer.addAttachment(
			m_WindowPtr->getWindowSize(),
			ImageFormat::C16SF_4C,
			Layout::ShaderR,
			true, false, false,
			BorderColor::OpaqueBlack,
			SamplerMode::ClampToEdge
		);

		static ShaderDescriptorInfo infoLightPass;
		infoLightPass.p_FragmentUniformBuffer = UniformBuffer(3 * sizeof(Vector4f) + sizeof(Matrix4f) + sizeof(P_Light) * 500, 0);
		for (int32_t i = 0; i < m_GBuffer.getImageAttachments().size(); i++) {
			infoLightPass.addImage(&m_GBuffer.getImageAttachments()[i][0].getImage(), 1, i);
			infoLightPass.addImage(&m_GBuffer.getImageAttachments()[i][1].getImage(), 2, i);
			infoLightPass.addImage(&m_GBuffer.getImageAttachments()[i][2].getImage(), 3, i);
			infoLightPass.addImage(&m_GBuffer.getImageAttachments()[i][3].getImage(), 4, i);
			infoLightPass.addImage(&m_SSAOBlurFramebuffer.getImageAttachments()[i][0].getImage(), 5, i);
			uint32_t binding = 5;
			//infoLightPass.addImage(i, &m_SSRFramebuffer.getImageAttachment()[i][0].getImage(), 6);
			for (auto& entity : m_ScenePtr->getEntities()) {
				auto d_shadow_map = m_ScenePtr->getComponent<DShadowMapComponent>(entity);
				if (d_shadow_map) {
					binding++;
					infoLightPass.addImage(&d_shadow_map->ShadowMap.m_Attachment.getImageAttachments()[i][0].getImage(), 6, i);
					shader_info.p_Effects |= (uint32_t)Effects::ShadowMapping;
				}
			}
			for (auto& entity : m_ScenePtr->getEntities()) {
				auto point_light = m_ScenePtr->getComponent<PointLight3DComponent>(entity);
				auto p_shadow_map = m_ScenePtr->getComponent<PShadowMapComponent>(entity);

				if (point_light && p_shadow_map) {
					binding++;
					infoLightPass.addImageCube(&p_shadow_map->ShadowMap.m_Attachment.getImageAttachments()[i][0].getImageCube(), binding, i);
					shader_info.p_Effects |= (uint32_t)Effects::ShadowMapping;
				}
			}
		}
		m_LightPassFramebuffer.getDescriptors().create(infoLightPass);
		m_GBufferLightPassShader.compile(ShaderPermutationsGenerator::generateShaderGBufferLightPass(shader_info));
		m_LightPassFramebuffer.create();
	}
	void RenderPipeline_Deferred::setupSSRBuffer()
	{
		/////////////////////////////////////////////////////////////////////////////////////////////
		//				Screen-Space-Reflections
		/////////////////////////////////////////////////////////////////////////////////////////////
		static ShaderDescriptorInfo infoSSR;
		infoSSR.p_FragmentUniformBuffer = UniformBuffer(sizeof(SSR_DATA) + 2 * sizeof(Matrix4f), 0);
		for (int32_t i = 0; i < m_GBuffer.getImageAttachments().size(); i++) {
			infoSSR.addImage(&m_LightPassFramebuffer.getImageAttachments()[i][0].getImage(), 1, i);
			infoSSR.addImage(&m_GBuffer.getImageAttachments()[i][0].getImage(), 2, i);
			infoSSR.addImage(&m_GBuffer.getImageAttachments()[i][1].getImage(), 3, i);
			infoSSR.addImage(&m_GBuffer.getImageAttachments()[i][3].getImage(), 4, i);
		}
		
		m_SSRFramebuffer.getDescriptors().create(infoSSR);
		
		// Create SSR framebuffer
		m_SSRFramebuffer.m_FramebufferAttachmentInfo.p_Size = m_WindowPtr->getWindowSize();
		m_SSRFramebuffer.m_FramebufferAttachmentInfo.p_Shader = &m_SSRShader;
		m_SSRFramebuffer.addAttachment(
			m_WindowPtr->getWindowSize(),
			ImageFormat::C16SF_4C,
			Layout::ShaderR
		);
		m_SSRFramebuffer.create();
	}
	void RenderPipeline_Deferred::setupPostProcessBuffers()
	{
		/////////////////////////////////////////////////////////////////////////////////////////////
		//				FXAA framebuffer
		/////////////////////////////////////////////////////////////////////////////////////////////
		m_FXAAFramebuffer.m_FramebufferAttachmentInfo.p_Size = m_WindowPtr->getWindowSize();
		m_FXAAFramebuffer.m_FramebufferAttachmentInfo.p_Shader = &m_FXAAShader;
		m_FXAAFramebuffer.m_FramebufferAttachmentInfo.p_PushConstantData = &m_FXAAInfo;
		m_FXAAFramebuffer.m_FramebufferAttachmentInfo.p_PushConstantSize = sizeof(FXAAInfo);
		m_FXAAFramebuffer.m_FramebufferAttachmentInfo.p_PushConstantShaderStage = ShaderStage::FragmentBit;

		// Takes image from lightpass and applies depth of field
		m_FXAAFramebuffer.addAttachment(m_WindowPtr->getWindowSize(), ImageFormat::C16SF_4C, Layout::ShaderR);

		static ShaderDescriptorInfo infoFXAA = {};
		infoFXAA.p_FragmentUniformBuffer = UniformBuffer(sizeof(FXAAInfo), 1);

		for (int32_t i = 0; i < m_SSRFramebuffer.getImageAttachments().size(); i++)
			infoFXAA.addImage(&m_SSRFramebuffer.getImageAttachments()[i][0].getImage(), 0, i);

		m_FXAAFramebuffer.getDescriptors().create(infoFXAA);
		m_FXAAFramebuffer.create();

		/////////////////////////////////////////////////////////////////////////////////////////////
		//				Depth of Field framebuffer
		/////////////////////////////////////////////////////////////////////////////////////////////
		m_DOFFramebuffer.m_FramebufferAttachmentInfo.p_Size = m_WindowPtr->getWindowSize();
		m_DOFFramebuffer.m_FramebufferAttachmentInfo.p_Shader = &m_DOFShader;

		// Takes image from lightpass and applies depth of field
		m_DOFFramebuffer.addAttachment(m_WindowPtr->getWindowSize(), ImageFormat::C16SF_4C, Layout::ShaderR, true);

		static ShaderDescriptorInfo infoDOF = {};
		infoDOF.p_FragmentUniformBuffer = UniformBuffer(sizeof(DOFInfo), 1);

		for (int32_t i = 0; i < m_FXAAFramebuffer.getImageAttachments().size(); i++)
			infoDOF.addImage(&m_FXAAFramebuffer.getImageAttachments()[i][0].getImage(), 0, i);

		m_DOFFramebuffer.getDescriptors().create(infoDOF);
		m_DOFFramebuffer.create();
	}

	void RenderPipeline_Deferred::setupBloomBuffers()
	{
		static ShaderDescriptorInfo infoPrefilter;
		//infoHDR.p_FragmentUniformBuffer = UniformBuffer(sizeof(m_DOFInfo), 1);
		for (int32_t i = 0; i < m_DOFFramebuffer.getImageAttachments().size(); i++)
			infoPrefilter.addImage(&m_DOFFramebuffer.getImageAttachments()[i][0].getImage(), 0, i);
		m_PrefilterFramebufferAttachment.getDescriptors().create(infoPrefilter);

		m_PrefilterFramebufferAttachment.m_FramebufferAttachmentInfo.p_Size = m_WindowPtr->getWindowSize();
		m_PrefilterFramebufferAttachment.m_FramebufferAttachmentInfo.p_Shader = &m_PrefilterShader;
		m_PrefilterFramebufferAttachment.m_FramebufferAttachmentInfo.p_PushConstantData = &m_BloomThreshold;
		m_PrefilterFramebufferAttachment.m_FramebufferAttachmentInfo.p_PushConstantSize = sizeof(m_BloomThreshold);
		m_PrefilterFramebufferAttachment.m_FramebufferAttachmentInfo.p_PushConstantShaderStage = ShaderStage::FragmentBit;
		m_PrefilterFramebufferAttachment.addAttachment(m_WindowPtr->getWindowSize(), ImageFormat::C16SF_4C, Layout::ShaderR, true, false);
		m_PrefilterFramebufferAttachment.create();

		/////////////////////////////////////////////////////////////////////////////////////////////
		//				Downsampling framebuffers
		/////////////////////////////////////////////////////////////////////////////////////////////
#undef max
		m_DownsampleMipLevels = static_cast<uint32_t>(std::floor(std::log2(std::max(m_WindowPtr->getWindowSize().x, m_WindowPtr->getWindowSize().y)))) + 1;

		static std::vector<ShaderDescriptorInfo> infoDownsample;
		infoDownsample.resize(m_DownsampleMipLevels);
		m_DownsamplingFramebuffers.resize(m_DownsampleMipLevels);

		for (int32_t i = 0; i < m_PrefilterFramebufferAttachment.getImageAttachments().size(); i++)
			infoDownsample[0].addImage(&m_PrefilterFramebufferAttachment.getImageAttachments()[i][0].getImage(), 0, i);
		infoDownsample[0].p_FragmentUniformBuffer = UniformBuffer(sizeof(Vector4f), 1);
		m_DownsamplingFramebuffers[0].getDescriptors().create(infoDownsample[0]);

		static std::vector<int32_t> mip_level(m_DownsampleMipLevels);
		mip_level[0] = 0;

		m_DownsamplingFramebuffers[0].m_FramebufferAttachmentInfo.p_Size = m_WindowPtr->getWindowSize();
		m_DownsamplingFramebuffers[0].m_FramebufferAttachmentInfo.p_Shader = &m_DownsamplingShader;
		m_DownsamplingFramebuffers[0].m_FramebufferAttachmentInfo.p_PushConstantData = &mip_level[0];
		m_DownsamplingFramebuffers[0].m_FramebufferAttachmentInfo.p_PushConstantSize = sizeof(int);
		m_DownsamplingFramebuffers[0].m_FramebufferAttachmentInfo.p_PushConstantShaderStage = ShaderStage::FragmentBit;
		m_DownsamplingFramebuffers[0].addAttachment(m_WindowPtr->getWindowSize(), ImageFormat::C16SF_4C, Layout::ShaderR, true, false);
		m_DownsamplingFramebuffers[0].create();


		for (int32_t mip = 1; mip < m_DownsampleMipLevels; mip++) {
			for (int32_t i = 0; i < m_DownsamplingFramebuffers[mip-1].getImageAttachments().size(); i++)
				infoDownsample[mip].addImage(&m_DownsamplingFramebuffers[mip-1].getImageAttachments()[i][0].getImage(), 0, i);
			infoDownsample[mip].p_FragmentUniformBuffer = UniformBuffer(sizeof(Vector4f), 1);
			m_DownsamplingFramebuffers[mip].getDescriptors().create(infoDownsample[mip]);

			mip_level[mip] = mip;

			m_DownsamplingFramebuffers[mip].m_FramebufferAttachmentInfo.p_Size = m_WindowPtr->getWindowSize() / (2*mip);
			m_DownsamplingFramebuffers[mip].m_FramebufferAttachmentInfo.p_Shader = &m_DownsamplingShader;
			m_DownsamplingFramebuffers[mip].m_FramebufferAttachmentInfo.p_PushConstantData = &mip_level[mip];
			m_DownsamplingFramebuffers[mip].m_FramebufferAttachmentInfo.p_PushConstantSize = sizeof(int32_t);
			m_DownsamplingFramebuffers[mip].m_FramebufferAttachmentInfo.p_PushConstantShaderStage = ShaderStage::FragmentBit;
			m_DownsamplingFramebuffers[mip].addAttachment(m_WindowPtr->getWindowSize() / (2 * mip), ImageFormat::C16SF_4C, Layout::ShaderR, true, false);
			m_DownsamplingFramebuffers[mip].create();
		}

		/////////////////////////////////////////////////////////////////////////////////////////////
		//				Upsampling framebuffers
		/////////////////////////////////////////////////////////////////////////////////////////////

		m_UpsampleMipLevels = m_DownsampleMipLevels;

		static std::vector<ShaderDescriptorInfo> infoUpsample;
		infoUpsample.resize(m_UpsampleMipLevels);
		m_UpsamplingFramebuffers.resize(m_UpsampleMipLevels);

		for (int32_t i = 0; i < m_DownsamplingFramebuffers[m_UpsampleMipLevels - 1].getImageAttachments().size(); i++) {
			infoUpsample[m_UpsampleMipLevels - 1].addImage(&m_DownsamplingFramebuffers[m_UpsampleMipLevels - 1].getImageAttachments()[i][0].getImage(), 0, i);
			infoUpsample[m_UpsampleMipLevels - 1].addImage(&m_DownsamplingFramebuffers[m_UpsampleMipLevels - 1].getImageAttachments()[i][0].getImage(), 1, i);
		}
		m_UpsamplingFramebuffers[m_UpsampleMipLevels - 1].getDescriptors().create(infoUpsample[m_UpsampleMipLevels - 1]);

		m_UpsamplingFramebuffers[m_UpsampleMipLevels - 1].m_FramebufferAttachmentInfo.p_Size = m_WindowPtr->getWindowSize() / (2 * (m_UpsampleMipLevels - 1));
		m_UpsamplingFramebuffers[m_UpsampleMipLevels - 1].m_FramebufferAttachmentInfo.p_Shader = &m_UpsamplingShader;
		m_UpsamplingFramebuffers[m_UpsampleMipLevels - 1].m_FramebufferAttachmentInfo.p_PushConstantData = &m_FilterRadius;
		m_UpsamplingFramebuffers[m_UpsampleMipLevels - 1].m_FramebufferAttachmentInfo.p_PushConstantSize = sizeof(m_FilterRadius);
		m_UpsamplingFramebuffers[m_UpsampleMipLevels - 1].m_FramebufferAttachmentInfo.p_PushConstantShaderStage = ShaderStage::FragmentBit;
		m_UpsamplingFramebuffers[m_UpsampleMipLevels - 1].addAttachment(m_WindowPtr->getWindowSize() / (2 * (m_UpsampleMipLevels - 1)), ImageFormat::C16SF_4C, Layout::ShaderR, true, false);
		m_UpsamplingFramebuffers[m_UpsampleMipLevels - 1].create();

		for (int32_t mip = m_UpsampleMipLevels - 2; mip > -1; mip--) {
			for (int32_t i = 0; i < m_DownsamplingFramebuffers[mip].getImageAttachments().size(); i++) {
				infoUpsample[mip].addImage(&m_UpsamplingFramebuffers[mip+1].getImageAttachments()[i][0].getImage(), 0, i);
				infoUpsample[mip].addImage(&m_DownsamplingFramebuffers[mip].getImageAttachments()[i][0].getImage(), 1, i);
			}
			m_UpsamplingFramebuffers[mip].getDescriptors().create(infoUpsample[mip]);

			if(mip == 0)
				m_UpsamplingFramebuffers[mip].m_FramebufferAttachmentInfo.p_Size = m_WindowPtr->getWindowSize();
			else
				m_UpsamplingFramebuffers[mip].m_FramebufferAttachmentInfo.p_Size = m_WindowPtr->getWindowSize() / (2 * mip);

			m_UpsamplingFramebuffers[mip].m_FramebufferAttachmentInfo.p_Shader = &m_UpsamplingShader;
			m_UpsamplingFramebuffers[mip].m_FramebufferAttachmentInfo.p_PushConstantData = &m_FilterRadius;
			m_UpsamplingFramebuffers[mip].m_FramebufferAttachmentInfo.p_PushConstantSize = sizeof(m_FilterRadius);
			m_UpsamplingFramebuffers[mip].m_FramebufferAttachmentInfo.p_PushConstantShaderStage = ShaderStage::FragmentBit;
			if (mip == 0)
				m_UpsamplingFramebuffers[mip].addAttachment(m_WindowPtr->getWindowSize(), ImageFormat::C16SF_4C, Layout::ShaderR, true, false);
			else
				m_UpsamplingFramebuffers[mip].addAttachment(m_WindowPtr->getWindowSize() / (2 * (mip)), ImageFormat::C16SF_4C, Layout::ShaderR, true, false);
			m_UpsamplingFramebuffers[mip].create();
		}

		// Mix DOF image and Upsampled image
		static ShaderDescriptorInfo infoBloom;
		//infoHDR.p_FragmentUniformBuffer = UniformBuffer(sizeof(m_DOFInfo), 1);
		for (int32_t i = 0; i < m_DOFFramebuffer.getImageAttachments().size(); i++) {
			infoBloom.addImage(&m_DOFFramebuffer.getImageAttachments()[i][0].getImage(), 0, i);
			infoBloom.addImage(&m_UpsamplingFramebuffers[0].getImageAttachments()[i][0].getImage(), 1, i);
		}
		m_BloomFramebufferAttachment.getDescriptors().create(infoBloom);

		m_BloomFramebufferAttachment.m_FramebufferAttachmentInfo.p_Size = m_WindowPtr->getWindowSize();
		m_BloomFramebufferAttachment.m_FramebufferAttachmentInfo.p_Shader = &m_BloomShader;
		{
			m_BloomFramebufferAttachment.m_FramebufferAttachmentInfo.p_PushConstantData = &m_BloomMod;
			m_BloomFramebufferAttachment.m_FramebufferAttachmentInfo.p_PushConstantSize = sizeof(m_BloomMod);
			m_BloomFramebufferAttachment.m_FramebufferAttachmentInfo.p_PushConstantShaderStage = ShaderStage::FragmentBit;
		}
		m_BloomFramebufferAttachment.addAttachment(m_WindowPtr->getWindowSize(), ImageFormat::C16SF_4C, Layout::ShaderR, true, true);
		m_BloomFramebufferAttachment.create();// Mix DOF image and Upsampled image

		/////////////////////////////////////////////////////////////////////////////////////////////
		//				Hight-Definition-Range & Post-Processing
		/////////////////////////////////////////////////////////////////////////////////////////////
		static ShaderDescriptorInfo infoHDR;
		//infoHDR.p_FragmentUniformBuffer = UniformBuffer(sizeof(m_DOFInfo), 1);
		for (int32_t i = 0; i < m_BloomFramebufferAttachment.getImageAttachments().size(); i++) {
			infoHDR.addImage(&m_BloomFramebufferAttachment.getImageAttachments()[i][0].getImage(), 0, i);
		}
		infoHDR.p_StorageBuffer = StorageBuffer({ ShaderStage::FragmentBit }, sizeof(float), 1);
		m_HDRFramebuffer.getDescriptors().create(infoHDR);
		m_HDRFramebuffer.getDescriptors().copyToStorageBuffer(&m_HDRInfo.exposure, sizeof(float), 0);

		// Create Post-Processing framebuffer
		m_HDRFramebuffer.m_FramebufferAttachmentInfo.p_Size = m_WindowPtr->getWindowSize();
		m_HDRFramebuffer.m_FramebufferAttachmentInfo.p_Shader = &m_HDRShader;
		m_HDRFramebuffer.m_FramebufferAttachmentInfo.p_PushConstantData = &m_HDRInfo;
		m_HDRFramebuffer.m_FramebufferAttachmentInfo.p_PushConstantSize = sizeof(m_HDRInfo);
		m_HDRFramebuffer.m_FramebufferAttachmentInfo.p_PushConstantShaderStage = ShaderStage::FragmentBit;
		m_HDRFramebuffer.addAttachment(m_WindowPtr->getWindowSize(), ImageFormat::C16SF_4C, Layout::ShaderR);
		m_HDRFramebuffer.create();
	}

	void RenderPipeline_Deferred::setupVoxelBuffers()
	{
		/////////////////////////////////////////////////////////////////////////////////////////////
		//				Geometry Buffer
		/////////////////////////////////////////////////////////////////////////////////////////////
		m_VoxelFramebuffer.m_FramebufferAttachmentInfo.p_Size = Vector2i(256);
		m_VoxelFramebuffer.m_FramebufferAttachmentInfo.p_RenderPipelineInfo.p_CreateGraphicsPipeline = false;

		// Albedo
		m_VoxelFramebuffer.addAttachment(m_WindowPtr->getWindowSize(), ImageFormat::C16SF_4C, Layout::ShaderR);
		m_VoxelFramebuffer.create();
	}
	
	void RenderPipeline_Deferred::transferGBufferData()
	{
	}
	void RenderPipeline_Deferred::transferSSAOData()
	{
		// copy SSAO specific data to uniformbuffer declared in SSAO shaders
		m_SSAOFramebuffer.getDescriptors().copy(ShaderStage::FragmentBit, SSAO_Intensity, 0);
		m_SSAOFramebuffer.getDescriptors().copy(ShaderStage::FragmentBit, SSAO_Radius, sizeof(float));
		m_SSAOFramebuffer.getDescriptors().copy(ShaderStage::FragmentBit, SSAO_Bias, 2 * sizeof(float));
		float kernel_size = Sample_Count;
		m_SSAOFramebuffer.getDescriptors().copy(ShaderStage::FragmentBit, kernel_size, 3 * sizeof(float));
		m_SSAOFramebuffer.getDescriptors().copy(ShaderStage::FragmentBit, m_RendererPtr->getCurrentCameraPtr()->getPerspectiveMatrix(), sizeof(Vector4f));
		m_SSAOFramebuffer.getDescriptors().copy(ShaderStage::FragmentBit, m_RendererPtr->getViewProjection(), sizeof(Vector4f) + sizeof(Matrix4f));
	}
	void RenderPipeline_Deferred::transferSSRData()
	{
		// copy SSR specific data to uniformbuffer declared in SSR shaders
		m_SSRFramebuffer.getDescriptors().copy(ShaderStage::FragmentBit, SSR_Data, 0);
		//m_SSRFramebuffer.getDescriptors().copy(ShaderStage::FragmentBit, m_RendererPtr->getCurrentCameraPtr()->getViewDirection(), sizeof(SSR_DATA));
		m_SSRFramebuffer.getDescriptors().copy(ShaderStage::FragmentBit, m_RendererPtr->getCurrentCameraPtr()->getPerspectiveMatrix(), sizeof(SSR_DATA));
		m_SSRFramebuffer.getDescriptors().copy(ShaderStage::FragmentBit, m_RendererPtr->getCurrentCameraPtr()->getViewMatrix() , sizeof(SSR_DATA) + sizeof(Matrix4f));
		//m_SSRFramebuffer.getDescriptors().copy(ShaderStage::FragmentBit, m_RendererPtr->getViewProjection() , sizeof(SSR_DATA) + sizeof(Matrix4f));
	}
	void RenderPipeline_Deferred::transferLightPassData()
	{
		float point_light_count = 0;
		static P_Light p_light;

		m_LightPassFramebuffer.getDescriptors().copy(ShaderStage::FragmentBit, m_RendererPtr->getCurrentCameraPtr()->getPosition(), 0);
		for (auto& entity : m_ScenePtr->getEntities()) {
			auto directional_light = m_ScenePtr->getComponent<DirectionalLight3DComponent>(entity);
			auto d_shadow_map = m_ScenePtr->getComponent<DShadowMapComponent>(entity);
			if (directional_light && d_shadow_map) {
				volumetric_light_mie_scattering = directional_light->VolumetricLightDensity;
				m_LightPassFramebuffer.getDescriptors().copy(ShaderStage::FragmentBit, d_shadow_map->ShadowMap.m_View * d_shadow_map->ShadowMap.m_Projection, sizeof(Vector4f));
			}
			if (directional_light) {
				m_LightPassFramebuffer.getDescriptors().copy(ShaderStage::FragmentBit, directional_light->Direction, sizeof(Matrix4f) + sizeof(Vector4f));
				m_LightPassFramebuffer.getDescriptors().copy(ShaderStage::FragmentBit, directional_light->Color * directional_light->LightIntensity, sizeof(Matrix4f) + 2 * sizeof(Vector4f));
			}
			auto point_light = m_ScenePtr->getComponent<PointLight3DComponent>(entity);
			if (point_light) {
				p_light.Color = { 
					point_light->Color.r * point_light->LightIntensity,
					point_light->Color.g * point_light->LightIntensity,
					point_light->Color.b * point_light->LightIntensity, 0 
				};
				p_light.Position = { 
					point_light->Transform.getPosition().x,
					point_light->Transform.getPosition().y,
					point_light->Transform.getPosition().z,
					0.0f 
				};
				p_light.Radius = point_light->Radius;
				p_light.m_ShadowMapID = point_light->ShadowMapID;
				m_LightPassFramebuffer.getDescriptors().copy(ShaderStage::FragmentBit, p_light, 3 * sizeof(Vector4f) + sizeof(Matrix4f) + sizeof(P_Light) * point_light_count);
				point_light_count++;
			}
		}

		m_LightPassFramebuffer.getDescriptors().copy(ShaderStage::FragmentBit, point_light_count, sizeof(Vector3f));
	}
	void RenderPipeline_Deferred::transferPostProcessData()
	{
		for (auto& framebuffer : m_DownsamplingFramebuffers) {
			framebuffer.getDescriptors().copy(ShaderStage::FragmentBit, m_BloomThreshold, 0);
			framebuffer.getDescriptors().copy(ShaderStage::FragmentBit, m_BloomKnee, 16);
		}

		m_DOFFramebuffer.getDescriptors().copy(ShaderStage::FragmentBit, m_DOFInfo, 0);

		m_HDRInfo.deltatime = m_WindowPtr->getDeltaTime();
		m_HDRInfo.fstop = m_DOFInfo.fstop;
	}
	void RenderPipeline_Deferred::transferBloomData()
	{

	}
}
