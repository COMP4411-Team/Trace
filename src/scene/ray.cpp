#include "Ray.h"
#include "material.h"
#include "scene.h"

const Material &
Isect::getMaterial() const
{
    return material ? *material : obj->getMaterial();
}

// Assume that the light intersects at the outer surface
// Check whether self-intersection happens before using
Ray Ray::reflect(const Isect& isect)
{
	return Ray(at(isect.t) + isect.N * DISPLACEMENT_EPSILON, 
				d - 2 * isect.N.dot(d) * isect.N);		// Ray constructor will normalize the direction
}

// If TIR happens, return false
bool Ray::refract(const Isect& isect, Ray& out)
{
	double eta = isect.material->index;		// eta = n1 / n2
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
	
	out = Ray(at(isect.t) + normal * DISPLACEMENT_EPSILON, 
				eta * d + (eta * cosTheta1 - sqrt(cosTheta2Square)) * normal); // dir will be normalized in Ray ctor
	return true;
}
