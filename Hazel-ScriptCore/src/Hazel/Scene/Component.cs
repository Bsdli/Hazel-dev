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
        public Matrix4 Transform
        {
            get
            {
                Matrix4 result;
                GetTransform_Native(Entity.ID, out result);
                return result;
            }
            set
            {
                SetTransform_Native(Entity.ID, ref value);
            }
        }

        [MethodImpl(MethodImplOptions.InternalCall)]
        public static extern void GetTransform_Native(ulong entityID, out Matrix4 result);

        [MethodImpl(MethodImplOptions.InternalCall)]
        public static extern void SetTransform_Native(ulong entityID, ref Matrix4 result);

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
        public static extern IntPtr GetMesh_Native(ulong entityID);

        [MethodImpl(MethodImplOptions.InternalCall)]
        public static extern void SetMesh_Native(ulong entityID, IntPtr unmanagedInstance);

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
}
