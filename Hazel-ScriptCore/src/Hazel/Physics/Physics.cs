using System;
using System.Runtime.CompilerServices;
using System.Runtime.InteropServices;

namespace Hazel
{

    [StructLayout(LayoutKind.Sequential)]
    public struct RaycastHit
    {
        public ulong EntityID { get; private set; }
        public Vector3 Position { get; private set; }
        public Vector3 Normal { get; private set; }
        public float Distance { get; private set; }
    }

    public static class Physics
    {
        public static bool Raycast(Vector3 origin, Vector3 direction, float maxDistance, out RaycastHit hit)
        {
            return Raycast_Native(ref origin, ref direction, maxDistance, out hit);
        }

        public static Collider[] OverlapBox(Vector3 origin, Vector3 halfSize)
        {
            return OverlapBox_Native(ref origin, ref halfSize);
        }

        public static Collider[] OverlapCapsule(Vector3 origin, float radius, float halfHeight)
		{
            return OverlapCapsule_Native(ref origin, radius, halfHeight);
		}

        public static Collider[] OverlapSphere(Vector3 origin, float radius)
        {
            return OverlapSphere_Native(ref origin, radius);
        }

		public static int OverlapBoxNonAlloc(Vector3 origin, Vector3 halfSize, Collider[] colliders)
		{
			return OverlapBoxNonAlloc_Native(ref origin, ref halfSize, colliders);
		}

		public static int OverlapCapsuleNonAlloc(Vector3 origin, float radius, float halfHeight, Collider[] colliders)
		{
			return OverlapCapsuleNonAlloc_Native(ref origin, radius, halfHeight, colliders);
		}

		public static int OverlapSphereNonAlloc(Vector3 origin, float radius, Collider[] colliders)
		{
			return OverlapSphereNonAlloc_Native(ref origin, radius, colliders);
		}

		[MethodImpl(MethodImplOptions.InternalCall)]
        internal static extern bool Raycast_Native(ref Vector3 origin, ref Vector3 direction, float maxDistance, out RaycastHit hit);

        [MethodImpl(MethodImplOptions.InternalCall)]
        internal static extern Collider[] OverlapBox_Native(ref Vector3 origin, ref Vector3 halfSize);
		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern Collider[] OverlapCapsule_Native(ref Vector3 origin, float radius, float halfHeight);
		[MethodImpl(MethodImplOptions.InternalCall)]
        internal static extern Collider[] OverlapSphere_Native(ref Vector3 origin, float radius);
		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern int OverlapBoxNonAlloc_Native(ref Vector3 origin, ref Vector3 halfSize, Collider[] colliders);
		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern int OverlapCapsuleNonAlloc_Native(ref Vector3 origin, float radius, float halfHeight, Collider[] colliders);
		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern int OverlapSphereNonAlloc_Native(ref Vector3 origin, float radius, Collider[] colliders);
	}
}
