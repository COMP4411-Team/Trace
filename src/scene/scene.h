//
// scene.h
//
// The Scene class and the geometric types that it can contain.
//

#ifndef __SCENE_H__
#define __SCENE_H__

#include <list>
#include <algorithm>
#include <vector>
#include <random>

class Geometry;
class Skybox;
using namespace std;

#include "Ray.h"
#include "material.h"
#include "camera.h"
#include "../vecmath/vecmath.h"
#include "SolidTexture.h"

extern std::mt19937_64 rng;
extern uniform_real_distribution<double> unif;

class Light;
class Scene;

class Texture
{
public:
	virtual ~Texture();

	virtual vec3f sample(double u, double v) const;
	virtual vec3f sample(const TexCoords& coords) const;
	
	int height, width;
	unsigned char* data{nullptr};
};

class HFmap
{
public:
	HFmap(unsigned char* m, int h, int w) :map(m), height(h), width(w) {}
	~HFmap();

	double getH(int x, int y) const;
	vec3f getC(int x, int y) const;


	int height, width, hf;
	unsigned char* map{ nullptr };

};

class SceneElement
{
public:
	virtual ~SceneElement() {}

	Scene *getScene() const { return scene; }

protected:
	SceneElement( Scene *s )
		: scene( s ) {}

    Scene *scene;
};

class BoundingBox
{
public:
	vec3f min;
	vec3f max;

	void operator=(const BoundingBox& target);

	// Does this bounding box intersect the target?
	bool intersects(const BoundingBox &target) const;
	
	// does the box contain this point?
	bool intersects(const vec3f& point) const;

	// if the ray hits the box, put the "t" value of the intersection
	// closest to the origin in tMin and the "t" value of the far intersection
	// in tMax and return true, else return false.
	bool intersect(const Ray& r, double& tMin, double& tMax) const;
};


// Node for BVH
class BVHNode
{
public:
	~BVHNode() { delete left; delete right; }

	bool intersect(const Ray& ray, Isect& isect) const;
	
	vector<Geometry*> objects;
	BoundingBox aabb;
	BVHNode* left{nullptr}, *right{nullptr};
};


// Bounding volume hierarchy
class BVH
{
public:
	~BVH() { delete root; }
	void build(const list<Geometry*>& objects);		// objects must support bounding box
	void buildHelper(BVHNode* cur, const vector<Geometry*>& objects);
	BoundingBox calMaxBoundingBox(const vector<Geometry*>& objects);
	
	bool intersect(const Ray& ray, Isect& isect) const;
	
	BVHNode* root{nullptr};
	int threshold{5};	// if the objects contained in a node is less than threshold, stop subdivision
};


class TransformNode
{
public:

    // information about this node's transformation
    mat4f    xform;
	mat4f    inverse;
	mat3f    normi;

    // information about parent & children
    TransformNode *parent;
    list<TransformNode*> children;
    
public:
   	typedef list<TransformNode*>::iterator          child_iter;
	typedef list<TransformNode*>::const_iterator    child_citer;

    ~TransformNode()
    {
        for(child_iter c = children.begin(); c != children.end(); ++c )
            delete (*c);
    }

    TransformNode *createChild(const mat4f& xform)
    {
        TransformNode *child = new TransformNode(this, xform);
        children.push_back(child);
        return child;
    }
    
    // Coordinate-Space transformation
    vec3f globalToLocalCoords(const vec3f &v)
    {
        return inverse * v;
    }

    vec3f localToGlobalCoords(const vec3f &v)
    {
        return xform * v;
    }

    vec4f localToGlobalCoords(const vec4f &v)
    {
        return xform * v;
    }

    vec3f localToGlobalCoordsNormal(const vec3f &v)
    {
        return (normi * v).normalize();
    }

protected:
    // protected so that users can't directly construct one of these...
    // force them to use the createChild() method.  Note that they CAN
    // directly create a TransformRoot object.
    TransformNode(TransformNode *parent, const mat4f& xform )
        : children()
    {
        this->parent = parent;
        if (parent == NULL)
            this->xform = xform;
        else
            this->xform = parent->xform * xform;
        
        inverse = this->xform.inverse();
        normi = this->xform.upper33().inverse().transpose();
    }
};

class TransformRoot : public TransformNode
{
public:
    TransformRoot()
        : TransformNode(NULL, mat4f()) {}
};

