#pragma once

#include "../Mesh.h"

class PrimitiveFactory
{
public:

	static Mesh createCube();
	
	static Mesh createBox(float width, float height, float depth);

	static Mesh createTetrahedron();
};