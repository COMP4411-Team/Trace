#ifndef __SPHERE_H__
#define __SPHERE_H__

#include "../scene/scene.h"

class Sphere
	: public MaterialSceneObject
{
public:
	Sphere( Scene *scene, Material *mat )
		: MaterialSceneObject( scene, mat )
	{
	}
    
	virtual bool intersectLocal( const Ray& r, Isect& i ) const;
	virtual bool hasBoundingBoxCapability() const { return true; }

    virtual BoundingBox ComputeLocalBoundingBox()
    {
        BoundingBox localbounds;
		localbounds.min = vec3f(-1.0f, -1.0f, -1.0f);
		localbounds.max = vec3f(1.0f, 1.0f, 1.0f);
        return localbounds;
    }

	void setEnableTexCoords(bool value) override { enableTexCoords = value; }
};

class SphereLight : public Sphere
{
public:
	SphereLight(Scene* scene, Material* mat, const vec3f& emission):
		Sphere(scene, mat), emission(emission) { hasEmission = true; }
	void calInfo();		// calculate position, radius and area, should be called after setTransform()
	Ray sample(vec3f& emit, double& pdf) const override;
	double getArea() const override { return area; }

protected:
	vec3f emission;
	vec3f position;
	double radius;
	double area;	
};

#endif // __SPHERE_H__
