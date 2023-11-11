#include "hzpch.h"
#include "Noise.h"

#include "FastNoise.h"

namespace Hazel {

	static FastNoise s_FastNoise;

	float Noise::PerlinNoise(float x, float y)
	{
		s_FastNoise.SetNoiseType(FastNoise::Perlin);
		float result = s_FastNoise.GetNoise(x, y); // This returns a value between -1 and 1
		return result;
	}

}