#include "Ray.h"
#include "material.h"
#include "light.h"


// Apply the phong model to this point on the surface of the object, returning
// the color of that point.
vec3f Material::shade( Scene *scene, const Ray& r, const Isect& i) const
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

	vec3f diffuse, specular, ambient, accum;		// accum for area lights
	vec3f position = r.at(i.t) + i.N * DISPLACEMENT_EPSILON;
	vec3f normal = i.N;

	if (i.obj->enableBumpMap)
		normal = perturbSurfaceNormal(i);
	if (i.obj->enableNormalMap)
		normal = (i.tbn * i.obj->normalMap.sample(i.texCoords)).normalize();
	
	vec3f diffuseColor = getDiffuseColor(r, i);

	// Punctual light
	for (auto iter = scene->beginLights(); iter != scene->endLights(); ++iter)
	{
		Light* light = *iter;
		if (typeid(*light) == typeid(AmbientLight))
		{
			ambient += light->getColor(vec3f());
			continue;
		}

		if (light->isAreaLight() && scene->enableDistributed)
		{
			vec3f tmp;
			for (int i = 0; i < scene->numChildRay; ++i)
			{
				vec3f atten;
				vec3f lDir = light->getDirAndAtten(position, atten, r.getTime());
				double lambertian = max(lDir.dot(normal), 0.0);
				vec3f h = (lDir - r.getDirection()).normalize();
				tmp += lambertian * prod(prod(diffuseColor, light->getColor(position)), atten);
				tmp += pow(max(h.dot(normal), 0.0), shininess * 256.0) * 
					prod(ks, prod(light->getColor(position), atten));
			}
			accum += tmp / scene->numChildRay;
			continue;
		}
		
		vec3f direction = light->getDirection(position);
		if (scene->enableFasterShadow && direction.dot(normal) < 0) {
			continue;	
		}
		
		double lambertian = max(direction.dot(normal), 0.0);
		vec3f shadowA= light->shadowAttenuation(position, r.getTime());
		vec3f attenuation = light->distanceAttenuation(position) * shadowA;
		vec3f h = (direction - r.getDirection()).normalize();
		
		// Really annoying that * has been overloaded as dot product... WHY????
		diffuse += lambertian * prod(prod(diffuseColor, light->getColor(position)), attenuation);

		// Using Blinn-Phong
		if (!i.obj->enableAnisotropicSpecular)
		{
			specular += pow(max(h.dot(normal), 0.0), shininess * 256.0) * 
				prod(ks, prod(light->getColor(position), attenuation));
		}
		else
		{
			vec3f anisoDir = (i.tbn * i.obj->anisoSpecular.sample(i.texCoords)).normalize();
			double hDotA = h.dot((normal + anisoDir).normalize());
			specular += pow(_max(0.0, hDotA), shininess * 128.0) * prod(ks, prod(light->getColor(position), attenuation));
		}
	}

	return ke + prod(ka, ambient) + specular + diffuse + accum;		// the direct illumination
}

vec3f Material::getDiffuseColor(const Ray& ray, const Isect& isect) const
{
	auto* object = isect.obj;
	if (object->enableSolidTexture)
		return object->solidTexture->sample(ray.at(isect.t));
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

vec3f Material::randomReflect(const vec3f& d, const vec3f& n) const
{
	vec3f r = reflect(d, n);
	if (glossiness > 0.0) 
		return (r + glossiness * uniformSampleSphere().normalize()).normalize();
	return r;
}

// wo is from the intersection to the previous vertex
double Material::fresnel(const vec3f& wo, const vec3f& n) const
{
	double n1 = 1.0, n2 = index;
	if (wo.dot(n) < 0.0)
		_swap(n1, n2);

	double F0 = (n1 - n2) / (n1 + n2);
	F0 *= F0;
	double cosTheta = _abs(wo.dot(n));

	if (n1 > n2)		// from the refractive index to the air
	{
		double eta = n1 / n2;
		double sinTheta2 = eta * eta * (1.0 - cosTheta * cosTheta);
		if (sinTheta2 > 1.0)		// total internal reflection
			return 1.0;
		// cosTheta should always be the cosine of the larger angle relative to the normal
		cosTheta = sqrt(1 - sinTheta2);
	}
	
	return F0 + (1.0 - F0) * pow(1.0 - cosTheta, 5.0);
}

// wo is from the intersection to the previous vertex
vec3f Material::fresnelReflective(const vec3f& wo, const vec3f& n) const
{	
	return kr + (vec3f(1.0) - kr) * fresnel(wo, n);
}

vec3f Material::bxdf(const vec3f& wi, const vec3f& wo, const vec3f& n) const
{
	if (wi.dot(n) > 0.0)
		return kd * INV_PI;
	return vec3f(0.0);
}

vec3f Material::sample(const vec3f& wo, const vec3f& n, double& pdf) const
{
	vec3f wi = localToWorld(uniformSampleHemisphere(), n);
	pdf = wi.dot(n) * INV_PI;
	return wi;
}

vec3f Material::sampleF(const vec3f& wo, vec3f& wi, const vec3f& n, double& pdf) const
{
	wi = sample(wo, n, pdf);
	return bxdf(wi, wo, n);
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
		vec3f attenuation = light->distanceAttenuation(position) * light->shadowAttenuation(position, ray.getTime());

		color += prod(bxdf(direction, -ray.getDirection(), normal), attenuation) * lambertian;
	}

	return color;
}

