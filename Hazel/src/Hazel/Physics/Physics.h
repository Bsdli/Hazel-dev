#pragma once

#include "Hazel/Core/TimeStep.h"
#include "Hazel/Core/Base.h"
#include "Hazel/Scene/Entity.h"

namespace Hazel {

	class PhysicsActor;

	enum class ForceMode : uint16_t
	{
		Force = 0,
		Impulse,
		VelocityChange,
		Acceleration
	};

	enum class FilterGroup : uint32_t
	{
		Static = BIT(0),
		Dynamic = BIT(1),
		Kinematic = BIT(2),
		All = Static | Dynamic | Kinematic
	};

	enum class BroadphaseType
	{
		SweepAndPrune,
		MultiBoxPrune,
		AutomaticBoxPrune
		// TODO: GPU?
	};

	enum class FrictionType
	{
		Patch,
		OneDirectional,
		TwoDirectional
	};

	struct PhysicsSettings
	{
		float FixedTimestep = 0.02F;
		glm::vec3 Gravity = { 0.0F, -9.81F, 0.0F };
		BroadphaseType BroadphaseAlgorithm = BroadphaseType::AutomaticBoxPrune;
		glm::vec3 WorldBoundsMin = glm::vec3(0.0F);
		glm::vec3 WorldBoundsMax = glm::vec3(1.0F);
		uint32_t WorldBoundsSubdivisions = 2;
		FrictionType FrictionModel = FrictionType::Patch;
		uint32_t SolverIterations = 6;
		uint32_t SolverVelocityIterations = 1;
	};

	struct RaycastHit
	{
		uint64_t EntityID;
		glm::vec3 Position;
		glm::vec3 Normal;
		float Distance;
	};

	class Physics
	{
	public:
		static void Init();
		static void Shutdown();

		static void CreateScene();
		static Ref<PhysicsActor> CreateActor(Entity e);

		static Ref<PhysicsActor> GetActorForEntity(Entity entity);

		static void Simulate(Timestep ts);

		static void DestroyScene();

		static void* GetPhysicsScene();

	public:
		static PhysicsSettings& GetSettings();
	};

}
