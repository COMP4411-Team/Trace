#ifndef __LIGHT_H__
#define __LIGHT_H__

#include "scene.h"

const double LIGHT_EPSILON = 0.01;

class Light
	: public SceneElement
{
public:
	virtual vec3f shadowAttenuation(const vec3f& P) const = 0;
	virtual double distanceAttenuation( const vec3f& P ) const = 0;
	virtual vec3f getColor( const vec3f& P ) const = 0;
	virtual vec3f getDirection( const vec3f& P ) const = 0;

protected:
	Light( Scene *scene, const vec3f& col )
		: SceneElement( scene ), color( col ) {}

	vec3f 		color;
};

class DirectionalLight
	: public Light
{
public:
	DirectionalLight( Scene *scene, const vec3f& orien, const vec3f& color )
		: Light( scene, color ), orientation( orien.normalize() ) { }
	virtual vec3f shadowAttenuation(const vec3f& P) const;
	virtual double distanceAttenuation( const vec3f& P ) const;
	virtual vec3f getColor( const vec3f& P ) const;
	virtual vec3f getDirection( const vec3f& P ) const;

protected:
	vec3f 		orientation;
};

class PointLight
	: public Light
{
public:
	PointLight( Scene *scene, const vec3f& pos, const vec3f& color )
		: Light( scene, color ), position( pos ) {}
	virtual vec3f shadowAttenuation(const vec3f& P) const;
	virtual double distanceAttenuation( const vec3f& P ) const;
	virtual vec3f getColor( const vec3f& P ) const;
	virtual vec3f getDirection( const vec3f& P ) const;
	void setAttenuationCoeff(double constant, double linear, double quadratic);

protected:
	vec3f position;
	double c0{LIGHT_EPSILON}, c1{0.0}, c2{1.0};	// coefficients for distance attenuation
};

class AmbientLight : public Light
{
public:
	AmbientLight( Scene *scene, const vec3f& color ): Light(scene, color) { }
	virtual vec3f shadowAttenuation(const vec3f& P) const { return vec3f(); }
	virtual double distanceAttenuation( const vec3f& P ) const { return 1.0; }
	virtual vec3f getColor( const vec3f& P ) const { return color; }
	virtual vec3f getDirection( const vec3f& P ) const { return vec3f(); }
};

#endif // __LIGHT_H__
