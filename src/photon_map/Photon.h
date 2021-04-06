#pragma once

#include "../vecmath/vecmath.h"

class Photon
{
public:
	// TODO: memory optimization: switch to vec3 of float type or more compact type
	vec3f position;
	vec3f direction;
	vec3f power;
};
