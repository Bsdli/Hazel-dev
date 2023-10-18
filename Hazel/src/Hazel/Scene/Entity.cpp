#include "hzpch.h"
#include "Entity.h"

namespace Hazel {

	Entity::Entity(const std::string& name)
		: m_Name(name), m_Transform(1.0f)
	{

	}

	Entity::~Entity()
	{

	}

}