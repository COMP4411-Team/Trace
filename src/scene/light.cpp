#include <cmath>

#include "light.h"

double DirectionalLight::distanceAttenuation( const vec3f& P ) const
{
	// distance to light is infinite, so f(di) goes to 0.  Return 1.
	return 1.0;
}


vec3f DirectionalLight::shadowAttenuation( const vec3f& P, double t ) const
{
    // YOUR CODE HERE:
    // You should implement shadow-handling code here.

	Ray r(P, -orientation, t);
	Isect isect;
	if (scene->bvhIntersect(r, isect))
		return isect.getMaterial().kt;
    return vec3f(1,1,1);
}

vec3f DirectionalLight::getColor( const vec3f& P ) const
{
	// Color doesn't depend on P 
	return color;
}

vec3f DirectionalLight::getDirection( const vec3f& P ) const
{
	return -orientation;
}

double PointLight::distanceAttenuation( const vec3f& P ) const
{
	// YOUR CODE HERE

	// You'll need to modify this method to attenuate the intensity 
	// of the light based on the distance between the source and the 
	// point P.  For now, I assume no attenuation and just return 1.0

	double dist = (P - position).length();
	
	return min(1.0, scene->lightScale / (c0 + c1 * dist + c2 * dist * dist));
}

vec3f PointLight::getColor( const vec3f& P ) const
{
	// Color doesn't depend on P 
	return color;
}

vec3f PointLight::getDirection( const vec3f& P ) const
{
	return (position - P).normalize();
}

void PointLight::setAttenuationCoeff(double constant, double linear, double quadratic)
{
	c0 = constant;
	c1 = linear;
	c2 = quadratic;
}


vec3f PointLight::shadowAttenuation(const vec3f& P, double t) const
{
    // YOUR CODE HERE:
    // You should implement shadow-handling code here.

	Ray r(P, getDirection(P), t);
	Isect isect;
	if (scene->bvhIntersect(r, isect))
		return isect.getMaterial().kt;
    return vec3f(1,1,1);
}

vec3f SpotLight::shadowAttenuation(const vec3f& P, double t) const
{
	vec3f dir = -getDirection(P);
	double cosAngle = dir.dot(direction);
	if (cosAngle < cosCone)		// out of the spotlight
		return vec3f();
	
	double attenuation = smoothstep(cosCone, cosPenumbra, cosAngle);
	
	Ray r(P, getDirection(P), t);
	Isect isect;
	if (scene->bvhIntersect(r, isect))
		return attenuation * isect.getMaterial().kt;
	
    return attenuation * vec3f(1,1,1);
}

double SpotLight::distanceAttenuation(const vec3f& P) const
{
	double dist = (position - P).length();
	
	return pow(_max(1.0 - dist / cutoffDist, 0.0), 2);
}

vec3f SpotLight::getDirection(const vec3f& P) const
{
	return (position - P).normalize();
}

double smoothstep(double edge0, double edge1, double x)
{
	double t;
    t = _min(_max((x - edge0) / (edge1 - edge0), 0.0), 1.0);
    return t * t * (3.0 - 2.0 * t);
}

