#include "Ray.h"
#include "material.h"
#include "light.h"

const double PI = 3.14159265358979323846;
const double INV_PI = 0.31830988618379067153;

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

void Material::initBRDF()
{
	alpha2 = roughness * roughness;
	alpha2 = alpha2 * alpha2;
	k = (roughness + 1.0) * (roughness + 1.0) / 8.0;
}

vec3f Material::pbs(Scene* scene, const Ray& ray, const Isect& isect) const
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

		color += prod(sampleBRDF(direction, -ray.getDirection(), normal, albedo), attenuation) * lambertian;
	}

	return color;
}

double Material::calNDF(double cosTheta) const
{
	double denom = cosTheta * cosTheta * (alpha2 - 1.0) + 1.0;
	denom = denom * denom * PI;
	return alpha2 / denom;
}

double Material::calGGX(double cosTheta) const
{
	return cosTheta / (cosTheta * (1.0 - k) + k);
}

vec3f Material::calFresnel(double cosTheta, const vec3f& F0) const
{
	return F0 + (vec3f(1.0, 1.0, 1.0) - F0) * pow(1.0 - cosTheta, 5.0);
}

vec3f Material::sampleBRDF(const vec3f& l, const vec3f& v, const vec3f& n, const vec3f& albedo) const
{
	vec3f h = ((l + v) / 2.0).normalize();
	vec3f F0 = vec3f(0.04, 0.04, 0.04) * (1.0 - metallic) + albedo * metallic;
	double vDotH = _max(v.dot(h), 0.0);
	double nDotH = _max(n.dot(h), 0.0);
	double nDotL = _max(n.dot(l), 0.0);
	double nDotV = _max(n.dot(v), 0.0);

	return albedo * INV_PI + calFresnel(vDotH, F0) * calNDF(nDotH) * calGGX(nDotL) * calGGX(nDotV) / 
		(4.0 * nDotL * nDotV + NORMAL_EPSILON);
}
