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
	virtual vec3f getDiffuseColor(const Ray& ray, const Isect& isect) const;
	virtual vec3f perturbSurfaceNormal(const Isect& isect) const;
	virtual vec3f randomReflect(const vec3f& d, const vec3f& n) const;
	double fresnel(const vec3f& wo, const vec3f& n) const;
	vec3f fresnelReflective(const vec3f& wo, const vec3f& n) const;

	virtual vec3f bxdf(const vec3f& wi, const vec3f& wo, const vec3f& n) const;             // given wi and wo, calculate BxDF
	virtual vec3f sample(const vec3f& wo, const vec3f& n, double& pdf) const;               // sample a new direction for ray
	virtual vec3f sampleF(const vec3f& wo, vec3f& wi, const vec3f& n, double& pdf) const;   // sample wi and BxDF simultaneous

	static vec3f localToWorld(const vec3f& v, const vec3f& n);
	static vec3f uniformSampleHemisphere();
	static vec3f uniformSampleSphere();
	static vec3f reflect(const vec3f& d, const vec3f& n);
	static bool refract(const vec3f& d, const vec3f& n, vec3f& t, double eta);

    vec3f ke;                    // emissive
    vec3f ka;                    // ambient
    vec3f ks;                    // specular
    vec3f kd;                    // diffuse
    vec3f kr;                    // reflective
    vec3f kt;                    // transmissive
	vec3f absorb;                // the light absorbed per unit distance traveled

	bool isTransmissive{false}; // for path tracing, no relation to kt
    double shininess;
	double glossiness{0.0};
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
	FresnelSpecular(const vec3f& albedo, double roughness, double metallic, double translucency):
		albedo(albedo), roughness(roughness), metallic(metallic), translucency(translucency),
		roughness2(roughness * roughness) { isTransmissive = translucency > 0.0; index = 1.5; ks = vec3f(1.0); }
	vec3f bxdf(const vec3f& wi, const vec3f& wo, const vec3f& n) const override;
	vec3f sample(const vec3f& wo, const vec3f& n, double& pdf) const override;
	vec3f sampleF(const vec3f& wo, vec3f& wi, const vec3f& n, double& pdf) const override;

protected:
	vec3f albedo;
	double roughness;
	double roughness2;
	double metallic;
	double translucency;
};


// Microfacet specular
class Microfacet : public Material
{
public:
	Microfacet(const vec3f& albedo, double roughness, double metallic);

	vec3f shade(Scene* scene, const Ray& ray, const Isect& isect) const override;
	vec3f bxdf(const vec3f& wi, const vec3f& wo, const vec3f& n) const override;
	vec3f sample(const vec3f& wo, const vec3f& n, double& pdf) const override;
	vec3f sampleF(const vec3f& wo, vec3f& wi, const vec3f& n, double& pdf) const override;

protected:
	double calD(double cosTheta) const; 
	double calG(double cosTheta) const;
	vec3f calF(double cosTheta, const vec3f& F0) const;

	static vec3f concentricSampleDisk();
	static vec3f cosineSampleHemisphere();
	static double cosineHemispherePdf(double cosTheta);

	// Material parameters
	vec3f albedo;
	double roughness;
	double metallic;
	double alpha2; // roughness ^ 4
	double k; // (roughness + 1) ^ 2 / 8
};

#endif // __MATERIAL_H__
