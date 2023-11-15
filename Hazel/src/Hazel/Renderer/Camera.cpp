#include "hzpch.h"
#include "Camera.h"

namespace Hazel {

	Camera::Camera(const glm::mat4& projectionMatrix)
		: m_ProjectionMatrix(projectionMatrix)
	{
	}

}