using System;
using System.Collections.Generic;
using System.Linq;
using System.Runtime.CompilerServices;
using System.Text;
using System.Threading.Tasks;

namespace Hazel
{
    public abstract class Component
    {
        public Entity Entity { get; set; }
    }

    public class TagComponent : Component
    {
        public string Tag
        {
            get
            {
                return GetTag_Native(Entity.ID);
            }
            set
            {
                SetTag_Native(value);
            }
        }

        [MethodImpl(MethodImplOptions.InternalCall)]
        public static extern string GetTag_Native(ulong entityID);

        [MethodImpl(MethodImplOptions.InternalCall)]
        public static extern void SetTag_Native(string tag);

    }

    public class TransformComponent : Component
    {
        private Transform m_Transform;

        public Transform Transform { get { return m_Transform; } }

		public Vector3 Position
		{
			get
			{
				GetTransform_Native(Entity.ID, out m_Transform);
				return m_Transform.Position;
			}

			set
			{
                m_Transform.Position = value;
				SetTransform_Native(Entity.ID, ref m_Transform);
			}
		}

		public Vector3 Rotation
		{
            get
			{
                GetTransform_Native(Entity.ID, out m_Transform);
                return m_Transform.Rotation;
			}

            set
			{
                m_Transform.Rotation = value;
                SetTransform_Native(Entity.ID, ref m_Transform);
			}
		}

		public Vector3 Scale
		{
			get
			{
				GetTransform_Native(Entity.ID, out m_Transform);
				return m_Transform.Scale;
			}

			set
			{
				m_Transform.Scale = value;
				SetTransform_Native(Entity.ID, ref m_Transform);
			}
		}

		[MethodImpl(MethodImplOptions.InternalCall)]
        internal static extern void GetTransform_Native(ulong entityID, out Transform result);
        [MethodImpl(MethodImplOptions.InternalCall)]
        internal static extern void SetTransform_Native(ulong entityID, ref Transform result);
	}

	public class MeshComponent : Component
    {
        public Mesh Mesh
        {
            get
            {
                Mesh result = new Mesh(GetMesh_Native(Entity.ID));
                return result;
            }
            set
            {
                IntPtr ptr = value == null ? IntPtr.Zero : value.m_UnmanagedInstance;
                SetMesh_Native(Entity.ID, ptr);
            }
        }

        [MethodImpl(MethodImplOptions.InternalCall)]
        internal static extern IntPtr GetMesh_Native(ulong entityID);

        [MethodImpl(MethodImplOptions.InternalCall)]
        internal static extern void SetMesh_Native(ulong entityID, IntPtr unmanagedInstance);

    }

    public class CameraComponent : Component
    {
       // TODO
    }

    public class ScriptComponent : Component
    {
        // TODO
    }

    public class SpriteRendererComponent : Component
    {
        // TODO
    }

    // TODO
    public class RigidBody2DComponent : Component
    {
        public void ApplyLinearImpulse(Vector2 impulse, Vector2 offset, bool wake)
        {
            ApplyLinearImpulse_Native(Entity.ID, ref impulse, ref offset, wake);
        }

        public Vector2 GetLinearVelocity()
        {
            GetLinearVelocity_Native(Entity.ID, out Vector2 velocity);
            return velocity;
        }

        public void SetLinearVelocity(Vector2 velocity)
        {
            SetLinearVelocity_Native(Entity.ID, ref velocity);
        }

        [MethodImpl(MethodImplOptions.InternalCall)]
        internal static extern void ApplyLinearImpulse_Native(ulong entityID, ref Vector2 impulse, ref Vector2 offset, bool wake);

        [MethodImpl(MethodImplOptions.InternalCall)]
        internal static extern void GetLinearVelocity_Native(ulong entityID, out Vector2 velocity);
        [MethodImpl(MethodImplOptions.InternalCall)]
        internal static extern void SetLinearVelocity_Native(ulong entityID, ref Vector2 velocity);
    }

    public class BoxCollider2DComponent : Component
    {
    }

    public class RigidBodyComponent : Component
	{
        public enum Type
		{
            Static,
            Dynamic
		}

        public Type BodyType
        {
            get
			{
                return GetBodyType_Native(Entity.ID);
			}
        }

        public float Mass
        {
            get { return GetMass_Native(Entity.ID); }
            set { SetMass_Native(Entity.ID, value); }
        }

        public uint Layer { get { return GetLayer_Native(Entity.ID); } }

        public void AddForce(Vector3 force, ForceMode forceMode = ForceMode.Force)
        {
            AddForce_Native(Entity.ID, ref force, forceMode);
        }

		public void AddTorque(Vector3 torque, ForceMode forceMode = ForceMode.Force)
		{
			AddTorque_Native(Entity.ID, ref torque, forceMode);
		}

        public Vector3 GetLinearVelocity()
		{
            GetLinearVelocity_Native(Entity.ID, out Vector3 velocity);
            return velocity;
		}

		public void SetLinearVelocity(Vector3 velocity)
		{
            SetLinearVelocity_Native(Entity.ID, ref velocity);
		}

        public Vector3 GetAngularVelocity()
		{
            GetAngularVelocity_Native(Entity.ID, out Vector3 velocity);
            return velocity;
		}

        public void SetAngularVelocity(Vector3 velocity)
		{
            SetAngularVelocity_Native(Entity.ID, ref velocity);
		}

        public void Rotate(Vector3 rotation)
		{
            Rotate_Native(Entity.ID, ref rotation);
		}

        // TODO: Add SetMaxLinearVelocity() as well

		[MethodImpl(MethodImplOptions.InternalCall)]
        internal static extern void AddForce_Native(ulong entityID, ref Vector3 force, ForceMode forceMode);
		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern void AddTorque_Native(ulong entityID, ref Vector3 torque, ForceMode forceMode);

		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern void GetLinearVelocity_Native(ulong entityID, out Vector3 velocity);
		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern void SetLinearVelocity_Native(ulong entityID, ref Vector3 velocity);

		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern void GetAngularVelocity_Native(ulong entityID, out Vector3 velocity);
		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern void SetAngularVelocity_Native(ulong entityID, ref Vector3 velocity);

		[MethodImpl(MethodImplOptions.InternalCall)]
        internal static extern void Rotate_Native(ulong entityID, ref Vector3 rotation);

        [MethodImpl(MethodImplOptions.InternalCall)]
        internal static extern uint GetLayer_Native(ulong entityID);

        [MethodImpl(MethodImplOptions.InternalCall)]
        internal static extern float GetMass_Native(ulong entityID);
        [MethodImpl(MethodImplOptions.InternalCall)]
        internal static extern float SetMass_Native(ulong entityID, float mass);

        [MethodImpl(MethodImplOptions.InternalCall)]
        internal static extern Type GetBodyType_Native(ulong entityID);
    }

}
