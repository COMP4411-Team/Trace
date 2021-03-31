#include "Ray.h"
#include "material.h"
#include "scene.h"

const Material &
Isect::getMaterial() const
{
    return material ? *material : obj->getMaterial();
}

Ray Ray::reflect(const Isect& isect) const
{
	vec3f normal = isect.N.dot(d) < 0.0 ? isect.N : -isect.N;
	return Ray(at(isect.t) + normal * DISPLACEMENT_EPSILON, 
				d - 2 * normal.dot(d) * normal, time);		// Ray constructor will normalize the direction
}

// If TIR happens, return false
bool Ray::refract(const Isect& isect, Ray& out) const
{
	double eta = isect.getMaterial().index;		// eta = n1 / n2
	vec3f normal = -isect.N;
	if (d.dot(isect.N) < 0.0)	// intersects from outside
	{
		eta = 1.0 / eta;		// TODO handle different indices of refraction of the other medium
		normal = isect.N;
	}
	
	double cosTheta1 = d.dot(-normal);
	double cosTheta2Square = 1 - eta * eta * (1 - cosTheta1 * cosTheta1);

	if (cosTheta2Square < 0.0)
		return false;		// TIR
	
	out = Ray(at(isect.t) - normal * DISPLACEMENT_EPSILON, 
				eta * d + (eta * cosTheta1 - sqrt(cosTheta2Square)) * normal, time); // dir will be normalized in Ray ctor
	return true;
}

vec3f Ray::normalToPoint(const vec3f& point) const
{
	vec3f v = point - p;
	vec3f proj = v.dot(d) * d;	// d is normalized in constructor
	return proj - v;
}

double Ray::solveT(const vec3f& point) const
{
	if (d[0] != 0.0)
		return (point[0] - p[0]) / d[0];
	if (d[1] != 0.0)
		return (point[1] - p[1]) / d[1];
	if (d[2] != 0.0)
		return (point[2] - p[2]) / d[2];
}
