#include "myVertex.h"
#include "myVector3D.h"
#include "myHalfedge.h"
#include "myFace.h"

myVertex::myVertex(void)
{
	point = NULL;
	originof = NULL;
	normal = new myVector3D(1.0,1.0,1.0);
}

myVertex::~myVertex(void)
{
	if (normal) delete normal;
}


void myVertex::computeNormal()
{
	myHalfedge* step = originof;
	normal->dX = normal->dY = normal->dZ = 0;
	int counter = 0;

	do {
		myVector3D* fn = step->adjacent_face->normal;
		*normal += *fn;
		counter++;
		step = step->twin->next;
	} while (step != originof);

	*normal = *normal / counter;
}

