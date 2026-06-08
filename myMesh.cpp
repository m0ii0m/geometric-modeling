#include "myMesh.h"
#include <fstream>
#include <iostream>
#include <sstream>
#include <map>
#include <utility>
#include <GL/glew.h>
#include "myVector3D.h"

using namespace std;

myMesh::myMesh(void)
{
	/**** TODO ****/
}


myMesh::~myMesh(void)
{
	/**** TODO ****/
}

void myMesh::clear()
{
	for (unsigned int i = 0; i < vertices.size(); i++) if (vertices[i]) delete vertices[i];
	for (unsigned int i = 0; i < halfedges.size(); i++) if (halfedges[i]) delete halfedges[i];
	for (unsigned int i = 0; i < faces.size(); i++) if (faces[i]) delete faces[i];

	vector<myVertex *> empty_vertices;    vertices.swap(empty_vertices);
	vector<myHalfedge *> empty_halfedges; halfedges.swap(empty_halfedges);
	vector<myFace *> empty_faces;         faces.swap(empty_faces);
}

void myMesh::checkMesh()
{
	int errors = 0;

	for (unsigned int i = 0; i < halfedges.size(); i++) {
		myHalfedge* he = halfedges[i];
		if (he->twin == NULL) {
			cout << "Error: halfedge " << i << " has no twin\n";
			errors++;
		}
		else if (he->twin->twin != he) {
			cout << "Error: halfedge " << i << " twin->twin != he\n";
			errors++;
		}
	}

	for (unsigned int i = 0; i < halfedges.size(); i++) {
		myHalfedge* he = halfedges[i];
		if (he->next == NULL || he->prev == NULL) {
			cout << "Error: halfedge " << i << " has null next or prev\n";
			errors++;
			continue;
		}
		if (he->next->prev != he) {
			cout << "Error: halfedge " << i << " next->prev != he\n";
			errors++;
		}
		if (he->prev->next != he) {
			cout << "Error: halfedge " << i << " prev->next != he\n";
			errors++;
		}
	}

	for (unsigned int i = 0; i < halfedges.size(); i++) {
		myHalfedge* he = halfedges[i];
		myHalfedge* step = he->next;
		int count = 1;
		while (step != he && count < 1000) {
			step = step->next;
			count++;
		}
		if (step != he) {
			cout << "Error: halfedge " << i << " face loop does not close\n";
			errors++;
		}
	}

	for (unsigned int i = 0; i < halfedges.size(); i++) {
		myHalfedge* he = halfedges[i];
		if (he->twin == NULL || he->next == NULL) continue;
		if (he->twin->source != he->next->source) {
			cout << "Error: halfedge " << i << " twin->source != next->source\n";
			errors++;
		}
	}

	for (unsigned int i = 0; i < faces.size(); i++) {
		myFace* f = faces[i];
		if (f->adjacent_halfedge == NULL) {
			cout << "Error: face " << i << " has null adjacent_halfedge\n";
			errors++;
			continue;
		}
		if (f->adjacent_halfedge->adjacent_face != f) {
			cout << "Error: face " << i << " adjacent_halfedge does not point back to face\n";
			errors++;
		}

		myHalfedge* he = f->adjacent_halfedge;
		do {
			if (he->adjacent_face != f) {
				cout << "Error: face " << i << " has a halfedge pointing to wrong face\n";
				errors++;
				break;
			}
			he = he->next;
		} while (he != f->adjacent_halfedge);
	}

	for (unsigned int i = 0; i < vertices.size(); i++) {
		myVertex* v = vertices[i];
		if (v->originof == NULL) {
			cout << "Warning: vertex " << i << " has null originof (isolated vertex)\n";
			continue;
		}
		if (v->originof->source != v) {
			cout << "Error: vertex " << i << " originof->source != vertex\n";
			errors++;
		}
	}

	if (errors == 0)
		cout << "Mesh check passed! All tests OK\n";
	else
		cout << "Mesh check found " << errors << " error(s)\n";
}


