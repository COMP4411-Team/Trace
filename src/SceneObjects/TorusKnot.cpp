#include "TorusKnot.h"

TorusKnot::TorusKnot(Scene* scene, Material* material, double radius, double tube, double p, double q):
	MaterialSceneObject(scene, material), radius(radius), tube(tube), p(p), q(q) { }

// https://www.shadertoy.com/view/ttXBzB
// https://zhuanlan.zhihu.com/p/195680326
double TorusKnot::getDistance(const vec3f& pos) const
{
	double phi = atan2(pos[0], pos[2]) * q;
	double r = sqrt(pos[0] * pos[0] + pos[2] * pos[2]) - radius;
	double nr = cos(phi) * r - sin(phi) * pos[1];
	double z = _abs(sin(phi) * r + cos(phi) * pos[1]) - p;
	double d = sqrt(z * z + nr * nr) - tube;
	return d * 0.5;
}

bool TorusKnot::intersectLocal(const Ray& ray, Isect& isect) const
{
	vec3f curPos = ray.getPosition();
	double t = 0.0;
	for (int i = 0; i < maxIter; ++i)
	{
		double dist = getDistance(curPos);
		t += dist;
		if (_abs(dist) < epsilon)
		{
			isect.t = t;
			isect.N = getNormal(ray.at(t));
			isect.obj = this;
			return true;
		}
		curPos += ray.getDirection() * dist;
	}
	return false;
}

vec3f TorusKnot::getNormal(const vec3f& pos) const
{
	double dist = getDistance(pos);
	double dx = getDistance(pos - vec3f(epsilon, 0.0, 0.0));
	double dy = getDistance(pos - vec3f(0.0, epsilon, 0.0));
	double dz = getDistance(pos - vec3f(0.0, 0.0, epsilon));
	return (vec3f(dist) - vec3f(dx, dy, dz)).normalize();
}
