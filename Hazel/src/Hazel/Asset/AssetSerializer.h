#pragma once

#include "Asset.h"

namespace Hazel {

	class AssetSerializer
	{
	public:
		virtual void Serialize(const Ref<Asset>& asset) const = 0;
		virtual bool TryLoadData(Ref<Asset>& asset) const = 0;

	protected:
		void CopyMetadata(const Ref<Asset>& from, Ref<Asset>& to) const;
	};

	class TextureSerializer : public AssetSerializer
	{
	public:
		virtual void Serialize(const Ref<Asset>& asset) const override{}
		virtual bool TryLoadData(Ref<Asset>& asset) const override;
	};

	class MeshSerializer : public AssetSerializer
	{
	public:
		virtual void Serialize(const Ref<Asset>& asset) const override{}
		virtual bool TryLoadData(Ref<Asset>& asset) const override;
	};

	class EnvironmentSerializer : public AssetSerializer
	{
	public:
		virtual void Serialize(const Ref<Asset>& asset) const override{}
		virtual bool TryLoadData(Ref<Asset>& asset) const override;
	};

	class PhysicsMaterialSerializer : public AssetSerializer
	{
	public:
		virtual void Serialize(const Ref<Asset>& asset) const override;
		virtual bool TryLoadData(Ref<Asset>& asset) const override;
	};

}