bool myMesh::readFile(std::string filename)
{
	string s, t, u;
	vector<int> faceids;
	myHalfedge **hedges;

	ifstream fin(filename);
	if (!fin.is_open()) {
		cout << "Unable to open file!\n";
		return false;
	}
	name = filename;

	map<pair<int, int>, myHalfedge *> twin_map;
	map<pair<int, int>, myHalfedge *>::iterator it;

	while (getline(fin, s))
	{
		stringstream myline(s);
		myline >> t;
		if (t == "g") {}
		else if (t == "v")
		{
			float x, y, z;
			myline >> x >> y >> z;
			cout << "v " << x << " " << y << " " << z << endl;

			myPoint3D* p = new myPoint3D(x, y, z);
			myVertex* v = new myVertex();
			v->point = p;
			vertices.push_back(v);
		}
		else if (t == "mtllib") {}
		else if (t == "usemtl") {}
		else if (t == "s") {}
		else if (t == "f")
		{
			cout << "f";
			faceids.clear();
			while (myline >> u) { // read indices of vertices from a face into a container - it helps to access them later
				int ind = atoi((u.substr(0, u.find("/"))).c_str());
				cout << " " << ind;
				faceids.push_back(ind - 1);
			}
			cout << endl;

			if (faceids.size() < 3) // ignore degenerate faces
				continue;

			hedges = new myHalfedge * [faceids.size()]; // allocate the array for storing pointers to half-edges
			for (unsigned int i = 0; i < faceids.size(); i++)
				hedges[i] = new myHalfedge(); // pre-allocate new half-edges

			myFace* f = new myFace(); // allocate the new face
			f->adjacent_halfedge = hedges[0]; // connect the face with incident edge

			for (unsigned int i = 0; i < faceids.size(); i++)
			{
				int iplusone = (i + 1) % faceids.size();
				int iminusone = (i - 1 + faceids.size()) % faceids.size();

				// YOUR CODE COMES HERE!

				// connect prevs, and next
				hedges[i]->next = hedges[iplusone];
				hedges[i]->prev = hedges[iminusone];
				hedges[i]->adjacent_face = f;

				// search for the twins using twin_map
				twin_map.insert({ { faceids[i], faceids[iplusone] }, hedges[i] });
				it = twin_map.find({ faceids[iplusone], faceids[i] });
				if (it != twin_map.end()) {
					hedges[i]->twin = it->second;
					it->second->twin = hedges[i];
				}

				// set originof
				myVertex *v = vertices[faceids[i]];
				hedges[i]->source = v;
				if (v->originof == NULL) {
					v->originof = hedges[i];
				}
				// push edges to halfedges in myMesh
				halfedges.push_back(hedges[i]);
			}
			delete[] hedges;
			// push faces to faces in myMesh
			faces.push_back(f);
		}
	}

	checkMesh();
	normalize();

	return true;
}


void myMesh::computeNormals()
{
	for (myFace* f : faces) {
		f->computeNormal();
	}
	for (myVertex* v : vertices) {
		v->computeNormal();
	}
}

