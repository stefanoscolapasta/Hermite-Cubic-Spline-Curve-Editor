#include "VAO_generators.h"

#define PHI0(t)  (2.0*t*t*t-3.0*t*t+1)
#define PHI1(t)  (t*t*t-2.0*t*t+t)
#define PSI0(t)  (-2.0*t*t*t+3.0*t*t)
#define PSI1(t)  (t*t*t-t*t)

int pval = 140;


float dx(int i, float* t, float Tens, float Bias, float Cont, Figura* Fig)
{
	if (i == 0)
		return  0.5 * (1 - Tens) * (1 - Bias) * (1 - Cont) * (Fig->CP[i + 1].x - Fig->CP[i].x) / (t[i + 1] - t[i]);
	if (i == Fig->CP.size() - 1)
		return  0.5 * (1 - Tens) * (1 - Bias) * (1 - Cont) * (Fig->CP[i].x - Fig->CP[i - 1].x) / (t[i] - t[i - 1]);

	if (i % 2 == 0)
		return  0.5 * (1 - Tens) * (1 + Bias) * (1 + Cont) * (Fig->CP.at(i).x - Fig->CP.at(i - 1).x) / (t[i] - t[i - 1]) + 0.5 * (1 - Tens) * (1 - Bias) * (1 - Cont) * (Fig->CP.at(i + 1).x - Fig->CP.at(i).x) / (t[i + 1] - t[i]);
	else
		return  0.5 * (1 - Tens) * (1 + Bias) * (1 - Cont) * (Fig->CP.at(i).x - Fig->CP.at(i - 1).x) / (t[i] - t[i - 1]) + 0.5 * (1 - Tens) * (1 - Bias) * (1 + Cont) * (Fig->CP.at(i + 1).x - Fig->CP.at(i).x) / (t[i + 1] - t[i]);
}
float dy(int i, float* t, float Tens, float Bias, float Cont, Figura* Fig)
{
	if (i == 0)
		return 0.5 * (1.0 - Tens) * (1.0 - Bias) * (1 - Cont) * (Fig->CP.at(i + 1).y - Fig->CP.at(i).y) / (t[i + 1] - t[i]);
	if (i == Fig->CP.size() - 1)
		return  0.5 * (1 - Tens) * (1 - Bias) * (1 - Cont) * (Fig->CP.at(i).y - Fig->CP.at(i - 1).y) / (t[i] - t[i - 1]);

	if (i % 2 == 0)
		return  0.5 * (1 - Tens) * (1 + Bias) * (1 + Cont) * (Fig->CP.at(i).y - Fig->CP.at(i - 1).y) / (t[i] - t[i - 1]) + 0.5 * (1 - Tens) * (1 - Bias) * (1 - Cont) * (Fig->CP.at(i + 1).y - Fig->CP.at(i).y) / (t[i + 1] - t[i]);
	else
		return  0.5 * (1 - Tens) * (1 + Bias) * (1 - Cont) * (Fig->CP.at(i).y - Fig->CP.at(i - 1).y) / (t[i] - t[i - 1]) + 0.5 * (1 - Tens) * (1 - Bias) * (1 + Cont) * (Fig->CP.at(i + 1).y - Fig->CP.at(i).y) / (t[i + 1] - t[i]);
}

float DX(int i, float* t, Figura* Poligonale, Figura* Derivata)
{
	//Nei vertici di controllo per i quali non sono stati modificati i parametri Tens, Bias, Cont il valore della derivata della componente x della curva � quello originale, altrimenti � quello che � stato modificato nella funzione 
	//keyboardfunc  in seguito alla modifica dei valori Tens, Bias e Cont.
	
	if (Derivata->CP.at(i).x == 0)
		return dx(i, t, 0.0, 0.0, 0.0, Poligonale);

	if (Derivata->CP.at(i).x != 0)
		return Derivata->CP.at(i).x;

}

float DY(int i, float* t, Figura* Poligonale, Figura* Derivata)
{
	// Nei vertici di controllo per i quali non sono stati modificati i parametri Tens, Bias, Cont il valore della derivata della componente y della curva � quello originale, altrimenti � quello che � stato modificato nella funzione
		//keyboardfunc  in seguito alla modifica dei valori Tens, Bias e Cont.

	if (Derivata->CP.at(i).y == 0)
		return dy(i, t, 0.0, 0.0, 0.0, Poligonale);

	if (Derivata->CP.at(i).y != 0)
		return Derivata->CP.at(i).y;

}

void InterpolazioneHermite(float* t, Figura* Curva, Figura* Poligonale, Figura* Derivata, vec4 color_top, vec4 color_bot)
{
	float p_t = 0, p_b = 0, p_c = 0, x, y;
	float passotg = 1.0 / (float)(pval - 1);

	float tg = 0, tgmapp, ampiezza; //t graficazione
	int i = 0;
	int is = 0; //indice dell'estremo sinistro dell'intervallo [t(i),t(i+1)] a cui il punto tg
				//appartiene

	Curva->vertici.clear();
	Curva->colors.clear();

	for (tg = 0; tg <= 1; tg += passotg)
	{
		if (tg > t[is + 1]) is++;

		ampiezza = (t[is + 1] - t[is]);
		tgmapp = (tg - t[is]) / ampiezza;

		x = Curva->CP[is].x * PHI0(tgmapp) + DX(is, t, Poligonale, Derivata) * PHI1(tgmapp) * ampiezza + Curva->CP[is + 1].x * PSI0(tgmapp) + DX(is + 1, t,Poligonale, Derivata) * PSI1(tgmapp) * ampiezza;
		y = Curva->CP[is].y * PHI0(tgmapp) + DY(is, t, Poligonale, Derivata) * PHI1(tgmapp) * ampiezza + Curva->CP[is + 1].y * PSI0(tgmapp) + DY(is + 1, t,Poligonale, Derivata) * PSI1(tgmapp) * ampiezza;
		Curva->vertici.push_back(vec3(x, y, 0.0));
		Curva->colors.push_back(color_top);
	}

}
