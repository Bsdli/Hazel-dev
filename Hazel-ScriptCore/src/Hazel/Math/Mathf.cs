using System;

namespace Hazel
{
	public static class Mathf
	{

		public const float Deg2Rad = ((float)Math.PI * 2.0F) / 360.0F;
		public const float Rad2Deg = 360.0F / ((float)Math.PI * 2.0F);

		public static float Clamp(float value, float min, float max)
		{
			if (value < min)
				return min;
			if (value > max)
				return max;
			return value;
		}
	}
}
