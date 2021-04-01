#pragma once
#include <vector>
#include "../vecmath/vecmath.h"

class SolidTexture
{
public:
	virtual ~SolidTexture() { }
	virtual vec3f sample(const vec3f& pos) const = 0;
};


class PerlinNoise : public SolidTexture
{
public:
	PerlinNoise(int numPoint, double scale, int depth);
	vec3f sample(const vec3f& pos) const override;

protected:
	double noise(const vec3f& pos) const;
	double turbulence(const vec3f& pos) const;
	void generatePermutation(vector<int>& vec);
	static double interpolate(vec3f color[2][2][2], double u, double v, double w);
	static vec3f getRandomVector();
	
	std::vector<vec3f> randVec;
	std::vector<int> permX, permY, permZ;
	int numPoint;
	int depth;
	double scale;
};

