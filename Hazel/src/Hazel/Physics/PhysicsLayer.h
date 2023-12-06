#pragma once

namespace Hazel {

	struct PhysicsLayer
	{
		uint32_t LayerID;
		std::string Name;
		uint32_t BitValue;
		int32_t CollidesWith = 0;

		bool IsValid() const
		{
			return LayerID >= 0 && !Name.empty() && BitValue > 0;
		}
	};

	class PhysicsLayerManager
	{
	public:
		static uint32_t AddLayer(const std::string& name, bool setCollisions = true);
		static void RemoveLayer(uint32_t layerId);

		static void SetLayerCollision(uint32_t layerId, uint32_t otherLayer, bool shouldCollide);
		static std::vector<PhysicsLayer> GetLayerCollisions(uint32_t layerId);

		static const std::vector<PhysicsLayer>& GetLayers() { return s_Layers; }

		static PhysicsLayer& GetLayer(uint32_t layerId);
		static PhysicsLayer& GetLayer(const std::string& layerName);
		static uint32_t GetLayerCount() { return s_Layers.size(); }

		static bool ShouldCollide(uint32_t layer1, uint32_t layer2);
		static bool IsLayerValid(uint32_t layerId);

	private:
		static uint32_t GetNextLayerID();

	private:
		static std::vector<PhysicsLayer> s_Layers;
		static PhysicsLayer s_NullLayer;
	};

}
