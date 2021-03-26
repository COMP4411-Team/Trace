//
// material.h
//
// The Material class: a description of the phsyical properties of a surface
// that are used to determine how that surface interacts with light.

#ifndef __MATERIAL_H__
#define __MATERIAL_H__

#include "../vecmath/vecmath.h"

class Scene;
class Ray;
class Isect;

class Material
{
public:
    Material()
        : ke( vec3f( 0.0, 0.0, 0.0 ) )
        , ka( vec3f( 0.0, 0.0, 0.0 ) )
        , ks( vec3f( 0.0, 0.0, 0.0 ) )
        , kd( vec3f( 0.0, 0.0, 0.0 ) )
        , kr( vec3f( 0.0, 0.0, 0.0 ) )
        , kt( vec3f( 0.0, 0.0, 0.0 ) )
        , shininess( 0.0 ) 
		, index(1.0) {}

    Material( const vec3f& e, const vec3f& a, const vec3f& s, 
              const vec3f& d, const vec3f& r, const vec3f& t, double sh, double in)
        : ke( e ), ka( a ), ks( s ), kd( d ), kr( r ), kt( t ), shininess( sh ), index( in ) {}

	virtual vec3f shade( Scene *scene, const Ray& r, const Isect& i ) const;
	virtual vec3f getDiffuseColor(const Isect& isect) const;
	virtual vec3f perturbSurfaceNormal(const Isect& isect) const;

	void initBRDF();
	vec3f pbs(Scene *scene, const Ray& r, const Isect& i) const;    // physically based shading
	double calNDF(double cosTheta) const;   // Trowbridge-Reitz GGX normal distribution function
	double calGGX(double cosTheta) const;   // Schlick-GGX
	vec3f calFresnel(double cosTheta, const vec3f& F0) const;
	vec3f sampleBRDF(const vec3f& l, const vec3f& v, const vec3f& n, const vec3f& albedo) const;

    vec3f ke;                    // emissive
    vec3f ka;                    // ambient
    vec3f ks;                    // specular
    vec3f kd;                    // diffuse
    vec3f kr;                    // reflective
    vec3f kt;                    // transmissive
    
    double shininess;
    double index;               // index of refraction

    
                                // material with zero coeffs for everything
                                // as opposed to the "default" material which is
                                // a pleasant blue.
    static const Material zero;

	// PBR material parameters, based on UE4 BRDF
	vec3f albedo;
	double roughness;
	double metallic;
	double alpha2;              // roughness ^ 4
	double k;                   // (roughness + 1) ^ 2 / 8
	bool pbrReady{false};       // true if all the parameters needed are given

    Material &
    operator+=( const Material &m )
    {
        ke += m.ke;
        ka += m.ka;
        ks += m.ks;
        kd += m.kd;
        kr += m.kr;
        kt += m.kt;
        index += m.index;
        shininess += m.shininess;
        return *this;
    }

    friend Material operator*( double d, Material m );
};

inline Material
operator*( double d, Material m )
{
    m.ke *= d;
    m.ka *= d;
    m.ks *= d;
    m.kd *= d;
    m.kr *= d;
    m.kt *= d;
    m.index *= d;
    m.shininess *= d;
    return m;
}
// extern Material THE_DEFAULT_MATERIAL;

#endif // __MATERIAL_H__
