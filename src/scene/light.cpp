#include <cmath>

#include "light.h"

double DirectionalLight::distanceAttenuation( const vec3f& P ) const
{
	// distance to light is infinite, so f(di) goes to 0.  Return 1.
	return 1.0;
}


vec3f DirectionalLight::shadowAttenuation( const vec3f& P ) const
{
    // YOUR CODE HERE:
    // You should implement shadow-handling code here.

	Ray r(P, -orientation);
	Isect isec;
	if (scene->intersect(r, isec))
		return vec3f(0, 0, 0);	// TODO add support to transparent objects
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
	
	return min(1.0, 1.0 / (c0 + c1 * dist + c2 * dist * dist));
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


vec3f PointLight::shadowAttenuation(const vec3f& P) const
{
    // YOUR CODE HERE:
    // You should implement shadow-handling code here.

	Ray r(P, getDirection(P));
	Isect isec;
	if (scene->intersect(r, isec))
		return vec3f(0, 0, 0);	// TODO add support to transparent objects
    return vec3f(1,1,1);
}
