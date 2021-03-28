#include "Ray.h"
#include "material.h"
#include "light.h"
#include <random>

const double PI = 3.14159265358979323846;
const double PI_OVER_2 = 1.57079632679489661923;
const double PI_OVER_4 = 0.78539816339744830961;
const double INV_PI = 0.31830988618379067153;

inline double getRandomReal()
{
	// return unif(rng);
	return ((double) rand() / RAND_MAX);		// for performance
}

inline vec3f Material::localToWorld(const vec3f& v, const vec3f& n)
{
	vec3f t;
	if (_abs(n[0]) > _abs(n[1]))
		t = vec3f(n[2], 0.0, -n[0]).normalize();
	else
		t = vec3f(0.0, n[2], -n[1]).normalize();
	vec3f b(t.cross(n));
	return v[0] * b + v[1] * t + v[2] * n;
}

vec3f Material::uniformSampleHemisphere()
{
	double x1 = getRandomReal(), x2 = getRandomReal();
	double z = _abs(1.0 - 2.0 * x1);
	double r = sqrt(1.0 - z * z), phi = 2.0 * PI * x2;
	return vec3f(r * cos(phi), r * sin(phi), z);
}

// Apply the phong model to this point on the surface of the object, returning
// the color of that point.
vec3f Material::shade( Scene *scene, const Ray& r, const Isect& i ) const
{
	// YOUR CODE HERE

	// For now, this method just returns the diffuse color of the object.
	// This gives a single matte color for every distinct surface in the
	// scene, and that's it.  Simple, but enough to get you started.
	// (It's also inconsistent with the phong model...)

	// Your mission is to fill in this method with the rest of the phong
	// shading model, including the contributions of all the light sources.
    // You will need to call both distanceAttenuation() and shadowAttenuation()
    // somewhere in your code in order to compute shadows and light falloff.

	vec3f diffuse, specular, ambient;

	for (auto iter = scene->beginLights(); iter != scene->endLights(); ++iter)
	{
		Light* light = *iter;

		if (typeid(*light) == typeid(AmbientLight))
		{
			ambient += light->getColor(vec3f());
			continue;
		}
		
		vec3f position = r.at(i.t) + i.N * DISPLACEMENT_EPSILON;
		vec3f normal = i.N;
		vec3f direction = light->getDirection(position);
		vec3f diffuseColor = getDiffuseColor(i);

		if (i.obj->enableBumpMap)
			normal = perturbSurfaceNormal(i);

		if (i.obj->enableNormalMap)
			normal = (i.tbn * i.obj->normalMap.sample(i.texCoords)).normalize();
		
		double lambertian = max(direction.dot(normal), 0.0);
		vec3f attenuation = light->distanceAttenuation(position) * light->shadowAttenuation(position);

		// Really annoying that * has been overloaded as dot product... WHY????
		diffuse += lambertian * prod(prod(diffuseColor, light->getColor(position)), attenuation);

		vec3f h = (direction - r.getDirection()).normalize();

		// Using Blinn-Phong
		specular += pow(max(h.dot(normal), 0.0), shininess * 256.0) * 
			prod(ks, prod(light->getColor(position), attenuation));
	}

	return ke + prod(ka, ambient) + specular + diffuse;		// the direct illumination
	
}

vec3f Material::getDiffuseColor(const Isect& isect) const
{
	auto* object = isect.obj;
	if (isect.hasTexCoords && object->enableDiffuseMap)
		return object->diffuseMap.sample(isect.texCoords) / 255.0;
	return kd;
}

vec3f Material::perturbSurfaceNormal(const Isect& isect) const
{
	double offset = 0.005, factor = 0.05;
	double h = isect.obj->bumpMap.sample(isect.texCoords).length();
	double h1 = isect.obj->bumpMap.sample(isect.texCoords.u + offset, isect.texCoords.v).length();
	double h2 = isect.obj->bumpMap.sample(isect.texCoords.u, isect.texCoords.v + offset).length();

	vec3f normal(factor * (h - h1), factor * (h - h2), 1.0);
	return (isect.tbn * normal).normalize();
}

vec3f Material::brdf(const vec3f& wi, const vec3f& wo, const vec3f& n) const
{
	if (!isTransmissive && wi.dot(n) > 0.0)
		return kd * INV_PI;
	if (isTransmissive && wi.dot(wo) < 0.0)
		return kt * INV_PI;
	return vec3f();
}

vec3f Material::sample(const vec3f& wo, const vec3f& n, double& pdf) const
{
	vec3f wi = localToWorld(uniformSampleHemisphere(), n);
	pdf = 0.5 * INV_PI;
	if (isTransmissive && wo.dot(n) > 0.0)
		return -wi;
	return wi;
}

