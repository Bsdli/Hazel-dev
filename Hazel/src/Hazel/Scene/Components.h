#pragma once

#include <glm/glm.hpp>

#include "Hazel/Renderer/Texture.h"
#include "Hazel/Renderer/Mesh.h"
#include "Hazel/Renderer/Camera.h"

namespace Hazel {

	struct TagComponent
	{
		std::string Tag;

		operator std::string& () { return Tag; }
		operator const std::string& () const { return Tag; }
	};

	struct TransformComponent
	{
		glm::mat4 Transform;

		operator glm::mat4& () { return Transform; }
		operator const glm::mat4& () const { return Transform; }
	};

	struct MeshComponent
	{
		Ref<Hazel::Mesh> Mesh;

		operator Ref<Hazel::Mesh> () { return Mesh; }
	};

	struct ScriptComponent
	{
		// TODO: C# script
		std::string ModuleName;
	};

	struct CameraComponent
	{
		//OrthographicCamera Camera;
		Hazel::Camera Camera;
		bool Primary = true;

		operator Hazel::Camera& () { return Camera; }
		operator const Hazel::Camera& () const { return Camera; }
	};

	struct SpriteRendererComponent
	{
		glm::vec4 Color = { 1.0f, 1.0f, 1.0f, 1.0f };
		Ref<Texture2D> Texture;
		float TilingFactor = 1.0f;
	};


}
