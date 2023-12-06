using System;
using System.Collections.Generic;
using System.Linq;
using System.Runtime.InteropServices;
using System.Text;
using System.Threading.Tasks;

namespace Hazel
{
    [StructLayout(LayoutKind.Sequential)]
    public struct Vector2
    {
        public float X;
        public float Y;

        public Vector2(float scalar)
        {
            X = Y = scalar;
        }

        public Vector2(float x, float y)
        {
            X = x;
            Y = y;
        }

        public Vector2(Vector3 vector)
        {
            X = vector.X;
            Y = vector.Y;
        }

		public void Clamp(Vector2 min, Vector2 max)
		{
			X = Mathf.Clamp(X, min.X, max.X);
			Y = Mathf.Clamp(Y, min.Y, max.Y);
		}

		public static Vector2 operator-(Vector2 left, Vector2 right)
		{
            return new Vector2(left.X - right.X, left.Y - right.Y);
		}

        public static Vector2 operator-(Vector2 vector)
        {
            return new Vector2(-vector.X, -vector.Y);
        }

		public override string ToString()
		{
			return "Vector2[" + X + ", " + Y + "]";
		}
	}
}