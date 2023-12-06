#include "hzpch.h"
#include "PhysicsActor.h"
#include "PhysicsUtil.h"
#include "Hazel/Script/ScriptEngine.h"
#include "PXPhysicsWrappers.h"
#include "PhysicsLayer.h"

#include <glm/gtx/compatibility.hpp>

#include <PhysX/PxPhysicsAPI.h>

namespace Hazel {
	
	PhysicsActor::PhysicsActor(Entity entity)
		: m_Entity(entity), m_RigidBody(entity.GetComponent<RigidBodyComponent>())
	{
		if (!m_Entity.HasComponent<PhysicsMaterialComponent>())
		{
			m_Material.StaticFriction = 1.0F;
			m_Material.DynamicFriction = 1.0F;
			m_Material.Bounciness = 0.0F;
		}
		else
		{
			m_Material = entity.GetComponent<PhysicsMaterialComponent>();
		}

		Initialize();
	}

	PhysicsActor::~PhysicsActor()
	{
		if (m_ActorInternal && m_ActorInternal->isReleasable())
		{
			m_ActorInternal->release();
			m_ActorInternal = nullptr;
		}
	}

	glm::vec3 PhysicsActor::GetPosition()
	{
		return FromPhysXVector(m_ActorInternal->getGlobalPose().p);
	}

	glm::quat PhysicsActor::GetRotation()
	{
		return FromPhysXQuat(m_ActorInternal->getGlobalPose().q);
	}

	void PhysicsActor::Rotate(const glm::vec3& rotation)
	{
		physx::PxTransform transform = m_ActorInternal->getGlobalPose();
		transform.q *= (physx::PxQuat(glm::radians(rotation.x), { 1.0F, 0.0F, 0.0F })
			* physx::PxQuat(glm::radians(rotation.y), { 0.0F, 1.0F, 0.0F })
			* physx::PxQuat(glm::radians(rotation.z), { 0.0F, 0.0F, 1.0F }));
		m_ActorInternal->setGlobalPose(transform);
	}

	float PhysicsActor::GetMass() const
	{
		if (!IsDynamic())
		{
			HZ_CORE_WARN("Trying to set mass of non-dynamic PhysicsActor.");
			return 0.0F;
		}

		physx::PxRigidDynamic* actor = (physx::PxRigidDynamic*)m_ActorInternal;
		return actor->getMass();
	}

	void PhysicsActor::SetMass(float mass)
	{
		if (!IsDynamic())
		{
			HZ_CORE_WARN("Trying to set mass of non-dynamic PhysicsActor.");
			return;
		}

		physx::PxRigidDynamic* actor = (physx::PxRigidDynamic*)m_ActorInternal;
		physx::PxRigidBodyExt::setMassAndUpdateInertia(*actor, mass);
		m_RigidBody.Mass = mass;
	}

	void PhysicsActor::AddForce(const glm::vec3& force, ForceMode forceMode)
	{
		if (!IsDynamic())
		{
			HZ_CORE_WARN("Trying to add force to non-dynamic PhysicsActor.");
			return;
		}

		physx::PxRigidDynamic* actor = (physx::PxRigidDynamic*)m_ActorInternal;
		actor->addForce(ToPhysXVector(force), (physx::PxForceMode::Enum)forceMode);
	}

	void PhysicsActor::AddTorque(const glm::vec3& torque, ForceMode forceMode)
	{
		if (!IsDynamic())
		{
			HZ_CORE_WARN("Trying to add torque to non-dynamic PhysicsActor.");
			return;
		}

		physx::PxRigidDynamic* actor = (physx::PxRigidDynamic*)m_ActorInternal;
		actor->addTorque(ToPhysXVector(torque), (physx::PxForceMode::Enum)forceMode);
	}

	glm::vec3 PhysicsActor::GetLinearVelocity() const
	{
		if (!IsDynamic())
		{
			HZ_CORE_WARN("Trying to get velocity of non-dynamic PhysicsActor.");
			return glm::vec3(0.0F);
		}

		physx::PxRigidDynamic* actor = (physx::PxRigidDynamic*)m_ActorInternal;
		return FromPhysXVector(actor->getLinearVelocity());
	}

	void PhysicsActor::SetLinearVelocity(const glm::vec3& velocity)
	{
		if (!IsDynamic())
		{
			HZ_CORE_WARN("Trying to set velocity of non-dynamic PhysicsActor.");
			return;
		}

		if (!glm::all(glm::isfinite(velocity)))
			return;

		physx::PxRigidDynamic* actor = (physx::PxRigidDynamic*)m_ActorInternal;
		actor->setLinearVelocity(ToPhysXVector(velocity));
	}

	glm::vec3 PhysicsActor::GetAngularVelocity() const
	{
		if (!IsDynamic())
		{
			HZ_CORE_WARN("Trying to get angular velocity of non-dynamic PhysicsActor.");
			return glm::vec3(0.0F);
		}

		physx::PxRigidDynamic* actor = (physx::PxRigidDynamic*)m_ActorInternal;
		return FromPhysXVector(actor->getAngularVelocity());
	}

	void PhysicsActor::SetAngularVelocity(const glm::vec3& velocity)
	{
		if (!IsDynamic())
		{
			HZ_CORE_WARN("Trying to set angular velocity of non-dynamic PhysicsActor.");
			return;
		}

		if (!glm::all(glm::isfinite(velocity)))
			return;

		physx::PxRigidDynamic* actor = (physx::PxRigidDynamic*)m_ActorInternal;
		actor->setAngularVelocity(ToPhysXVector(velocity));
	}

