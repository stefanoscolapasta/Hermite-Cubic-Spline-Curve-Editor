#include "ShaderMaker.h"
#include "Lib.h"
#include "Hermite.h"
#include <algorithm>


// viewport size
int width = 1280;
int height = 720;

//Inizializzazione Menu
int submenu_Opzioni_I, menu_id, submenu_Shader_selection;

//Inizializzazioni
int mod_par_der = 0;  //(0) 1 : (non) si intende agire sui parametri T,B,C nel calcolo numerico della derivata nel vertice di controllo selezionato; 
int visualizzaTg = 0; //(0) 1 : (non) si intende visualizzare graficamente la tangente nei vertici di controllo
int visualizzaPC=0; //(0) 1 : (non) si intende visualizzare graficamente il poligono di controllo

//Inizializzazione parametri Tes, Bias, Cont per la modifica delle derivate agli estremi
float Tens = 0.0, Bias = 0.0, Cont = 0.0;  //Questa inizializzazione 'rappresenta le derivate come semplice rapporto incrementale

int shader = 0;  //Metodo=1 --> interpolazione curve di Hermite;  Metodo=2 --> approssimazione di forma curva di Bezier

int M_I = 0;
static unsigned int programId;
mat4 Projection;
GLuint MatProj, MatModel;
unsigned int lsceltavs, lsceltafs, loc_time, loc_res, loc_mouse;
vec2 res, mouse;

float* t;
#define  PI   3.14159265358979323846

typedef struct {
	bool startBrush;
	int brushSize;
	float brushMultiplier;
	Figura* brushCircle;
	vec3 lastMousePos;
}Brush;
Figura circle = {};
Brush brush = { false , 10.0f,1.1f, &circle,vec3(0,0,0)};

typedef struct {
	float angle;
	float scale;
	int curveToModify;
}RotationScaleModifier;
RotationScaleModifier modifier = { 0.0f, 1.0f, -1 };

typedef struct {
	bool start;
	vec3 startingPos;
	vec3 endingPos;
	Figura* line;
}LineDrawing;
Figura Linea = {};
LineDrawing lineDrawing = { true, vec3(0,0,0), vec3(0,0,0), &Linea };

typedef struct {
	bool dragging;
	int dragging_curve;
}Dragging;
Dragging dragging = {false,-1};

typedef struct {
	int selected_curva;
	int selected_CP;
}Selezione;
Selezione selezione = {-1,-1};

vector<std::pair<int, int>> toPerformCut = { };
bool firstPoint = true;
float mousex, mousey;

void update(int a) {
	glutTimerFunc(60, update, 0);
	glutPostRedisplay();
}

bool isPointInFigure(Figura* Curva, vec3 point)
{ //Si basa sul teorema della curva di Jordan, funziona per figure convesse
	int i, j;
	bool c = false;
	int nvert = Curva->vertici.size();
	for (i = 0, j = nvert - 1; i < nvert; j = i++) {
		if (((Curva->vertici[i].y > point.y) != (Curva->vertici[j].y > point.y)) &&
			(point.x < (Curva->vertici[j].x - Curva->vertici[i].x) * (point.y - Curva->vertici[i].y) / (Curva->vertici[j].y - Curva->vertici[i].y) + Curva->vertici[i].x)) {
			c = !c;
		}
	}
	return c;
}

bool intersection(vec3 start1, vec3 end1, vec3 start2, vec3 end2)
{
	float ax = end1.x - start1.x;    
	float ay = end1.y - start1.y;     

	float bx = start2.x - end2.x;
	float by = start2.y - end2.y;

	float dx = start2.x - start1.x;
	float dy = start2.y - start1.y;

	float det = ax * by - ay * bx;

	if (det == 0) return false;

	float r = (dx * by - dy * bx) / det;
	float s = (ax * dy - ay * dx) / det;

	return !(r < 0 || r > 1 || s < 0 || s > 1);
}

vec3 getFigureCentroid(Figura* fig) {
	float sum_x = 0;
	float sum_y = 0;
	for (int cp = 0; cp < fig->vertici.size(); cp++) {
		sum_x += fig->vertici[cp].x;
		sum_y += fig->vertici[cp].y;
	}

	float y_centroid = sum_y / float(fig->vertici.size());
	float x_centroid = sum_x / float(fig->vertici.size());
	return vec3(x_centroid, y_centroid, 0.0f);
}

