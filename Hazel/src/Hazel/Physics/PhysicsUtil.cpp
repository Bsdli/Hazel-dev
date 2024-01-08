#include "hzpch.h"
#include "PhysicsUtil.h"
#include "Hazel/Utilities/FileSystem.h"

#include <filesystem>

namespace Hazel {

	namespace Utils {

		static const char* GetCacheDirectory()
		{
			return "assets/cache/colliders/";
		}

		static void CreateCacheDirectoryIfNeeded()
		{
			std::string cacheDirectory = GetCacheDirectory();
			if (!std::filesystem::exists(cacheDirectory))
				std::filesystem::create_directories(cacheDirectory);
		}
	}

	physx::PxTransform ToPhysXTransform(const TransformComponent& transform)
	{
		physx::PxQuat r = ToPhysXQuat(glm::normalize(glm::quat(transform.Rotation)));
		physx::PxVec3 p = ToPhysXVector(transform.Translation);
		return physx::PxTransform(p, r);
	}

	physx::PxTransform ToPhysXTransform(const glm::mat4& transform)
	{
		// TODO(Yan): I don't trust glm::toQuat because it doesn't normalize the scale
 		physx::PxQuat r = ToPhysXQuat(glm::normalize(glm::toQuat(transform)));
		physx::PxVec3 p = ToPhysXVector(glm::vec3(transform[3]));
		return physx::PxTransform(p, r);
	}

	physx::PxTransform ToPhysXTransform(const glm::vec3& translation, const glm::vec3& rotation)
	{
		return physx::PxTransform(ToPhysXVector(translation), ToPhysXQuat(glm::quat(rotation)));
	}

	physx::PxMat44 ToPhysXMatrix(const glm::mat4& matrix)
	{
		return *(physx::PxMat44*)&matrix;
	}

	physx::PxVec3 ToPhysXVector(const glm::vec3& vector)
	{
		return *(physx::PxVec3*)&vector;
	}

	physx::PxVec4 ToPhysXVector(const glm::vec4& vector)
	{
		return *(physx::PxVec4*)&vector;
	}

	physx::PxQuat ToPhysXQuat(const glm::quat& quat)
	{
		// Note: PxQuat elements are in a different order than glm::quat!
		return physx::PxQuat(quat.x, quat.y, quat.z, quat.w);
	}

	glm::mat4 FromPhysXTransform(const physx::PxTransform& transform)
	{
		glm::quat rotation = FromPhysXQuat(transform.q);
		glm::vec3 position = FromPhysXVector(transform.p);
		return glm::translate(glm::mat4(1.0F), position) * glm::toMat4(rotation);
	}

	glm::mat4 FromPhysXMatrix(const physx::PxMat44& matrix)
	{
		return *(glm::mat4*)&matrix;
	}

	glm::vec3 FromPhysXVector(const physx::PxVec3& vector)
	{
		return *(glm::vec3*)&vector;
	}

	glm::vec4 FromPhysXVector(const physx::PxVec4& vector)
	{
		return *(glm::vec4*)&vector;
	}

	glm::quat FromPhysXQuat(const physx::PxQuat& quat)
	{
		return *(glm::quat*)&quat;
	}

	physx::PxFilterFlags HazelFilterShader(physx::PxFilterObjectAttributes attributes0, physx::PxFilterData filterData0, physx::PxFilterObjectAttributes attributes1, physx::PxFilterData filterData1, physx::PxPairFlags& pairFlags, const void* constantBlock, physx::PxU32 constantBlockSize)
	{
		if (physx::PxFilterObjectIsTrigger(attributes0) || physx::PxFilterObjectIsTrigger(attributes1))
		{
			pairFlags = physx::PxPairFlag::eTRIGGER_DEFAULT;
			return physx::PxFilterFlag::eDEFAULT;
		}

		pairFlags = physx::PxPairFlag::eCONTACT_DEFAULT;

		if ((filterData0.word0 & filterData1.word1) || (filterData1.word0 & filterData0.word1))
		{
			pairFlags |= physx::PxPairFlag::eNOTIFY_TOUCH_FOUND;
			pairFlags |= physx::PxPairFlag::eNOTIFY_TOUCH_LOST;
			return physx::PxFilterFlag::eDEFAULT;
		}

		return physx::PxFilterFlag::eSUPPRESS;
	}

	void PhysicsMeshSerializer::DeleteIfSerialized(const std::string& filepath)
	{
		if (!FileSystem::Exists(Utils::GetCacheDirectory()))
			FileSystem::CreateFolder(Utils::GetCacheDirectory());

		std::filesystem::path path = filepath;
		std::string cachePath = Utils::GetCacheDirectory() + path.filename().string() + ".pxm";
		if (IsSerialized(filepath))
			FileSystem::DeleteFile(cachePath);
	}

	void PhysicsMeshSerializer::SerializeMesh(const std::string& filepath, const Buffer& data)
	{
		if (!FileSystem::Exists(Utils::GetCacheDirectory()))
			FileSystem::CreateFolder(Utils::GetCacheDirectory());

		std::filesystem::path path = filepath;
		std::string cachePath = Utils::GetCacheDirectory() + path.filename().string() + ".pxm";

		FILE* f = fopen(cachePath.c_str(), "wb");
		if (f)
		{
			fwrite(data.Data, sizeof(byte), data.Size / sizeof(byte), f);
			fclose(f);
		}
		else
		{
			HZ_CORE_INFO("File Already Exists");
		}
	}

	bool PhysicsMeshSerializer::IsSerialized(const std::string& filepath)
	{
		if (!FileSystem::Exists(Utils::GetCacheDirectory()))
			FileSystem::CreateFolder(Utils::GetCacheDirectory());

		std::filesystem::path path = filepath;
		std::string cachePath = Utils::GetCacheDirectory() + path.filename().string() + ".pxm";
		return FileSystem::Exists(cachePath);
	}

	Buffer PhysicsMeshSerializer::DeserializeMesh(const std::string& filepath)
	{
		if (!FileSystem::Exists(Utils::GetCacheDirectory()))
			FileSystem::CreateFolder(Utils::GetCacheDirectory());

		std::filesystem::path path = filepath;
		std::string cachePath = Utils::GetCacheDirectory() + path.filename().string() + ".pxm";
		FILE* f = fopen(cachePath.c_str(), "rb");
		uint32_t size = 0;

		Buffer buffer;

		if (f)
		{
			fseek(f, 0, SEEK_END);
			size = ftell(f);
			fseek(f, 0, SEEK_SET);

			buffer.Allocate(size);
			fread(buffer.Data, sizeof(byte), size / sizeof(byte), f);

			fclose(f);
		}

		return buffer;
	}
}