	void PhysicsActor::SetLinearDrag(float drag) const
	{
		if (!IsDynamic())
			return;

		physx::PxRigidDynamic* actor = (physx::PxRigidDynamic*)m_ActorInternal;
		actor->setLinearDamping(drag);
	}

	void PhysicsActor::SetAngularDrag(float drag) const
	{
		if (!IsDynamic())
			return;

		physx::PxRigidDynamic* actor = (physx::PxRigidDynamic*)m_ActorInternal;
		actor->setAngularDamping(drag);
	}

	void PhysicsActor::SetLayer(uint32_t layerId)
	{
		physx::PxAllocatorCallback& allocator = PXPhysicsWrappers::GetAllocator();
		const PhysicsLayer& layerInfo = PhysicsLayerManager::GetLayer(layerId);

		if (layerInfo.CollidesWith == 0)
			return;

		physx::PxFilterData filterData;
		filterData.word0 = layerInfo.BitValue;
		filterData.word1 = layerInfo.CollidesWith;

		const physx::PxU32 numShapes = m_ActorInternal->getNbShapes();
		physx::PxShape** shapes = (physx::PxShape**)allocator.allocate(sizeof(physx::PxShape*) * numShapes, "", "", 0);
		m_ActorInternal->getShapes(shapes, numShapes);

		for (physx::PxU32 i = 0; i < numShapes; i++)
			shapes[i]->setSimulationFilterData(filterData);

		allocator.deallocate(shapes);
	}

	void PhysicsActor::Initialize()
	{
		physx::PxPhysics& physics = PXPhysicsWrappers::GetPhysics();

		if (m_RigidBody.BodyType == RigidBodyComponent::Type::Static)
		{
			m_ActorInternal = physics.createRigidStatic(ToPhysXTransform(m_Entity.Transform()));
		}
		else
		{
			const PhysicsSettings& settings = Physics::GetSettings();

			physx::PxRigidDynamic* actor = physics.createRigidDynamic(ToPhysXTransform(m_Entity.Transform()));
			actor->setLinearDamping(m_RigidBody.LinearDrag);
			actor->setAngularDamping(m_RigidBody.AngularDrag);
			actor->setRigidBodyFlag(physx::PxRigidBodyFlag::eKINEMATIC, m_RigidBody.IsKinematic);
			actor->setRigidDynamicLockFlag(physx::PxRigidDynamicLockFlag::eLOCK_LINEAR_X, m_RigidBody.LockPositionX);
			actor->setRigidDynamicLockFlag(physx::PxRigidDynamicLockFlag::eLOCK_LINEAR_Y, m_RigidBody.LockPositionY);
			actor->setRigidDynamicLockFlag(physx::PxRigidDynamicLockFlag::eLOCK_LINEAR_Z, m_RigidBody.LockPositionZ);
			actor->setRigidDynamicLockFlag(physx::PxRigidDynamicLockFlag::eLOCK_ANGULAR_X, m_RigidBody.LockRotationX);
			actor->setRigidDynamicLockFlag(physx::PxRigidDynamicLockFlag::eLOCK_ANGULAR_Y, m_RigidBody.LockRotationY);
			actor->setRigidDynamicLockFlag(physx::PxRigidDynamicLockFlag::eLOCK_ANGULAR_Z, m_RigidBody.LockRotationZ);
			actor->setActorFlag(physx::PxActorFlag::eDISABLE_GRAVITY, m_RigidBody.DisableGravity);
			actor->setSolverIterationCounts(settings.SolverIterations, settings.SolverVelocityIterations);

			physx::PxRigidBodyExt::setMassAndUpdateInertia(*actor, m_RigidBody.Mass);
			m_ActorInternal = actor;
		}

		m_MaterialInternal = physics.createMaterial(m_Material.StaticFriction, m_Material.DynamicFriction, m_Material.Bounciness);
		if (m_Entity.HasComponent<BoxColliderComponent>()) PXPhysicsWrappers::AddBoxCollider(*this);
		if (m_Entity.HasComponent<SphereColliderComponent>()) PXPhysicsWrappers::AddSphereCollider(*this);
		if (m_Entity.HasComponent<CapsuleColliderComponent>()) PXPhysicsWrappers::AddCapsuleCollider(*this);
		if (m_Entity.HasComponent<MeshColliderComponent>()) PXPhysicsWrappers::AddMeshCollider(*this);

		if (!PhysicsLayerManager::IsLayerValid(m_RigidBody.Layer))
			m_RigidBody.Layer = 0;

		SetLayer(m_RigidBody.Layer);
		m_ActorInternal->userData = &m_Entity;
	}

	void PhysicsActor::Spawn()
	{
		((physx::PxScene*)Physics::GetPhysicsScene())->addActor(*m_ActorInternal);
	}

	void PhysicsActor::Update(float fixedTimestep)
	{
		if (!ScriptEngine::IsEntityModuleValid(m_Entity))
			return;

		ScriptEngine::OnPhysicsUpdateEntity(m_Entity, fixedTimestep);
	}

	void PhysicsActor::SynchronizeTransform()
	{
		if (IsDynamic())
		{
			TransformComponent& transform = m_Entity.Transform();
			physx::PxTransform actorPose = m_ActorInternal->getGlobalPose();
			transform.Translation = FromPhysXVector(actorPose.p);
			transform.Rotation = glm::eulerAngles(FromPhysXQuat(actorPose.q));
		}
		else
		{
			// Synchronize Physics Actor with static Entity
			m_ActorInternal->setGlobalPose(ToPhysXTransform(m_Entity.Transform()));
		}
	}

	void PhysicsActor::AddCollisionShape(physx::PxShape* shape)
	{
		bool status = m_ActorInternal->attachShape(*shape);
		shape->release();
		if (!status)
			shape = nullptr;
	}

}