int findCurveWithClosestVertices() {
	float dist, dist1;
	
	int curve = -1;
	if (Curve.size() > 0) {
		dist = sqrt((mousex - Curve[0].CP[0].x) * (mousex - Curve[0].CP[0].x) + (mousey - Curve[0].CP[0].y) * (mousey - Curve[0].CP[0].y));
		//Calcolo la distanza tra il punto in cui si trova attualmente il mouse e tutti i vertici di controllo sulla finestre
	   //ed individuo l'indice selected_obj del vertice di controllo pi� vicino alla posizione del mouse

		for (int curva = 0; curva < Curve.size(); curva++) {
			for (int contrP = 0; contrP < Curve[curva].vertici.size(); contrP++) {
				dist1 = sqrt((mousex - Curve[curva].vertici[contrP].x) * (mousex - Curve[curva].vertici[contrP].x) + (mousey - Curve[curva].vertici[contrP].y) * (mousey - Curve[curva].vertici[contrP].y));

				if (dist1 <= dist) {
					curve = curva;
					dist = dist1;
				}
			}
		}
	}
	return curve;
}

void setClosestCpAndCurve(float TOLL, float mousex, float mousey) {
	if (Curve.size() > 0) {
		float dist, dist1;

		dist = sqrt((mousex - Curve[0].CP[0].x) * (mousex - Curve[0].CP[0].x) + (mousey - Curve[0].CP[0].y) * (mousey - Curve[0].CP[0].y));

		for (int curva = 0; curva < Curve.size(); curva++) {
			for (int contrP = 0; contrP < Curve[curva].CP.size(); contrP++) {
				dist1 = sqrt((mousex - Curve[curva].CP[contrP].x) * (mousex - Curve[curva].CP[contrP].x) + (mousey - Curve[curva].CP[contrP].y) * (mousey - Curve[curva].CP[contrP].y));

				if (dist1 <= dist) {
					selezione.selected_curva = curva;
					selezione.selected_CP = contrP;
					dist = dist1;
				}
			}
		}

		if (dist > TOLL) {
			selezione.selected_CP = -1;
			selezione.selected_curva = -1;
		}
	}
}

float distanceBetweenTwoPoints(vec3 p1, vec3 p2) {
	return sqrt(pow(p1.x - p2.x, 2) + pow(p1.y - p2.y, 2));
}

void mouseMotion(int x, int y)
{
	mousex = x;
	mousey = height - y;
	if (M_I != 4)
	{
		firstPoint = true;
	}

	if (M_I == 1)
	{
		if (selezione.selected_curva >= 0 && selezione.selected_CP >= 0)
		{
			for (int i = 0; i < Curve[selezione.selected_curva].CP.size(); i++) {
				Curve[selezione.selected_curva].colCP[i] = vec4(1.0, 0.0, 0.0, 1.0);
			}
			Curve[selezione.selected_curva].CP[selezione.selected_CP] = vec3(mousex, mousey, 0.0);
			Curve[selezione.selected_curva].colCP[selezione.selected_CP] = vec4(0.0, 0.0, 1.0, 1.0);

			if (selezione.selected_CP == 0 || selezione.selected_CP == Curve[selezione.selected_curva].CP.size() - 1) {
				int complementar_index = (selezione.selected_CP == 0) ? Curve[selezione.selected_curva].CP.size() - 1 : 0;
				Curve[selezione.selected_curva].CP[complementar_index] = vec3(mousex, mousey, 0.0);
				Curve[selezione.selected_curva].colCP[complementar_index] = vec4(0.0, 0.0, 1.0, 1.0);
			}
		}
	}
	else if (M_I == 6) {
		if (!dragging.dragging) {
			for (int curva = 0; curva < Curve.size(); curva++) {
				if (isPointInFigure(&Curve[curva], vec3(mousex, mousey, 0.0f))) {
					dragging.dragging = true;
					dragging.dragging_curve = curva;
					break;
				}
			}
		}
		else {
			vec3 dir_vec = vec3(mousex, mousey, 0.0f) - getFigureCentroid(&Curve[dragging.dragging_curve]);
			for (int i = 0; i < Curve[dragging.dragging_curve].vertici.size(); i++) {
				Curve[dragging.dragging_curve].vertici[i] += dir_vec;
			}
			for (int i = 0; i < Curve[dragging.dragging_curve].CP.size(); i++) {
				Curve[dragging.dragging_curve].CP[i] += dir_vec;
			}
		}
	}

	else if (M_I == 7) {
		if (lineDrawing.start) {
			lineDrawing.startingPos = vec3(mousex, mousey, 0.0f);
			lineDrawing.start = false;
			lineDrawing.line->sceltaFS = shader;
			lineDrawing.line->sceltaVS = 0;
			lineDrawing.line->vertici.push_back(lineDrawing.startingPos);
			lineDrawing.line->colors.push_back(vec4(0.0f,1.0f,0.0f,1.0f));
			lineDrawing.line->vertici.push_back(lineDrawing.startingPos);
			lineDrawing.line->colors.push_back(vec4(0.0f, 1.0f, 0.0f, 1.0f));
		} 
		if (!lineDrawing.start) {
			lineDrawing.endingPos = vec3(mousex, mousey, 0.0f);
			lineDrawing.line->vertici[1] = lineDrawing.endingPos;
		}
	}

	else if (M_I == 9) {
		if (!brush.startBrush) {
			brush.startBrush = true;
			brush.brushSize = 10.0;
			brush.brushCircle->sceltaFS = shader;
			brush.brushCircle->sceltaVS = 0;
			brush.brushCircle->nTriangles = 40;
			brush.brushCircle->nv = brush.brushCircle->nTriangles + 2;

			float stepA = (2 * PI) / brush.brushCircle->nTriangles;
			for (int i = 0; i <= brush.brushCircle->nTriangles; i++)
			{
				float x = (cos((float)i * stepA) * 40);
				float y = (sin((float)i * stepA) * 40);
				brush.brushCircle->vertici.push_back(vec3(x, y, 0.0f));
				brush.brushCircle->colors.push_back(vec4(0.0f, 1.0f, 0.0f, 1.0f));
			}
		}

		vec3 centroid = getFigureCentroid(brush.brushCircle);
		vec3 dir_vec = vec3(mousex, mousey, 0.0f) - centroid;
		for (int i = 0; i < brush.brushCircle->vertici.size(); i++) {
			brush.brushCircle->vertici[i] += (dir_vec);
			brush.brushCircle->colors[i] = vec4(0.0f, 1.0f, 0.0f, 1.0f);
		}
		
		float calculateRadious = distanceBetweenTwoPoints(vec3(mousex,mousey,0.0f), brush.brushCircle->vertici[0]);
		if (Curve.size() > 0) {
			vector<std::pair<int, int>> CPtoMove;

			for (int curva = 0; curva < Curve.size(); curva++) {
				for (int cp = 0; cp < Curve[curva].CP.size(); cp++) {
					if (distanceBetweenTwoPoints(Curve[curva].CP[cp], vec3(mousex, mousey, 0.0f)) <= calculateRadious) {
						CPtoMove.push_back({ curva,cp });
					}
				}
			}

			for (int cp = 0; cp < CPtoMove.size(); cp++) {
				vec3 dir_vec = vec3(mousex, mousey, 0.0f) - brush.lastMousePos;
				Curve[CPtoMove[cp].first].CP[CPtoMove[cp].second] += dir_vec;
			}
		}
		brush.lastMousePos = vec3(mousex, mousey, 0.0f);
	}

	glutPostRedisplay();
}

