#include "Ray.h"
#include "material.h"
#include "light.h"

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
		vec3f direction = light->getDirection(position);
		vec3f diffuseColor = getDiffuseColor(i);
		
		double lambertian = max(direction.dot(i.N), 0.0);
		vec3f attenuation = light->distanceAttenuation(position) * light->shadowAttenuation(position);

		// Really annoying that * has been overloaded as dot product... WHY????
		diffuse += lambertian * prod(prod(diffuseColor, light->getColor(position)), attenuation);

		vec3f h = (direction - r.getDirection()).normalize();

		// Using Blinn-Phong
		specular += pow(max(h.dot(i.N), 0.0), shininess * 256.0) * 
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