void myMesh::normalize()
{
	if (vertices.size() < 1) return;

	int tmpxmin = 0, tmpymin = 0, tmpzmin = 0, tmpxmax = 0, tmpymax = 0, tmpzmax = 0;

	for (unsigned int i = 0; i < vertices.size(); i++) {
		if (vertices[i]->point->X < vertices[tmpxmin]->point->X) tmpxmin = i;
		if (vertices[i]->point->X > vertices[tmpxmax]->point->X) tmpxmax = i;

		if (vertices[i]->point->Y < vertices[tmpymin]->point->Y) tmpymin = i;
		if (vertices[i]->point->Y > vertices[tmpymax]->point->Y) tmpymax = i;

		if (vertices[i]->point->Z < vertices[tmpzmin]->point->Z) tmpzmin = i;
		if (vertices[i]->point->Z > vertices[tmpzmax]->point->Z) tmpzmax = i;
	}

	double xmin = vertices[tmpxmin]->point->X, xmax = vertices[tmpxmax]->point->X,
		ymin = vertices[tmpymin]->point->Y, ymax = vertices[tmpymax]->point->Y,
		zmin = vertices[tmpzmin]->point->Z, zmax = vertices[tmpzmax]->point->Z;

	double scale = (xmax - xmin) > (ymax - ymin) ? (xmax - xmin) : (ymax - ymin);
	scale = scale > (zmax - zmin) ? scale : (zmax - zmin);

	for (unsigned int i = 0; i < vertices.size(); i++) {
		vertices[i]->point->X -= (xmax + xmin) / 2;
		vertices[i]->point->Y -= (ymax + ymin) / 2;
		vertices[i]->point->Z -= (zmax + zmin) / 2;

		vertices[i]->point->X /= scale;
		vertices[i]->point->Y /= scale;
		vertices[i]->point->Z /= scale;
	}
}


void myMesh::splitFaceTRIS(myFace *f, myPoint3D *p)
{
	/**** TODO ****/
}

void myMesh::splitEdge(myHalfedge *e1, myPoint3D *p)
{

	/**** TODO ****/
}

void myMesh::splitFaceQUADS(myFace *f, myPoint3D *p)
{
	/**** TODO ****/
}


void myMesh::subdivisionCatmullClark()
{
	/**** TODO ****/
}

void myMesh::simplify()
{
	/**** TODO ****/
}

void myMesh::simplify(myVertex *)
{
	/**** TODO ****/
}

void myMesh::triangulate()
{
	int initial_face_count = faces.size();
	for (int i = 0; i < initial_face_count; i++) {
		myFace* f = faces[i];
		while (triangulate(f)) {
			myHalfedge* o = f->adjacent_halfedge;
			myHalfedge* he1 = new myHalfedge();
			myHalfedge* he2 = new myHalfedge();
			myFace* new_f = new myFace();

			o->adjacent_face = new_f;
			o->next->adjacent_face = new_f;

			he1->adjacent_face = new_f;
			he1->next = o;
			he1->prev = o->next;
			he1->twin = he2;
			he1->source = o->next->next->source;

			he2->adjacent_face = f;
			he2->next = o->next->next;
			he2->prev = o->prev;
			he2->twin = he1;
			he2->source = o->source;

			he1->next->prev = he1;
			he1->prev->next = he1;
			he2->next->prev = he2;
			he2->prev->next = he2;

			new_f->adjacent_halfedge = he1;
			f->adjacent_halfedge = he2;

			faces.push_back(new_f);
			halfedges.push_back(he1);
			halfedges.push_back(he2);
		}
	}
}

