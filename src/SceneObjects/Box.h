#ifndef __BOX_H__
#define __BOX_H__

#include "../scene/scene.h"

class Box
	: public MaterialSceneObject
{
public:
	Box( Scene *scene, Material *mat )
		: MaterialSceneObject( scene, mat )
	{
	}

	virtual bool intersectLocal( const Ray& r, Isect& i ) const;
	virtual bool hasBoundingBoxCapability() const { return true; }
	virtual void setEnableTexCoords(bool value) override;
    virtual BoundingBox ComputeLocalBoundingBox()
    {
        BoundingBox localbounds;
        localbounds.max = vec3f(0.5, 0.5, 0.5);
		localbounds.min = vec3f(-0.5, -0.5, -0.5);
        return localbounds;
    }
};

class Skybox : public Box
{
public:
	Skybox(Scene* scene, Material* material);
	vec3f getColor(const Ray& ray, const Isect& isect) const;
	void initialize(double width);
	
	Texture back, front, bottom, top, left, right;
	bool initialized{false};
};

#endif // __BOX_H__
