#pragma once

#include "Mesh/Mesh.h"
#include "Shapes.h"
#include "../Application/RenderAPI.h"
#include "../GFXDefinitions.h"

namespace vgl
{
	class Skybox : public MeshData, public Transform3D
	{
		public:
			Skybox();
			Skybox(ImageCube& p_CubeMap);
			~Skybox();

			void create();
			void create(ImageCube& p_CubeMap);

		private:
			ImageCube* m_CubeMap = nullptr;
      
      AtmosphericScatteringInfo m_AtmosphericScatteringInfo;

			friend class vk::Renderer;

	};
}
