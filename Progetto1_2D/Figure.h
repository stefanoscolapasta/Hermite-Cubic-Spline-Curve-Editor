#include <iostream>
#include <vector>
#include "ShaderMaker.h"
#include "Lib.h"

typedef struct {
	GLuint VAO;
	GLuint VBO_G;
	GLuint VBO_C;
	GLuint EBO_indici;
	int nTriangles;
	bool fill;
	// Vertici
	vector<vec3> vertici;
	vector<vec3> CP;
	vector<vec4> colors;
	vector<vec4> colCP;
	vector<int> indici;
	// Numero vertici
	int nv;
	//Matrice di Modellazione: Traslazione*Rotazione*Scala
	mat4 Model;
	int sceltaVS;
	int sceltaFS;
	string name;
} Figura;

Figura cur, pol, der, tang;
vector<Figura> Curve = {  }, Poligonali = {  }, Derivate = {  };
vector<Figura> Oggetti;