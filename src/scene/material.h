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
		, index(1.0), isTransmissive(false) {}

    Material( const vec3f& e, const vec3f& a, const vec3f& s, 
              const vec3f& d, const vec3f& r, const vec3f& t, double sh, double in)
        : ke( e ), ka( a ), ks( s ), kd( d ), kr( r ), kt( t ), shininess( sh ), index( in ),
		  isTransmissive(!t.iszero()) {}

	virtual vec3f shade( Scene *scene, const Ray& r, const Isect& i) const;
	virtual vec3f getDiffuseColor(const Isect& isect) const;
	virtual vec3f perturbSurfaceNormal(const Isect& isect) const;
	vec3f fresnelReflective(const vec3f& wo, const vec3f& n) const;  // used for Whitted ray tracing

	// Basic Lambertian model
	virtual vec3f bsdf(const vec3f& wi, const vec3f& wo, const vec3f& n) const;
	virtual vec3f sample(const vec3f& wo, const vec3f& n, double& pdf) const;
	virtual vec3f sampleF(const vec3f& wo, vec3f& wi, const vec3f& n, double& pdf) const;   // sample wi and bsdf simultaneous

	static vec3f localToWorld(const vec3f& v, const vec3f& n);
	static vec3f uniformSampleHemisphere();
	static vec3f reflect(const vec3f& d, const vec3f& n);
	static bool refract(const vec3f& d, const vec3f& n, vec3f& t, double eta);

    vec3f ke;                    // emissive
    vec3f ka;                    // ambient
    vec3f ks;                    // specular
    vec3f kd;                    // diffuse
    vec3f kr;                    // reflective
    vec3f kt;                    // transmissive
	vec3f absorb;               // the light absorbed per unit distance traveled
	bool isTransmissive;
    
    double shininess;
    double index;               // index of refraction

    
                                // material with zero coeffs for everything
                                // as opposed to the "default" material which is
                                // a pleasant blue.
    static const Material zero;

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

class FresnelSpecular : public Material
{
public:
	FresnelSpecular(const vec3f& r, const vec3f& t, double eta);
	vec3f bsdf(const vec3f& wi, const vec3f& wo, const vec3f& n) const override;
	vec3f sample(const vec3f& wo, const vec3f& n, double& pdf) const override;
	vec3f sampleF(const vec3f& wo, vec3f& wi, const vec3f& n, double& pdf) const override;	
};

// Microfacet GGX model
class Microfacet : public Material
{
public:
	Microfacet(const vec3f& albedo, double roughness, double metallic);

	vec3f shade(Scene* scene, const Ray& ray, const Isect& isect) const override; // physically based shading
	vec3f bsdf(const vec3f& wi, const vec3f& wo, const vec3f& n) const override;
	vec3f sample(const vec3f& wo, const vec3f& n, double& pdf) const override;
	vec3f sampleF(const vec3f& wo, vec3f& wi, const vec3f& n, double& pdf) const override;

protected:
	double calNDF(double cosTheta) const; // Trowbridge-Reitz GGX normal distribution function
	double calGGX(double cosTheta) const; // Schlick-GGX
	vec3f calFresnel(double cosTheta, const vec3f& F0) const;

	static vec3f concentricSampleDisk();
	static vec3f cosineSampleHemisphere();
	static double cosineHemispherePdf(double cosTheta);

	// PBR material parameters, based on UE4 BRDF
	vec3f albedo;
	double roughness;
	double metallic;
	double alpha2; // roughness ^ 4
	double k; // (roughness + 1) ^ 2 / 8
};

#endif // __MATERIAL_H__