//return false if already triangle, true othewise.
bool myMesh::triangulate(myFace *f)
{
	myHalfedge* start = f->adjacent_halfedge;

	if (start == start->next->next->next)
		return false;

	myVector3D face_normal(0, 0, 0);
	myHalfedge* he = start;
	do {
		myPoint3D* cur = he->source->point;
		myPoint3D* nxt = he->next->source->point;
		face_normal.dX += (cur->Y - nxt->Y) * (cur->Z + nxt->Z);
		face_normal.dY += (cur->Z - nxt->Z) * (cur->X + nxt->X);
		face_normal.dZ += (cur->X - nxt->X) * (cur->Y + nxt->Y);
		he = he->next;
	} while (he != start);

	he = start;
	do {
		myPoint3D* a = he->source->point;
		myPoint3D* b = he->next->source->point;
		myPoint3D* c = he->next->next->source->point;

		myVector3D ab(b->X - a->X, b->Y - a->Y, b->Z - a->Z);
		myVector3D bc(c->X - b->X, c->Y - b->Y, c->Z - b->Z);
		myVector3D cross;
		cross.crossproduct(ab, bc);
		double dot = cross.dX * face_normal.dX + cross.dY * face_normal.dY + cross.dZ * face_normal.dZ;

		if (dot > 0) {
			
			bool ear = true;
			myHalfedge* test = he->next->next->next;
			while (test != he) {
				myPoint3D* p = test->source->point;

				myVector3D ap(p->X - a->X, p->Y - a->Y, p->Z - a->Z);
				myVector3D bp(p->X - b->X, p->Y - b->Y, p->Z - b->Z);
				myVector3D cp(p->X - c->X, p->Y - c->Y, p->Z - c->Z);

				myVector3D cross1; cross1.crossproduct(ab, ap);
				myVector3D edge_bc(c->X - b->X, c->Y - b->Y, c->Z - b->Z);
				myVector3D cross2; cross2.crossproduct(edge_bc, bp);
				myVector3D ca(a->X - c->X, a->Y - c->Y, a->Z - c->Z);
				myVector3D cross3; cross3.crossproduct(ca, cp);

				double d1 = cross1.dX * face_normal.dX + cross1.dY * face_normal.dY + cross1.dZ * face_normal.dZ;
				double d2 = cross2.dX * face_normal.dX + cross2.dY * face_normal.dY + cross2.dZ * face_normal.dZ;
				double d3 = cross3.dX * face_normal.dX + cross3.dY * face_normal.dY + cross3.dZ * face_normal.dZ;

				if (d1 >= 0 && d2 >= 0 && d3 >= 0) {
					ear = false;
					break;
				}
				test = test->next;
			}
			if (ear) {
				f->adjacent_halfedge = he;
				return true;
			}
		}
		he = he->next;
	} while (he != start);

	f->adjacent_halfedge = start;
	return true;
}

void myMesh::generateSurfaceOfRevolution(vector<myPoint3D> &profile, int slices)
{
	clear();

	int n = profile.size();
	if (n < 2 || slices < 3) return;

	double PI2 = 2.0 * 3.14159265358979323846;

	for (int j = 0; j < slices; j++) {
		double angle = PI2 * j / slices;
		double cosA = cos(angle);
		double sinA = sin(angle);
		for (int i = 0; i < n; i++) {
			myVertex* v = new myVertex();
			v->point = new myPoint3D(
				profile[i].X * cosA,
				profile[i].Z,
				profile[i].X * sinA
			);
			vertices.push_back(v);
		}
	}

	map<pair<int, int>, myHalfedge*> twin_map;
	map<pair<int, int>, myHalfedge*>::iterator it;

	for (int j = 0; j < slices; j++) {
		int j_next = (j + 1) % slices;
		for (int i = 0; i < n - 1; i++) {
			int v0 = j * n + i;
			int v1 = j * n + (i + 1);
			int v2 = j_next * n + (i + 1);
			int v3 = j_next * n + i;

			int faceids[4] = { v0, v1, v2, v3 };

			myHalfedge* hedges[4];
			for (int k = 0; k < 4; k++)
				hedges[k] = new myHalfedge();

			myFace* f = new myFace();
			f->adjacent_halfedge = hedges[0];

			for (int k = 0; k < 4; k++) {
				int kn = (k + 1) % 4;
				int kp = (k - 1 + 4) % 4;

				hedges[k]->next = hedges[kn];
				hedges[k]->prev = hedges[kp];
				hedges[k]->adjacent_face = f;
				hedges[k]->source = vertices[faceids[k]];

				if (vertices[faceids[k]]->originof == NULL)
					vertices[faceids[k]]->originof = hedges[k];

				twin_map.insert({ { faceids[k], faceids[kn] }, hedges[k] });
				it = twin_map.find({ faceids[kn], faceids[k] });
				if (it != twin_map.end()) {
					hedges[k]->twin = it->second;
					it->second->twin = hedges[k];
				}

				halfedges.push_back(hedges[k]);
			}

			faces.push_back(f);
		}
	}

	normalize();
	checkMesh();
}