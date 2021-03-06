#ifndef CAMERA_H
#define CAMERA_H

#include "Ray.h"

class Obj;

class Camera
{
	friend class RayTracer;
	friend static void processCamera( Obj *child, Scene *scene );

public:
    Camera();
    void rayThrough( double x, double y, Ray &r );
    void setEye( const vec3f &eye );
    void setLook( double, double, double, double );
    void setLook( const vec3f &viewDir, const vec3f &upDir );
    void setFOV( double );
    void setAspectRatio( double );
	void setEnableDof(bool value);
	void setAperture(double aperture);
	void setFocalLength(double focal);

    double getAspectRatio() { return aspectRatio; }
	const vec3f& getEye() { return eye; }
	const vec3f& getLook() { return look; }
	double getFov() const { return fov; }

private:
	void update();              // using the above three values calculate look,u,v
	
    static vec3f uniformSampleDisk();
	
    mat3f m;                     // rotation matrix
    double normalizedHeight;    // dimensions of image place at unit dist from eye
    double aspectRatio;
	double fov;
	double time0, time1;			// time to open the shutter
	bool enableMotionBlur;
	
	// Depth of field
	bool enableDof{false};
	double aperture{0.0};
	double focalLength{5.0};
	double lenRadius{0.0};
	
    vec3f eye;
    vec3f look;                  // direction to look
    vec3f u,v;                   // u and v in the
	vec3f uDir, vDir;
	vec3f horizontal, vertical;
	vec3f lowerLeftCorner;
};

#endif
