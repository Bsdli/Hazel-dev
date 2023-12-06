#include "hzpch.h"
#include "PhysicsLayer.h"

namespace Hazel {

	template<typename T, typename ConditionFunction>
	static bool RemoveIfExists(std::vector<T>& vector, ConditionFunction condition)
	{
		for (std::vector<T>::iterator it = vector.begin(); it != vector.end(); ++it)
		{
			if (condition(*it))
			{
				vector.erase(it);
				return true;
			}
		}

		return false;
	}

	uint32_t PhysicsLayerManager::AddLayer(const std::string& name, bool setCollisions)
	{
		uint32_t layerId = GetNextLayerID();
		PhysicsLayer layer = { layerId, name, BIT(layerId), BIT(layerId) };
		s_Layers.insert(s_Layers.begin() + layerId, layer);

		if (setCollisions)
		{
			for (const auto& layer2 : s_Layers)
			{
				SetLayerCollision(layer.LayerID, layer2.LayerID, true);
			}
		}

		return layer.LayerID;
	}

	void PhysicsLayerManager::RemoveLayer(uint32_t layerId)
	{
		PhysicsLayer& layerInfo = GetLayer(layerId);

		for (auto& otherLayer : s_Layers)
		{
			if (otherLayer.LayerID == layerId)
				continue;

			if (otherLayer.CollidesWith & layerInfo.BitValue)
			{
				otherLayer.CollidesWith &= ~layerInfo.BitValue;
			}
		}

		RemoveIfExists<PhysicsLayer>(s_Layers, [&](const PhysicsLayer& layer) { return layer.LayerID == layerId; });
	}

	void PhysicsLayerManager::SetLayerCollision(uint32_t layerId, uint32_t otherLayer, bool shouldCollide)
	{
		if (ShouldCollide(layerId, otherLayer) && shouldCollide)
			return;

		PhysicsLayer& layerInfo = GetLayer(layerId);
		PhysicsLayer& otherLayerInfo = GetLayer(otherLayer);

		if (shouldCollide)
		{
			layerInfo.CollidesWith |= otherLayerInfo.BitValue;
			otherLayerInfo.CollidesWith |= layerInfo.BitValue;
		}
		else
		{
			layerInfo.CollidesWith &= ~otherLayerInfo.BitValue;
			otherLayerInfo.CollidesWith &= ~layerInfo.BitValue;
		}
	}

	std::vector<PhysicsLayer> PhysicsLayerManager::GetLayerCollisions(uint32_t layerId)
	{
		const PhysicsLayer& layer = GetLayer(layerId);

		std::vector<PhysicsLayer> layers;
		for (const auto& otherLayer : s_Layers)
		{
			if (otherLayer.LayerID == layerId)
				continue;

			if (layer.CollidesWith & otherLayer.BitValue)
				layers.push_back(otherLayer);
		}

		return layers;
	}

	PhysicsLayer& PhysicsLayerManager::GetLayer(uint32_t layerId)
	{
		return layerId >= s_Layers.size() ? s_NullLayer : s_Layers[layerId];
	}

	PhysicsLayer& PhysicsLayerManager::GetLayer(const std::string& layerName)
	{
		for (auto& layer : s_Layers)
		{
			if (layer.Name == layerName)
			{
				return layer;
			}
		}

		return s_NullLayer;
	}

	bool PhysicsLayerManager::ShouldCollide(uint32_t layer1, uint32_t layer2)
	{
		return GetLayer(layer1).CollidesWith & GetLayer(layer2).BitValue;
	}

	bool PhysicsLayerManager::IsLayerValid(uint32_t layerId)
	{
		const PhysicsLayer& layer = GetLayer(layerId);
		return layer.LayerID != s_NullLayer.LayerID && layer.IsValid();
	}

	uint32_t PhysicsLayerManager::GetNextLayerID()
	{
		int32_t lastId = -1;

		for (const auto& layer : s_Layers)
		{
			if (lastId != -1)
			{
				if (layer.LayerID != lastId + 1)
				{
					return lastId + 1;
				}
			}

			lastId = layer.LayerID;
		}

		return s_Layers.size();
	}

	std::vector<PhysicsLayer> PhysicsLayerManager::s_Layers;
	PhysicsLayer PhysicsLayerManager::s_NullLayer = { 0, "NULL", 0, -1 };
}