// Trowbridge-Reitz/GGX NDF
double Microfacet::calD(double cosTheta) const
{
	double denom = cosTheta * cosTheta * (alpha2 - 1.0) + 1.0;
	denom = denom * denom * PI;
	return alpha2 / denom;
}

// Schlick model
double Microfacet::calG(double cosTheta) const
{
	return cosTheta / (cosTheta * (1.0 - k) + k);
}

vec3f Microfacet::calF(double cosTheta, const vec3f& F0) const
{
	return F0 + (vec3f(1.0, 1.0, 1.0) - F0) * pow(1.0 - cosTheta, 5.0);
}

vec3f Microfacet::bxdf(const vec3f& wi, const vec3f& wo, const vec3f& n) const
{
	vec3f h = ((wi + wo) / 2.0).normalize();
	vec3f F0 = vec3f(0.04, 0.04, 0.04) * (1.0 - metallic) + albedo * metallic;
	double vDotH = _max(wo.dot(h), 0.0);
	double nDotH = _max(n.dot(h), 0.0);
	double nDotL = _max(n.dot(wi), 0.0);
	double nDotV = _max(n.dot(wo), 0.0);

	return albedo * INV_PI + calF(vDotH, F0) * calD(nDotH) * calG(nDotL) * calG(nDotV) / 
		(4.0 * nDotL * nDotV + NORMAL_EPSILON);
}

vec3f Microfacet::sample(const vec3f& wo, const vec3f& n, double& pdf) const
{
	vec3f localDir = cosineSampleHemisphere();
	pdf = localDir[2] * INV_PI;
	return localToWorld(localDir, n);
}

vec3f Microfacet::sampleF(const vec3f& wo, vec3f& wi, const vec3f& n, double& pdf) const
{
	wi = sample(wo, n, pdf);
	return bxdf(wi, wo, n);
}

Microfacet::Microfacet(const vec3f& albedo, double roughness, double metallic):
	Material(), albedo(albedo), roughness(roughness), metallic(metallic)
{
	alpha2 = roughness * roughness;
	alpha2 = alpha2 * alpha2;
	k = (roughness + 1.0) * (roughness + 1.0) / 8.0;
}


vec3f FresnelSpecular::bxdf(const vec3f& wi, const vec3f& wo, const vec3f& n) const
{
	return albedo;
}

vec3f FresnelSpecular::sample(const vec3f& wo, const vec3f& n, double& pdf) const
{
	vec3f wi = localToWorld(uniformSampleHemisphere(), n);
	pdf = wi.dot(n) * INV_PI;
	return albedo;
}

vec3f FresnelSpecular::sampleF(const vec3f& wo, vec3f& wi, const vec3f& n, double& pdf) const
{
	double f = fresnel(wo, n);
	double specularProb = (1.0 - f) * metallic + f;
	double refractProb = (1.0 - specularProb) / (1.0 - metallic) * translucency;
	double r = getRandomReal();
	vec3f diffuseRay = localToWorld(uniformSampleHemisphere(), n);
	
	if (r < specularProb)
	{
		pdf = specularProb;
		wi = reflect(-wo, n);
		wi = roughness2 * diffuseRay + (1.0 - roughness2) * wi;
		return vec3f(1.0);
	}
	if (r < specularProb + refractProb)
	{
		pdf = refractProb;
		double eta = index;
		if (wo.dot(n) > 0.0)
			eta = 1.0 / eta;
		refract(-wo, n, wi, eta);
		wi = roughness2 * diffuseRay + (1.0 - roughness2) * wi;
		return vec3f(1.0);
	}
	pdf = 1.0 - specularProb - refractProb;
	wi = diffuseRay;
	return albedo;
}