void mousePassiveMovement(int x, int y) {
	mousex = (float)x;
	mousey = (float)height - y;

	if (M_I == 9) {
		vec3 centroid = getFigureCentroid(brush.brushCircle);
		vec3 dir_vec = vec3(mousex, mousey, 0.0f) - centroid;
		bool toReColor = false;
		toReColor = brush.brushCircle->colors[0] == vec4(0.0f, 1.0f, 0.0f, 1.0f);
		for (int i = 0; i < brush.brushCircle->vertici.size(); i++) {
			brush.brushCircle->vertici[i] += (dir_vec);
			if (toReColor) brush.brushCircle->colors[i] = vec4(1.0f, 1.0f, 1.0f, 1.0f);
		}
	}
	glutPostRedisplay();
}

void costruisci_formaHermite(vec4 color_top, vec4 color_bot, Figura* Curva, Figura* Poligonale, Figura* Derivata)
{
	Poligonale->CP = Curva->CP;
	Poligonale->colCP = Curva->colCP;

	if (Poligonale->CP.size() > 1)
	{
		t = new float[Curva->CP.size()];
		int i;
		float step = 1.0 / (float)(Curva->CP.size() - 1);

		for (i = 0; i < Curva->CP.size(); i++)
			t[i] = (float)i * step;

		InterpolazioneHermite(t, Curva,Poligonale, Derivata, color_top, color_bot);

		Curva->nv = Curva->vertici.size();
	}

}

