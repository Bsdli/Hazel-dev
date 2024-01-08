using System.Runtime.InteropServices;

namespace Hazel
{
	[StructLayout(LayoutKind.Sequential)]
	public struct Transform
	{
		public Vector3 Position;
		public Vector3 Rotation;
		public Vector3 Scale;

        // If we need these:
        // glm::vec3 right = glm::normalize(glm::rotate(rotation, glm::vec3(1.0F, 0.0F, 0.0F)));
        // glm::vec3 up = glm::normalize(glm::rotate(rotation, glm::vec3(0.0F, 1.0F, 0.0F)));
        // glm::vec3 forward = glm::normalize(glm::rotate(rotation, glm::vec3(0.0F, 0.0F, -1.0F)));
        public Vector3 Up { get; }
		public Vector3 Right { get; }
		public Vector3 Forward { get; }

	}
}
