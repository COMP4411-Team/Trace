#ifndef CAMERA_H
#define CAMERA_H

#include "Ray.h"

class Camera
{
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
    static vec3f uniformSampleDisk();
	
    mat3f m;                     // rotation matrix
    double normalizedHeight;    // dimensions of image place at unit dist from eye
    double aspectRatio;
	double fov;
	
	// Depth of field
	bool enableDof{false};
	double aperture{1.0};
	double focalLength{5.0};
	double lenRadius{0.5};
	
    void update();              // using the above three values calculate look,u,v
    
    vec3f eye;
    vec3f look;                  // direction to look
    vec3f u,v;                   // u and v in the
	vec3f uDir, vDir;
	vec3f horizontal, vertical;
	vec3f lowerLeftCorner;
};

#endif
