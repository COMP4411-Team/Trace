#ifndef __LIGHT_H__
#define __LIGHT_H__

#include "scene.h"

class Photon;
const double LIGHT_EPSILON = 0.01;

class Light
	: public SceneElement
{
public:
	virtual vec3f shadowAttenuation(const vec3f& P, double t) const = 0;
	virtual double distanceAttenuation( const vec3f& P ) const = 0;
	virtual vec3f getColor( const vec3f& P ) const = 0;
	virtual vec3f getDirection( const vec3f& P ) const = 0;
	virtual vec3f getDirAndAtten(const vec3f& objPos, vec3f& attenuation,  double t) const { return vec3f(0.0); }
	virtual Photon* emitPhoton() const { return nullptr; }
	virtual void buildProjectionMap() { }
	virtual bool isAreaLight() const { return false; }

protected:
	Light( Scene *scene, const vec3f& col )
		: SceneElement( scene ), color( col ) {}

	vec3f 		color;
	int mapSize{64};
	bool* projMap{nullptr};
	std::vector<int> cells;
};

class DirectionalLight
	: public Light
{
public:
	DirectionalLight( Scene *scene, const vec3f& orien, const vec3f& color )
		: Light( scene, color ), orientation( orien.normalize() )
	{
		if (orientation[0] > 0.9)
			u = vec3f(0.0, 1.0, 0.0);
		else
			u = vec3f(1.0, 0.0, 0.0);
		u = u.cross(orientation).normalize();
		v = orientation.cross(u);
	}
	virtual vec3f shadowAttenuation(const vec3f& P, double t) const;
	virtual double distanceAttenuation( const vec3f& P ) const;
	virtual vec3f getColor( const vec3f& P ) const;
	virtual vec3f getDirection( const vec3f& P ) const;
	Photon* emitPhoton() const override;
	void buildProjectionMap() override;

protected:
	vec3f project(const vec3f& pos) const;
	vec3f unproject(double x, double y) const;
	
	vec3f 		orientation;
	vec3f u, v;		// for projection map
	double sceneRadius{-1.0};
	vec3f position;		// virtual position of the directional light plane
};

class PointLight
	: public Light
{
public:
	PointLight( Scene *scene, const vec3f& pos, const vec3f& color )
		: Light( scene, color ), position( pos ) {}
	virtual vec3f shadowAttenuation(const vec3f& P, double t) const;
	virtual double distanceAttenuation( const vec3f& P ) const;
	virtual vec3f getColor( const vec3f& P ) const;
	virtual vec3f getDirection( const vec3f& P ) const;
	Photon* emitPhoton() const override;
	void setAttenuationCoeff(double constant, double linear, double quadratic);

protected:
	vec3f position;
	double c0{LIGHT_EPSILON}, c1{0.0}, c2{1.0};	// coefficients for distance attenuation
};

class AmbientLight : public Light
{
public:
	AmbientLight( Scene *scene, const vec3f& color ): Light(scene, color) { }
	virtual vec3f shadowAttenuation(const vec3f& P, double t) const { return vec3f(); }
	virtual double distanceAttenuation( const vec3f& P ) const { return 1.0; }
	virtual vec3f getColor( const vec3f& P ) const { return color; }
	virtual vec3f getDirection( const vec3f& P ) const { return vec3f(); }
};

class SpotLight : public Light
{
public:
	SpotLight( Scene *scene, const vec3f& color, double cutoffDist, double penumbra, double coneAngle ):
		Light(scene, color), cutoffDist(cutoffDist), cosPenumbra(cos(penumbra)), cosCone(cos(coneAngle)) { }
	virtual vec3f shadowAttenuation(const vec3f& P, double t) const;
	virtual double distanceAttenuation( const vec3f& P ) const;
	virtual vec3f getColor( const vec3f& P ) const { return color; }
	virtual vec3f getDirection( const vec3f& P ) const;
	void setPos(const vec3f& pos) { position = pos; direction = (target - position).normalize(); }
	void setTarget(const vec3f& target) { this->target = target; direction = (target - position).normalize(); }

protected:
	double cutoffDist;
	double cosPenumbra;
	double cosCone;
	vec3f position;
	vec3f target;
	vec3f direction;
};

class AreaLight : public Light
{
public:
	AreaLight(Scene* scene, const vec3f& color, const vec3f& pos, const vec3f& u, const vec3f& v);
	vec3f getDirAndAtten(const vec3f& objPos, vec3f& attenuation,  double t) const override;
	vec3f shadowAttenuation(const vec3f& P, double t) const override { return vec3f(); }
	double distanceAttenuation(const vec3f& P) const override { return 0.0; }
	vec3f getColor(const vec3f& P) const override { return color; }
	vec3f getDirection(const vec3f& P) const override { return (pos - P).normalize(); }
	bool isAreaLight() const override { return true; }
	Photon* emitPhoton() const override;

protected:
	vec3f sample() const;
	
	vec3f pos;
	vec3f u, v;
	vec3f direction;
	double area;
};

double smoothstep(double edge0, double edge1, double x);

#endif // __LIGHT_H__
