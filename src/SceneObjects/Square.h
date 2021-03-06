#ifndef __SQUARE_H__
#define __SQUARE_H__

#include "../scene/scene.h"

class Square
	: public MaterialSceneObject
{
public:
	Square( Scene *scene, Material *mat )
		: MaterialSceneObject( scene, mat )
	{
	}

	virtual bool intersectLocal( const Ray& r, Isect& i ) const;
	virtual bool hasBoundingBoxCapability() const { return true; }

    virtual BoundingBox ComputeLocalBoundingBox()
    {
        BoundingBox localbounds;
        localbounds.min = vec3f(-0.5f, -0.5f, -RAY_EPSILON);
		localbounds.max = vec3f(0.5f, 0.5f, RAY_EPSILON);
        return localbounds;
    }

	void setEnableTexCoords(bool value) override { enableTexCoords = value; }
};

#endif // __SQUARE_H__