void onMouse(int button, int state, int x, int y) 
{
	int i;
	float dist, dist1;
	float TOLL = 10;  //Tolleranza per la selezione del vertice di controllo da modificare
	
	if (state == GLUT_UP) {

		if (dragging.dragging) {
			dragging.dragging = false;
			dragging.dragging_curve = -1;
		}

		if (selezione.selected_curva >= 0 && Curve.size()>0) {
			for (int i = 0; i < Curve[selezione.selected_curva].CP.size(); i++) {
				Curve[selezione.selected_curva].colCP[i] = vec4(1.0f, 0.0f, 0.0f, 1.0f);
			}
		}

		if (!lineDrawing.start) {
			lineDrawing.start = true;
			vector<std::pair<int,std::pair<int,int>>> intersectors;

			for (int curva = 0; curva < Curve.size(); curva++) {
				for (int cp = 0; cp < Curve[curva].CP.size()-1; cp++) {
					if (intersection(lineDrawing.startingPos, lineDrawing.endingPos, Curve[curva].CP[cp], Curve[curva].CP[cp + 1])) {
						intersectors.push_back({ curva ,{cp,cp+1} });
					}
				}
				if (intersectors.size() == 2) {
					break;
				}
				else {
					intersectors.clear();
				}
			}

			if (intersectors.size() == 2 && intersectors[0].first == intersectors[1].first) { //eseguo il taglio solo se interseco esattamente due segmenti

				vec4 col_bottom = vec4{ 0.5451, 0.2706, 0.0745, 1.0000 };
				vec4 col_top = vec4{ 1.0,0.4980, 0.0353,1.0000 };
				int curva = intersectors[0].first;
				Figura Curva2 = {}, Poligonale2 = {}, Derivata2 = {};

				for (int j = intersectors[0].second.second; j <= intersectors[1].second.first; j++) {
					Curva2.CP.push_back(Curve[curva].CP[j]);
					Curva2.colCP.push_back(vec4(1.0, 0.0, 0.0, 1.0));
					Derivata2.CP.push_back(vec3(0.0, 0.0, 0.0));
				}

				if (Curva2.CP.size() >= 3) {
					Curva2.sceltaFS = shader; //TODO: qui sistema se vuoi lo shader
					Curva2.sceltaVS = 0;

					Curva2.CP.push_back(Curva2.CP[0]);
					Curva2.colCP.push_back(vec4(1.0, 0.0, 0.0, 1.0));
					Curva2.fill = Curve[curva].fill;
					Derivata2.CP.push_back(vec3(0.0, 0.0, 0.0));
					//Chiudo il loop

					costruisci_formaHermite(col_top, col_bottom, &Curva2, &Poligonale2, &Derivata2);
					crea_VAO_Vector(&Curva2);
					crea_VAO_CP(&Poligonale2);

					Curve.push_back(Curva2);
					Poligonali.push_back(Poligonale2);
					Derivate.push_back(Derivata2);
				}
				//devo creare due figure separate
				Curve[curva].CP.erase(Curve[curva].CP.begin() + intersectors[0].second.second,
					Curve[curva].CP.begin() + intersectors[1].second.second);

				//Elimino derivate non usate più
				Derivate[curva].CP.erase(Derivate[curva].CP.begin() + intersectors[0].second.second,
					Derivate[curva].CP.begin() + intersectors[1].second.second);

				if (Curve[curva].CP.size() >= 4) {
					costruisci_formaHermite(col_top, col_bottom, &Curve[curva], &Poligonali[curva], &Derivate[curva]);
					crea_VAO_Vector(&Curve[curva]);
					crea_VAO_CP(&Poligonali[curva]);
				}
				else {
					Curve.erase(Curve.begin() + curva);
					Derivate.erase(Derivate.begin() + curva);
				}

				toPerformCut.clear();
			}
				
			lineDrawing.line->vertici.clear();
			lineDrawing.line->colors.clear();
		}

	}

	if (state == GLUT_DOWN)
	{
		switch (button)
		{

		case GLUT_LEFT_BUTTON:
			mousex = (float)x;
			mousey = (float)height - y;
			if (M_I == 1 || mod_par_der == 1)
			{
				setClosestCpAndCurve(TOLL, mousex, mousey);
				Tens = 0.0;
				Cont = 0.0;
				Bias = 0.0;

			}

			else if (M_I == 0)
			{
				if (Curve.size() > 0) {
					if (selezione.selected_curva < 0) selezione.selected_curva = 0;
					if (Curve[selezione.selected_curva].CP.size() > 0) {
						Curve[selezione.selected_curva].CP.pop_back();
						Curve[selezione.selected_curva].colCP.pop_back();
						Derivate[selezione.selected_curva].CP.pop_back();
					}

					Curve[selezione.selected_curva].CP.push_back(vec3(mousex, mousey, 0.0));
					Curve[selezione.selected_curva].colCP.push_back(vec4(1.0, 0.0, 0.0, 1.0));
					Derivate[selezione.selected_curva].CP.push_back(vec3(0, 0, 0.0));

					Curve[selezione.selected_curva].CP.push_back(Curve[selezione.selected_curva].CP[0]);
					Curve[selezione.selected_curva].colCP.push_back(vec4(1.0, 0.0, 0.0, 1.0));
					Derivate[selezione.selected_curva].CP.push_back(vec3(0, 0, 0.0));
				}
			}

			else if (M_I == 2)
			{
				setClosestCpAndCurve(TOLL, mousex, mousey);
				//Elimino l'ultimo vertice di controllo introdotto.
				if (selezione.selected_CP >= 0 && selezione.selected_curva >= 0) {

					vec4 col_bottom = vec4{ 0.5451, 0.2706, 0.0745, 1.0000 };
					vec4 col_top = vec4{ 1.0,0.4980, 0.0353,1.0000 };

					if (selezione.selected_CP == 0 || selezione.selected_CP == Curve[selezione.selected_curva].CP.size() - 1) {
						Curve[selezione.selected_curva].CP.erase(Curve[selezione.selected_curva].CP.begin());
						Curve[selezione.selected_curva].CP.erase(Curve[selezione.selected_curva].CP.begin() + Curve[selezione.selected_curva].CP.size() - 1);
						Curve[selezione.selected_curva].CP.push_back(Curve[selezione.selected_curva].CP[0]);
					}
					else {
						Curve[selezione.selected_curva].CP.erase(Curve[selezione.selected_curva].CP.begin() + selezione.selected_CP);
						Curve[selezione.selected_curva].colCP.erase(Curve[selezione.selected_curva].colCP.begin() + selezione.selected_CP);
					}
					costruisci_formaHermite(col_top, col_bottom, &Curve[selezione.selected_curva], &Poligonali[selezione.selected_curva], &Derivate[selezione.selected_curva]);
					crea_VAO_Vector(&Curve[selezione.selected_curva]);
				}
			}

			else if (M_I == 3)
			{
				setClosestCpAndCurve(TOLL, mousex, mousey);

				if (selezione.selected_CP >= 0 && selezione.selected_curva >= 0) {
					if (toPerformCut.size() == 0) {
						toPerformCut.push_back({ selezione.selected_curva,selezione.selected_CP });
					}
					else if (toPerformCut.size() == 1 && toPerformCut[0].first == selezione.selected_curva) {
						if (abs(toPerformCut[0].second - selezione.selected_CP) > 4) {
							vec4 col_bottom = vec4{ 0.5451, 0.2706, 0.0745, 1.0000 };
							vec4 col_top = vec4{ 1.0,0.4980, 0.0353,1.0000 };
							toPerformCut.push_back({ selezione.selected_curva,selezione.selected_CP });
							costruisci_formaHermite(col_top, col_bottom, &Curve[selezione.selected_curva], &Poligonali[selezione.selected_curva], &Derivate[selezione.selected_curva]);
							crea_VAO_Vector(&Curve[selezione.selected_curva]);
							crea_VAO_CP(&Poligonali[selezione.selected_curva]);
						}
					}

					if (toPerformCut.size() == 2) {
						vec4 col_bottom = vec4{ 0.5451, 0.2706, 0.0745, 1.0000 };
						vec4 col_top = vec4{ 1.0,0.4980, 0.0353,1.0000 };

						Figura Curva2 = {}, Poligonale2 = {}, Derivata2 = {};

						for (int j = std::min(toPerformCut[0].second, toPerformCut[1].second)+1; j < std::max(toPerformCut[0].second, toPerformCut[1].second); j++) {
							Curva2.CP.push_back(Curve[selezione.selected_curva].CP[j]);
							Curva2.colCP.push_back(vec4(1.0, 0.0, 0.0, 1.0));
							Derivata2.CP.push_back(vec3(0.0, 0.0, 0.0));
						}

						Curva2.CP.push_back(Curva2.CP[0]);
						Curva2.colCP.push_back(vec4(1.0, 0.0, 0.0, 1.0));
						Curva2.fill = Curve[selezione.selected_curva].fill;
						Derivata2.CP.push_back(vec3(0.0, 0.0, 0.0));
						//Chiudo il loop

						costruisci_formaHermite(col_top, col_bottom, &Curva2, &Poligonale2, &Derivata2);
						crea_VAO_Vector(&Curva2);
						crea_VAO_CP(&Poligonale2);

						Curve.push_back(Curva2);
						Poligonali.push_back(Poligonale2);
						Derivate.push_back(Derivata2);

						//devo creare due figure separate
						Curve[selezione.selected_curva].CP.erase(Curve[selezione.selected_curva].CP.begin() + std::min(toPerformCut[0].second, toPerformCut[1].second),
							Curve[selezione.selected_curva].CP.begin() + std::max(toPerformCut[0].second, toPerformCut[1].second)+1);

						//Elimino derivate non usate più
						Derivate[selezione.selected_curva].CP.erase(Derivate[selezione.selected_curva].CP.begin() + std::min(toPerformCut[0].second, toPerformCut[1].second),
							Derivate[selezione.selected_curva].CP.begin() + std::max(toPerformCut[0].second, toPerformCut[1].second)+1);

						costruisci_formaHermite(col_top, col_bottom, &Curve[selezione.selected_curva], &Poligonali[selezione.selected_curva], &Derivate[selezione.selected_curva]);
						crea_VAO_Vector(&Curve[selezione.selected_curva]);
						crea_VAO_CP(&Poligonali[selezione.selected_curva]);
						toPerformCut.clear();
					}
				}

			}
			else if (M_I == 4) {

				int indexCurva = findCurveWithClosestVertices();
				if (indexCurva >= 0) {
					printf("INDEX CURVA PER INSERIRE IL CP: %d\n", indexCurva);
					for (int i = 0; i < Curve[indexCurva].CP.size()-1; i++) {
						if (mousex >= std::min(Curve[indexCurva].CP[i].x, Curve[indexCurva].CP[i + 1].x) &&
							mousex <= std::max(Curve[indexCurva].CP[i].x, Curve[indexCurva].CP[i + 1].x) &&
							mousey >= std::min(Curve[indexCurva].CP[i].y, Curve[indexCurva].CP[i + 1].y) &&
							mousey <= std::max(Curve[indexCurva].CP[i].y, Curve[indexCurva].CP[i + 1].y)) {
							vec4 col_bottom = vec4{ 0.5451, 0.2706, 0.0745, 1.0000 };
							vec4 col_top = vec4{ 1.0,0.4980, 0.0353,1.0000 };
							Curve[indexCurva].CP.insert(Curve[indexCurva].CP.begin() + i + 1, vec3(mousex, mousey, 0));
							Curve[indexCurva].colCP.insert(Curve[indexCurva].colCP.begin() + i + 1, vec4(1.0, 0.0, 0.0, 1.0));
							Derivate[indexCurva].CP.push_back(vec3(0, 0, 0));

							costruisci_formaHermite(col_top, col_bottom, &Curve[indexCurva], &Poligonali[indexCurva], &Derivate[indexCurva]);
							crea_VAO_Vector(&Curve[indexCurva]);
							crea_VAO_CP(&Poligonali[indexCurva]);
							indexCurva = -1;
							break;
						}
					}
				}
			}
			else if (M_I == 5) {
			for (int curva = 0; curva < Curve.size(); curva++) {
				if (isPointInFigure(&Curve[curva], vec3(mousex, mousey, 0.0f))) {
						Curve[curva].fill = !Curve[curva].fill;
					}
				}
			}

			else if (M_I == 8) {
			for (int curva = 0; curva < Curve.size(); curva++) {
				if (isPointInFigure(&Curve[curva], vec3(mousex, mousey, 0.0f))) {
					modifier.curveToModify = curva;
					break;
				}
			}
			}

			else if (M_I == 10)
			{
				vec4 col_bottom = vec4{ 0.5451, 0.2706, 0.0745, 1.0000 };
				vec4 col_top = vec4{ 1.0,0.4980, 0.0353,1.0000 };
				selezione.selected_curva = Curve.size();

				Figura newCurve = {}, Poligonale2 = {}, Derivata2 = {};

				newCurve.CP.push_back(vec3(mousex, mousey, 0.0f));
				newCurve.colCP.push_back(vec4(1.0f,0.0f,0.0f,1.0f));
				Derivata2.CP.push_back(vec3(0.0, 0.0, 0.0));

				newCurve.CP.push_back(vec3(mousex, mousey, 0.0f));
				newCurve.colCP.push_back(vec4(1.0f, 0.0f, 0.0f, 1.0f));
				Derivata2.CP.push_back(vec3(0.0, 0.0, 0.0));

				Curve.push_back(newCurve);
				Poligonali.push_back(Poligonale2);
				Derivate.push_back(Derivata2);

				M_I = 0; 
			}

			break;

			case 3:
			case 4:
				 if (M_I == 8 && modifier.curveToModify >= 0) {
					if (button == 3 || button == 4) {
						if (button == 3) {
							modifier.angle += 1.0f;
						}
						else {
							modifier.angle -= 1.0f;
						}
						vec3 centre = getFigureCentroid(&Curve[modifier.curveToModify]);
						for (int i = 0; i < Curve[modifier.curveToModify].CP.size(); i++) {

							float s = sin(radians(modifier.angle));
							float c = cos(radians(modifier.angle));

							// trasla all'origine
							Curve[modifier.curveToModify].CP[i].x -= centre.x;
							Curve[modifier.curveToModify].CP[i].y -= centre.y;

							// ruotalo
							float xnew = Curve[modifier.curveToModify].CP[i].x * c - Curve[modifier.curveToModify].CP[i].y * s;
							float ynew = Curve[modifier.curveToModify].CP[i].x * s + Curve[modifier.curveToModify].CP[i].y * c;

							// traslalo indietro
							Curve[modifier.curveToModify].CP[i].x = xnew + centre.x;
							Curve[modifier.curveToModify].CP[i].y = ynew + centre.y;

						}
						modifier.angle = 0.0f;
					}
				 }
				 else if (M_I == 9) {
					 if (button == 3 || button == 4) {
						 for (int i = 0; i < brush.brushCircle->vertici.size(); i++)
						 {
							 brush.brushCircle->vertici[i] -= vec3(mousex, mousey, 0.0);

							 if (button == 3) {
								 brush.brushCircle->vertici[i] /= brush.brushMultiplier;
							 }
							 else {
								 brush.brushCircle->vertici[i] *= brush.brushMultiplier;
							 }
							 
							 brush.brushCircle->vertici[i] += vec3(mousex, mousey, 0.0);
						 }
						
					 }
				}
		}
	}
	glutPostRedisplay();
}

 
void mykeyboard(unsigned char key, int x, int y)
{
	switch (key)
	{
	case 'a':
		modifier.angle += 0.1;
		break;
	case 'd':
		modifier.angle -= 0.1;
		break;
	}
		glutPostRedisplay();
}

