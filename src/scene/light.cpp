#include <cmath>

#include "light.h"
#include "../photon_map/Photon.h"

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

Photon* DirectionalLight::emitPhoton() const
{
	int idx = round(getRandomReal() * (cells.size() - 1));
	auto* photon = new Photon;
	double x = cells[idx] % mapSize + getRandomReal(), y = cells[idx] / mapSize + getRandomReal();
	photon->position = unproject(x, y);
	photon->direction = orientation;
	photon->power = color * PI * sceneRadius * sceneRadius;
	
	return photon;
}

void DirectionalLight::buildProjectionMap()
{
	delete projMap;
	projMap = new bool[mapSize * mapSize];
	for (int i = 0; i < mapSize * mapSize; ++i)
		projMap[i] = false;
	cells.clear();

	sceneRadius = (scene->sceneBounds.max - scene->sceneBounds.min).length() / 2.0;
	position = (scene->sceneBounds.max + scene->sceneBounds.min) / 2.0 - orientation * sceneRadius;

	for (auto* geometry : scene->boundedobjects)
	{
		auto* object = dynamic_cast<MaterialSceneObject*>(geometry);
		if (object == nullptr)
			continue;
		const auto& material = object->getMaterial();
		if (material.kr.iszero() && material.kt.iszero())
			continue;
		const auto& box = object->getBoundingBox();

		vec3f max = box.max, min = box.min;
		vec3f vertices[] = 
		{
			max, min,
			{max[0], max[1], min[2]},
			{max[0], min[1], max[2]},
			{min[0], max[1], max[2]},
			{min[0], min[1], max[2]},
			{min[0], max[1], min[2]},
			{max[0], min[1], min[2]}
		};

		for (int i = 0; i < 8; ++i)
		{
			vec3f proj = project(vertices[i]);
			int x = proj[0] + mapSize / 2;
			int y = proj[1] + mapSize / 2;
			projMap[x + y * mapSize] = true;
		}
	}

	for (int i = 0; i < mapSize * mapSize; ++i)
		if (projMap[i])
			cells.push_back(i);
}

vec3f DirectionalLight::project(const vec3f& pos) const
{
	vec3f vec = pos - position;
	vec3f proj = vec - vec.dot(orientation) * orientation;
	return vec3f(proj.dot(u), proj.dot(v), 0.0) / (sceneRadius * 2) * mapSize;
}

vec3f DirectionalLight::unproject(double x, double y) const
{
	double tmp = mapSize / 2;
	double uu = (x - tmp) / tmp * sceneRadius;
	double vv = (y - tmp) / tmp * sceneRadius;
	return uu * u + vv * v + position;
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

Photon* PointLight::emitPhoton() const
{
	auto* photon = new Photon;
	photon->position = position;
	photon->direction = uniformSampleSphere().normalize();
	photon->power = color * PI_4;
	return photon;
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

AreaLight::AreaLight(Scene* scene, const vec3f& color, const vec3f& pos, const vec3f& u, const vec3f& v):
	Light(scene, color), pos(pos), u(u), v(v), direction(u.cross(v).normalize()), area(u.cross(v).length()) { }

vec3f AreaLight::getDirAndAtten(const vec3f& objPos, vec3f& attenuation, double t) const
{
	vec3f lDir = sample() - objPos;
	double distAtten = scene->lightScale / (lDir.length_squared() + LIGHT_EPSILON);
	Ray r(objPos, lDir, t);
	Isect isect;
	if (scene->bvhIntersect(r, isect))
		attenuation = isect.getMaterial().kt;
	else
		attenuation = vec3f(1.0);
	attenuation *= distAtten;
    return lDir.normalize();
}

Photon* AreaLight::emitPhoton() const
{
	auto* photon = new Photon;
	photon->position = sample();
	photon->direction = localToWorld(cosineSampleHemisphere(), direction);
	photon->power = color * (PI * area);
	return photon;
}

vec3f AreaLight::sample() const
{
	return pos + getRandomReal() * u + getRandomReal() * v;
}
