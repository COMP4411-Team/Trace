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

protected:
	virtual void setTexCoords(Isect& isect) const;
};

class movingSphere : public Sphere
{
public:
	movingSphere(Scene* scene, Material* material, const vec3f& pos, const vec3f& target, double radius, double time0, double time1):
		Sphere(scene, material), pos(pos), target(target), radius(radius), time0(time0), time1(time1) { }

	bool intersect(const Ray& r, Isect& i) const override;
	bool hasBoundingBoxCapability() const override { return false; }
	void ComputeBoundingBox() override { }

private:
	vec3f getCurPosition(double time) const;
	
	vec3f pos, target;
	double radius;
	double time0, time1;
};

class SphereLight : public Sphere
{
public:
	SphereLight(Scene* scene, Material* mat, const vec3f& emission):
		Sphere(scene, mat) { hasEmission = true; this->emission = emission; }
	void calInfo();		// calculate position, radius and area, should be called after setTransform()
	Ray sample(vec3f& emit, double& pdf) const override;
	double getArea() const override { return area; }

protected:
	vec3f position;
	double radius;
	double area;	
};

#endif // __SPHERE_H__
