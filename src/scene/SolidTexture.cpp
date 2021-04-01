#include "SolidTexture.h"
#include "scene.h"
#include "../vecmath/vecmath.h"

// Reference: https://raytracing.github.io/books/RayTracingTheNextWeek.html#solidtextures
PerlinNoise::PerlinNoise(int numPoint, double scale, int depth): numPoint(numPoint), scale(scale), depth(depth)
{
	randVec.resize(numPoint);
	for (auto& vec : randVec)
		vec = getRandomVector();

	generatePermutation(permX);
	generatePermutation(permY);
	generatePermutation(permZ);
}

vec3f PerlinNoise::sample(const vec3f& pos) const
{
	return vec3f(1.0) * 0.5 * (1.0 + sin(scale * pos[2] + 10.0 * turbulence(pos)));
}

double PerlinNoise::noise(const vec3f& pos) const
{
	double u = pos[0] - floor(pos[0]);
	double v = pos[1] - floor(pos[1]);
	double w = pos[2] - floor(pos[2]);

	int x = int(floor(pos[0]));
	int y = int(floor(pos[1]));
	int z = int(floor(pos[2]));

	vec3f color[2][2][2];

	for (int i = 0; i < 2; ++i)
		for (int j = 0; j < 2; ++j)
			for (int k = 0; k < 2; ++k)
				color[i][j][k] = randVec[permX[(i + x) & 255] ^ permY[(j + y) & 255] ^ permZ[(k + z) & 255]];

	return interpolate(color, u, v, w);
}

double PerlinNoise::turbulence(const vec3f& pos) const
{
	double weight = 1.0, result = 0.0;
	vec3f curPos = pos;
	for (int i = 0; i < depth; ++i)
	{
		result += weight * noise(curPos);
		curPos *= 2.0;
		weight *= 0.5;
	}
	return _abs(result);
}

void PerlinNoise::generatePermutation(std::vector<int>& vec)
{
	vec.resize(numPoint);
	for (int i = 0, size = vec.size(); i < size; ++i)
		vec[i] = i;
	for (int i = vec.size() - 1; i > 0; --i)
	{
		int idx = int(getUniformReal() * (i - 1));
		int tmp = vec[idx];
		vec[idx] = vec[i];
		vec[i] = tmp;
	}
}

double PerlinNoise::interpolate(vec3f color[2][2][2], double u, double v, double w)
{
	// Hermitian smoothing
	double su = u * u * (3.0 - 2.0 * u);
	double sv = v * v * (3.0 - 2.0 * v);
	double sw = w * w * (3.0 - 2.0 * w);
	double sum = 0.0;

	for (int i = 0; i < 2; ++i)
		for (int j = 0; j < 2; ++j)
			for (int k = 0; k < 2; ++k)
			{
				vec3f weight(u - i, v - j, w - k);
				sum += (i * su + (1 - i) * (1 - su)) * (j * sv + (1 - j) * (1 - sv)) * (k * sw + (1 - k) * (1 - sw))
					* color[i][j][k].dot(weight);
			}
	return sum;
}

vec3f PerlinNoise::getRandomVector()
{
	double x = getUniformReal(), y = getUniformReal(), z = getUniformReal();
	x = x * 2.0 - 1.0;
	y = y * 2.0 - 1.0;
	z = z * 2.0 - 1.0;
	return vec3f(x, y, z).normalize();
}


