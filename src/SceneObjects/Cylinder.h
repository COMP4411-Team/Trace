#ifndef __CYLINDER_H__
#define __CYLINDER_H__

#include "../scene/scene.h"

class Cylinder
	: public MaterialSceneObject
{
public:
	Cylinder( Scene *scene, Material *mat , bool cap = true)
		: MaterialSceneObject( scene, mat ), capped( cap )
	{
	}

	virtual bool intersectLocal( const Ray& r, Isect& i ) const;
	virtual bool hasBoundingBoxCapability() const { return true; }

    virtual BoundingBox ComputeLocalBoundingBox()
    {
        BoundingBox localbounds;
		localbounds.min = vec3f(-1.0f, -1.0f, 0.0f);
		localbounds.max = vec3f(1.0f, 1.0f, 1.0f);
        return localbounds;
    }

    bool intersectBody( const Ray& r, Isect& i ) const;
	bool intersectCaps( const Ray& r, Isect& i ) const;

	void setEnableTexCoords(bool value) override { enableTexCoords = value; }	

protected:
	void setTexCoords(const vec3f& isectPos, Isect& isect) const;
	
	bool capped;
};

#endif // __CYLINDER_H__