// A Geometry object is anything that has extent in three dimensions.
// It may not be an actual visible scene object.  For example, hierarchical
// spatial subdivision could be expressed in terms of Geometry instances.
class Geometry
	: public SceneElement
{
public:
    // intersections performed in the global coordinate space.
    virtual bool intersect(const Ray&r, Isect&i) const;
    
    // intersections performed in the object's local coordinate space
    // do not call directly - this should only be called by intersect()
	virtual bool intersectLocal( const Ray& r, Isect& i ) const;

	virtual bool hasTexCoords() const { return enableTexCoords; }
	virtual void setEnableTexCoords(bool value) { enableTexCoords = value; }

	// For area light sampling
	virtual Ray sample(vec3f& emit, double& pdf) const { return Ray{vec3f(), vec3f()}; }
	virtual double getArea() const { return 0.0; }
	virtual vec3f getEmission() const { return emission; }

	virtual bool hasBoundingBoxCapability() const;
	const BoundingBox& getBoundingBox() const { return bounds; }
	virtual void ComputeBoundingBox()
    {
        // take the object's local bounding box, transform all 8 points on it,
        // and use those to find a new bounding box.

        BoundingBox localBounds = ComputeLocalBoundingBox();
        
        vec3f min = localBounds.min;
		vec3f max = localBounds.max;

		vec4f v, newMax, newMin;

		v = transform->localToGlobalCoords( vec4f(min[0], min[1], min[2], 1) );
		newMax = v;
		newMin = v;
		v = transform->localToGlobalCoords( vec4f(max[0], min[1], min[2], 1) );
		newMax = maximum(newMax, v);
		newMin = minimum(newMin, v);
		v = transform->localToGlobalCoords( vec4f(min[0], max[1], min[2], 1) );
		newMax = maximum(newMax, v);
		newMin = minimum(newMin, v);
		v = transform->localToGlobalCoords( vec4f(max[0], max[1], min[2], 1) );
		newMax = maximum(newMax, v);
		newMin = minimum(newMin, v);
		v = transform->localToGlobalCoords( vec4f(min[0], min[1], max[2], 1) );
		newMax = maximum(newMax, v);
		newMin = minimum(newMin, v);
		v = transform->localToGlobalCoords( vec4f(max[0], min[1], max[2], 1) );
		newMax = maximum(newMax, v);
		newMin = minimum(newMin, v);
		v = transform->localToGlobalCoords( vec4f(min[0], max[1], max[2], 1) );
		newMax = maximum(newMax, v);
		newMin = minimum(newMin, v);
		v = transform->localToGlobalCoords( vec4f(max[0], max[1], max[2], 1) );
		newMax = maximum(newMax, v);
		newMin = minimum(newMin, v);
		
		bounds.max = vec3f(newMax);
		bounds.min = vec3f(newMin);
    }

    // default method for ComputeLocalBoundingBox returns a bogus bounding box;
    // this should be overridden if hasBoundingBoxCapability() is true.
    virtual BoundingBox ComputeLocalBoundingBox() { return BoundingBox(); }

    void setTransform(TransformNode *transform) { this->transform = transform; }
    
	Geometry( Scene *scene ) 
		: SceneElement( scene ) {}

	bool enableBumpMap{false};
	bool enableNormalMap{false};
	bool enableDisplacementMap{false};
	bool enableDiffuseMap{false};
	bool hasEmission{false};
	bool enableSolidTexture{false};
	
	Texture bumpMap;
	Texture diffuseMap;
	Texture normalMap;
	SolidTexture* solidTexture{nullptr};
	vec3f emission;

protected:
	BoundingBox bounds;
    TransformNode *transform{nullptr};
	bool enableTexCoords{false};
};

// A SceneObject is a real actual thing that we want to model in the 
// world.  It has extent (its Geometry heritage) and surface properties
// (its material binding).  The decision of how to store that material
// is left up to the subclass.
class SceneObject
	: public Geometry
{
public:
	virtual const Material& getMaterial() const = 0;
	virtual void setMaterial( Material *m ) = 0;

protected:
	SceneObject( Scene *scene )
		: Geometry( scene ) {}
};

// A simple extension of SceneObject that adds an instance of Material
// for simple material bindings.
class MaterialSceneObject
	: public SceneObject
{
public:
	virtual ~MaterialSceneObject() { if( material ) delete material; }

	virtual const Material& getMaterial() const { return *material; }
	virtual void setMaterial( Material *m )	{ material = m; }

protected:
	MaterialSceneObject( Scene *scene, Material *mat ) 
		: SceneObject( scene ), material( mat ) {}
    //	MaterialSceneObject( Scene *scene ) 
	//	: SceneObject( scene ), material( new Material ) {}

	Material *material;
};

class Scene
{
public:
	typedef list<Light*>::iterator 			liter;
	typedef list<Light*>::const_iterator 	cliter;

	typedef list<Geometry*>::iterator 		giter;
	typedef list<Geometry*>::const_iterator cgiter;

    TransformRoot transformRoot;

public:
	Scene() 
		: transformRoot(), objects(), lights() {}
	virtual ~Scene();

	void add( Geometry* obj )
	{
		if (obj == nullptr)
			return;
		obj->ComputeBoundingBox();
		objects.push_back( obj );
	}
	void add( Light* light )
	{ lights.push_back( light ); }

	bool intersect( const Ray& r, Isect& i ) const;
	bool bvhIntersect(const Ray& ray, Isect& isect) const;	// use BVH for acceleration
	void initScene();

	list<Light*>::const_iterator beginLights() const { return lights.begin(); }
	list<Light*>::const_iterator endLights() const { return lights.end(); }

	Ray uniformSampleOneLight(vec3f& emit, double& pdf);
	
	bool loadHFmap(const string& filename);
	bool HFmapLoaded() { if (hfmap != nullptr) return true; else return false; }
	bool enableHField{ true };//control HField
	HFmap* hfmap{ nullptr };
        
	Camera *getCamera() { return &camera; }

	double lightScale{10.0};
	bool useSkybox{false};
	Skybox* skybox{nullptr};
	bool enableFasterShadow{ false }; //Acceleration of shadow attenuation
	

private:
    list<Geometry*> objects;
	list<Geometry*> nonboundedobjects;
	list<Geometry*> boundedobjects;
    list<Light*> lights;
	list<Geometry*> emittingObjects;
    Camera camera;
	
	// Each object in the scene, provided that it has hasBoundingBoxCapability(),
	// must fall within this bounding box.  Objects that don't have hasBoundingBoxCapability()
	// are exempt from this requirement.
	BoundingBox sceneBounds;

	BVH bvh;
};


inline double _max(double a, double b)
{
	return a > b ? a : b;
}

inline double _min(double a, double b)
{
	return a < b ? a : b;
}

inline double _abs(double a)
{
	return a < 0.0 ? -a : a;
}

inline void _swap(double& a, double& b)
{
	double tmp = b;
	b = a;
	a = tmp;
}

inline double getUniformReal()
{
	// return unif(rng);
	return ((double)rand() / RAND_MAX);
}

#endif // __SCENE_H__