void INIT_VAO(void)
{
	Projection = ortho(0.0f, float(width), 0.0f, float(height));
	MatProj = glGetUniformLocation(programId, "Projection");
	MatModel = glGetUniformLocation(programId, "Model");

	lsceltavs = glGetUniformLocation(programId, "sceltaVS");
	lsceltafs = glGetUniformLocation(programId, "sceltaFS");

	loc_time = glGetUniformLocation(programId, "time");
	loc_res = glGetUniformLocation(programId, "res");
	loc_mouse = glGetUniformLocation(programId, "mouse");

}
	
void INIT_SHADER(void)
{
	GLenum ErrorCheckValue = glGetError();

	char* vertexShader = (char*)"vertexShader_M.glsl";
	char* fragmentShader = (char*)"fragmentShader_M.glsl";

	programId = ShaderMaker::createProgram(vertexShader, fragmentShader);
	glUseProgram(programId);
}

void drawScene(void)
{
	res.x = width;
	res.y = height;
	float time = glutGet(GLUT_ELAPSED_TIME) * 0.0005;
	glUniform1f(loc_time, time);

	glClearStencil(0);

	glClear(GL_COLOR_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

	glClearColor(0.0, 0.0, 0.0, 1.0);
	glClear(GL_COLOR_BUFFER_BIT);

	vec4 col_bottom = vec4{ 0.5451, 0.2706, 0.0745, 1.0000 };
	vec4 col_top = vec4{ 1.0,0.4980, 0.0353,1.0000 };

	for (int i = 0; i < Curve.size(); i++) {

		costruisci_formaHermite(col_top, col_bottom, &Curve[i], &Poligonali[i], &Derivate[i]);
		if (Curve[i].fill) { //Controlla se alla release forse devi rimuoverlo, probabilmente sì
			Curve[i].vertici.insert(Curve[i].vertici.begin(), getFigureCentroid(&Curve[i]));
			Curve[i].colors.insert(Curve[i].colors.begin(), vec4(1.0f, 0.0f, 0.0f, 1.0f));
		}

		if (dragging.dragging && dragging.dragging_curve == i) {
			for (int v = 0; v < Curve[i].colors.size(); v++) {
				Curve[i].colors[v] = vec4(0.0f,1.0f,0.0f,0.5f);
			}
		}

		crea_VAO_Vector(&Curve[i]);
		crea_VAO_CP(&Poligonali[i]);

		glUniformMatrix4fv(MatProj, 1, GL_FALSE, value_ptr(Projection));
		glUniform2f(loc_res, res.x, res.y);
		glUniform2f(loc_mouse, mouse.x, mouse.y);

		glUniform1i(lsceltavs, Curve[i].sceltaVS);
		glUniform1i(lsceltafs, shader);
		glStencilFunc(GL_ALWAYS, 1, -1);

		//la matrice di Modellazione della Curva coincide con la matrice identit�, perch� selezioniamo i punti con il mouse in coordinate del mondo

		glPointSize(8.0f);

		Curve[i].Model = mat4(1.0);

		if (dragging.dragging && dragging.dragging_curve >= 0) {
			vec3 dir_vec = vec3(mousex, mousey, 0.0f) - getFigureCentroid(&Curve[dragging.dragging_curve]);
			Curve[dragging.dragging_curve].Model = translate(Curve[dragging.dragging_curve].Model, dir_vec);
		}

		glUniformMatrix4fv(MatModel, 1, GL_FALSE, value_ptr(Curve[i].Model));

		//Disegno la curva se ho pi� di 2 vertici di controllo
		if (Poligonali[i].CP.size() > 1)
		{
			glBindVertexArray(Curve[i].VAO);
			if (Curve[i].fill) {
				glDrawArrays(GL_TRIANGLE_FAN, 0, Curve[i].vertici.size());
			}
			else {
				glDrawArrays(GL_LINE_STRIP, 0, Curve[i].vertici.size());
			}
			glBindVertexArray(0);
		}

		//Disegno i vertici di controllo
		glBindVertexArray(Poligonali[i].VAO);
		glDrawArrays(GL_POINTS, 0, Poligonali[i].CP.size());
		glBindVertexArray(0);
			
	}

	if (M_I == 9) {
		crea_VAO_Vector(brush.brushCircle);
		glUniformMatrix4fv(MatProj, 1, GL_FALSE, value_ptr(Projection));
		glBindVertexArray(brush.brushCircle->VAO);
		brush.brushCircle->Model = mat4(1.0);
		glUniform2f(loc_res, res.x, res.y);
		glUniform2f(loc_mouse, mouse.x, mouse.y);

		glDrawArrays(GL_LINE_STRIP, 0, brush.brushCircle->vertici.size());

		glUniformMatrix4fv(MatModel, 1, GL_FALSE, value_ptr(brush.brushCircle->Model));
		glBindVertexArray(0);
	}

	if (M_I == 7 && !lineDrawing.start) {
		crea_VAO_Vector(lineDrawing.line);
		glUniformMatrix4fv(MatProj, 1, GL_FALSE, value_ptr(Projection));
		glBindVertexArray(lineDrawing.line->VAO);
		lineDrawing.line->sceltaFS = shader;
		lineDrawing.line->Model = mat4(1.0);
		glUniform2f(loc_res, res.x, res.y);
		glUniform2f(loc_mouse, mouse.x, mouse.y);

		glDrawArrays(GL_LINE_STRIP, 0, lineDrawing.line->vertici.size());

		glUniformMatrix4fv(MatModel, 1, GL_FALSE, value_ptr(lineDrawing.line->Model));
		glBindVertexArray(0);
	}

	//Disegno altri oggetti 
	for (int i = 0; i < Oggetti.size(); i++) {
		crea_VAO_Vector(&Oggetti[i]);
		glUniformMatrix4fv(MatProj, 1, GL_FALSE, value_ptr(Projection));
		glBindVertexArray(Oggetti[i].VAO);
		Oggetti[i].Model = mat4(1.0);

		glUniform2f(loc_res, res.x, res.y);
		glUniform2f(loc_mouse, mouse.x, mouse.y);
			

		if (Oggetti[i].name == "picker") {
			Oggetti[i].Model = translate(Oggetti[i].Model, vec3(mousex, mousey, 0.0));
			Oggetti[i].Model = scale(Oggetti[i].Model, vec3(25, 25, 1.0));
			glDrawArrays(GL_LINE_STRIP, 0, Oggetti[i].vertici.size());
		}
		glUniformMatrix4fv(MatModel, 1, GL_FALSE, value_ptr(Oggetti[i].Model));
		glBindVertexArray(0);
			
	}

	glutSwapBuffers();

}

void menu_shader_selector(int num)
{
	shader = num;
	glutPostRedisplay();
}

void menu_M_I(int num) {
	M_I = num;
	glutPostRedisplay();
}
void menu(int num) {

	glutPostRedisplay();
}

void createMenu(void) {
	submenu_Opzioni_I = glutCreateMenu(menu_M_I);
	glutAddMenuEntry("Add ordered CP", 0);
	glutAddMenuEntry("Move single CP", 1);
	glutAddMenuEntry("Delete single CP", 2);
	glutAddMenuEntry("Split by selecting two CPs", 3);
	glutAddMenuEntry("Insert CP", 4);
	glutAddMenuEntry("Fill/Emply figure", 5);
	glutAddMenuEntry("Move selected figure", 6);
	glutAddMenuEntry("Figure line cutter tool", 7);
	glutAddMenuEntry("Rotate figure", 8);
	glutAddMenuEntry("Brush selection", 9);
	glutAddMenuEntry("Create new figure", 10);

	submenu_Shader_selection = glutCreateMenu(menu_shader_selector);
	glutAddMenuEntry("No shader", 0);
	glutAddMenuEntry("Fractal shader", 1);
	glutAddMenuEntry("Cell interaction", 2);

	// Creazione del menu principale
	menu_id = glutCreateMenu(menu);
	glutAddMenuEntry("Hermite Curve Editor", -1);
	glutAddSubMenu("TOOL selection", submenu_Opzioni_I);
	glutAddSubMenu("Shader selection", submenu_Shader_selection);
	glutAttachMenu(GLUT_RIGHT_BUTTON);
}


int main(int argc, char* argv[])
{
	glutInit(&argc, argv);

	glutInitContextVersion(4, 0);
	glutInitContextProfile(GLUT_CORE_PROFILE);

	glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGBA);

	glutInitWindowSize(width, height);
	glutInitWindowPosition(100, 100);
	glutCreateWindow("Scena OpenGL");
	glutDisplayFunc(drawScene);
	//gestione animazione
	glutMouseFunc(onMouse);
	glutMotionFunc(mouseMotion);
	glutPassiveMotionFunc(mousePassiveMovement);
	glutKeyboardFunc(mykeyboard);
	glutTimerFunc(60, update, 0);
	glewExperimental = GL_TRUE;
	glewInit();
	INIT_SHADER();
	INIT_VAO();
	createMenu();
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	glEnable(GL_STENCIL_TEST);
	glStencilOp(GL_KEEP, GL_KEEP, GL_REPLACE);

	glutMainLoop();
}