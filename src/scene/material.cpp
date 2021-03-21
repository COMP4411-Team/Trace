#include "ray.h"
#include "material.h"
#include "light.h"

// Apply the phong model to this point on the surface of the object, returning
// the color of that point.
vec3f Material::shade( Scene *scene, const ray& r, const isect& i ) const
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
		
		vec3f position = r.getPosition() + r.getDirection() * i.t;
		vec3f direction = light->getDirection(position);
		
		double lambertian = max(direction.dot(i.N), 0.0);
		vec3f attenuation = light->distanceAttenuation(position) * light->shadowAttenuation(position);

		// Really annoying that * has been overloaded as dot product... WHY????
		diffuse += lambertian * prod(prod(kd, light->getColor(position)), attenuation);

		vec3f vDir = -scene->getCamera()->getLook().normalize();
		vec3f h = (direction + vDir).normalize();

		// Using Blinn-Phong
		specular += pow(max(h.dot(i.N), 0.0), shininess) * 
			prod(ks, prod(light->getColor(position), attenuation));
	}

	return ke + prod(ka, ambient) + specular + diffuse;		// the direct illumination
}