// Sample the nest direction for path tracing for transparent materials
// v is from the previous vertex to the intersection point
Ray Material::sampleBTDF(const Ray& ray, const Isect& isect) const
{
	double n1 = 1.0, n2 = index;
	vec3f v = ray.getDirection();
	if (v.dot(isect.N) > 0.0)
		_swap(n1, n2);

	double F0 = (n1 - n2) / (n1 + n2);
	F0 *= F0;
	double theta = _abs(v.dot(isect.N));
	double fresnel = F0 + (1.0 - F0) * pow(theta, 5.0);

	double eta = n1 / n2;
	double cosTheta2 = 1.0 - eta * eta * (1.0 - theta * theta);
	if (cosTheta2 < 0.0 || getRandomReal() < fresnel)
	{
		return ray.reflect(isect);
	}
	else
	{
		Ray wi{vec3f(), vec3f()};
		ray.refract(isect, wi);
		return wi;
	}
}


vec3f Microfacet::shade(Scene* scene, const Ray& ray, const Isect& isect) const
{
	vec3f color;

	for (auto iter = scene->beginLights(); iter != scene->endLights(); ++iter)
	{
		Light* light = *iter;
		vec3f albedo = this->albedo;

		auto* object = isect.obj;
		if (isect.hasTexCoords && object->enableDiffuseMap)
			albedo = object->diffuseMap.sample(isect.texCoords) / 255.0;

		if (typeid(*light) == typeid(AmbientLight))
		{
			color += prod(light->getColor(vec3f()), albedo);
			continue;
		}
		
		vec3f position = ray.at(isect.t) + isect.N * DISPLACEMENT_EPSILON;
		vec3f normal = isect.N;
		vec3f direction = light->getDirection(position);

		if (object->enableBumpMap)
			normal = perturbSurfaceNormal(isect);

		if (object->enableNormalMap)
			normal = (isect.tbn * isect.obj->normalMap.sample(isect.texCoords)).normalize();
		
		double lambertian = max(direction.dot(normal), 0.0);
		vec3f attenuation = light->distanceAttenuation(position) * light->shadowAttenuation(position);

		color += prod(brdf(direction, -ray.getDirection(), normal), attenuation) * lambertian;
	}

	return color;
}

double Microfacet::calNDF(double cosTheta) const
{
	double denom = cosTheta * cosTheta * (alpha2 - 1.0) + 1.0;
	denom = denom * denom * PI;
	return alpha2 / denom;
}

double Microfacet::calGGX(double cosTheta) const
{
	return cosTheta / (cosTheta * (1.0 - k) + k);
}

vec3f Microfacet::calFresnel(double cosTheta, const vec3f& F0) const
{
	return F0 + (vec3f(1.0, 1.0, 1.0) - F0) * pow(1.0 - cosTheta, 5.0);
}

// Sample the BRDF
vec3f Microfacet::brdf(const vec3f& wi, const vec3f& wo, const vec3f& n) const
{
	vec3f h = ((wi + wo) / 2.0).normalize();
	vec3f F0 = vec3f(0.04, 0.04, 0.04) * (1.0 - metallic) + albedo * metallic;
	double vDotH = _max(wo.dot(h), 0.0);
	double nDotH = _max(n.dot(h), 0.0);
	double nDotL = _max(n.dot(wi), 0.0);
	double nDotV = _max(n.dot(wo), 0.0);

	return albedo * INV_PI + calFresnel(vDotH, F0) * calNDF(nDotH) * calGGX(nDotL) * calGGX(nDotV) / 
		(4.0 * nDotL * nDotV + NORMAL_EPSILON);
}

// Sample the next direction for path tracing for opaque materials
vec3f Microfacet::sample(const vec3f& wo, const vec3f& n, double& pdf) const
{
	// TODO: add importance sampling
	vec3f localDir = cosineSampleHemisphere();
	pdf = cosineHemispherePdf(localDir[2]);
	return localToWorld(localDir, n);
}

Microfacet::Microfacet(const vec3f& albedo, double roughness, double metallic):
	Material(), albedo(albedo), roughness(roughness), metallic(metallic)
{
	alpha2 = roughness * roughness;
	alpha2 = alpha2 * alpha2;
	k = (roughness + 1.0) * (roughness + 1.0) / 8.0;
}

// The result is stored in the x and y component
// Ref: PBRT-v3
inline vec3f Microfacet::concentricSampleDisk()
{
	double x = 2.0 * getRandomReal() - 1.0, y = 2.0 * getRandomReal() - 1.0;
	if (x == 0.0 && y == 0.0)
		return vec3f();
	double theta, r;
	if (_abs(x) > _abs(y))
	{
		r = x;
		theta = PI_OVER_4 * (y / x);
	}
	else
	{
		r = y;
		theta = PI_OVER_2 - PI_OVER_4 * (x / y);
	}
	return vec3f(r * cos(theta), r * sin(theta), 0.0);
}

// Ref: PBRT-v3
inline vec3f Microfacet::cosineSampleHemisphere()
{
    vec3f d = concentricSampleDisk();
    double z = sqrt(_max(0.0, 1 - d[0] * d[0] - d[1] * d[1]));
    return vec3f(d[0], d[1], z);
}

inline double Microfacet::cosineHemispherePdf(double cosTheta)
{
	return cosTheta * INV_PI;
}