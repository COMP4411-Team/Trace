//
// ray.h
//
// The low-level classes used by ray tracing: the ray and isect classes.
//

#ifndef __RAY_H__
#define __RAY_H__

#include "../vecmath/vecmath.h"
#include "material.h"

class SceneObject;

class TexCoords
{
public:
	TexCoords(): u(0.0), v(0.0) { }
	TexCoords(double u, double v): u(u), v(v) { }
	double u, v;
};

// A ray has a position where the ray starts, and a direction (which should
// always be normalized!)

class Ray {
public:
	Ray( const vec3f& pp, const vec3f& dd )
		: p( pp ), d( dd.normalize() ) {}
	Ray( const Ray& other ) 
		: p( other.p ), d( other.d ) {}
	~Ray() {}

	Ray& operator =( const Ray& other ) 
	{ p = other.p; d = other.d; return *this; }

	vec3f at( double t ) const
	{ return p + (t*d); }

	vec3f getPosition() const { return p; }
	vec3f getDirection() const { return d; }

	Ray reflect(const Isect& isect) const;
	bool refract(const Isect& isect, Ray& out) const;

protected:
	vec3f p;
	vec3f d;
};

// The description of an intersection point.

class Isect
{
public:
    Isect()
        : obj( NULL ), t( 0.0 ), N(), material(0) {}

    ~Isect()
    {
        delete material;
    }
    
    void setObject( SceneObject *o ) { obj = o; }
    void setT( double tt ) { t = tt; }
    void setN( const vec3f& n ) { N = n; }
    void setMaterial( Material *m ) { delete material; material = m; }
        
    Isect& operator =( const Isect& other )
    {
        if( this != &other )
        {
            obj = other.obj;
            t = other.t;
            N = other.N;
//            material = other.material ? new Material( *(other.material) ) : 0;
			if( other.material )
            {
                if( material )
                    *material = *other.material;
                else
                    material = new Material(*other.material );
            }
            else
            {
                material = 0;
            }
        		hasTexCoords = other.hasTexCoords;
        		texCoords = other.texCoords;
        		tbn = other.tbn;
        }
        return *this;
    }

public:
    const SceneObject 	*obj;
    double t;
    vec3f N;
    Material *material;         // if this intersection has its own material
                                // (as opposed to one in its associated object)
                                // as in the case where the material was interpolated
	bool hasTexCoords{false};
	TexCoords texCoords{0.0, 0.0};
	mat3f tbn;      // TBN matrix to transform from tangent space to world space
	
    const Material &getMaterial() const;
    // Other info here.
};

const double RAY_EPSILON = 0.00001;
const double NORMAL_EPSILON = 0.00001;
const double DISPLACEMENT_EPSILON = 0.00001;

#endif // __RAY_H__
