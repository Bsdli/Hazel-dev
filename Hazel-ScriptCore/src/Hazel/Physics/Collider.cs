using System;

namespace Hazel
{
	public class Collider
	{
		public ulong EntityID { get; protected set; }
		public bool IsTrigger { get; protected set; }

		private Entity entity;
		private RigidBodyComponent _rigidBodyComponent;

		public Entity Entity
		{
			get
			{
				if (entity == null)
					entity = new Entity(EntityID);

				return entity;
			}
		}

		public RigidBodyComponent RigidBody
		{
			get
			{
				if (_rigidBodyComponent == null)
					_rigidBodyComponent = Entity.GetComponent<RigidBodyComponent>();

				return _rigidBodyComponent;
			}
		}

		public override string ToString()
		{
			string type = "Collider";

			if (this is BoxCollider) type = "BoxCollider";
			else if (this is SphereCollider) type = "SphereCollider";
			else if (this is CapsuleCollider) type = "CapsuleCollider";
			else if (this is MeshCollider) type = "MeshCollider";

			return "Collider(" + type + ", " + EntityID + ", " + IsTrigger + ")";
		}
	}

	public class BoxCollider : Collider
	{
		public Vector3 Size { get; protected set; }
		public Vector3 Offset { get; protected set; }

		private BoxCollider(ulong entityID, bool isTrigger, Vector3 size, Vector3 offset)
        {
			EntityID = entityID;
			IsTrigger = isTrigger;
			Size = size;
			Offset = offset;
        }
	}

	public class SphereCollider : Collider
	{
        public float Radius { get; protected set; }

        private SphereCollider(ulong entityID, bool isTrigger, float radius)
        {
            EntityID = entityID;
            IsTrigger = isTrigger;
			Radius = radius;
        }
    }

	public class CapsuleCollider : Collider
	{
        public float Radius { get; protected set; }
		public float Height { get; protected set; }

        private CapsuleCollider(ulong entityID, bool isTrigger, float radius, float height)
        {
            EntityID = entityID;
            IsTrigger = isTrigger;
            Radius = radius;
			Height = height;
        }
    }

	public class MeshCollider : Collider
	{
        public Mesh Mesh { get; protected set; }

		private MeshCollider(ulong entityID, bool isTrigger, IntPtr mesh)
        {
            EntityID = entityID;
			IsTrigger = isTrigger;
			Mesh = new Mesh(mesh);
        }
	}
}
