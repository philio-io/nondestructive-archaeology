#include <stdio.h>
#include <stdlib.h>
#include <float.h>
#include <math.h>
#include <time.h>

typedef struct Point {
	double x, y;
}	Point;

typedef struct Polygon {
	double density;
	int num;
	Point *points;
}	Polygon;

double rateMuons(double minEnergy);
double minEnergy(double distance, double density);
double calcEnergyLoss(double density, double distance);
void rangeOfAngles(double angle, double distance, double length, double* minangle, double* maxangle);
double RandomAngle(double det_length, double det_dist, double angle);
int readPolygons(FILE* f);
void calcTopDet(Point* toppoint, double* topdx, double* topdy, double angle);
void calcBotDet(Point* botpoint, double* botdx, double* botdy, double angle);
void randRay(Point toppoint, double topdx, double topdy, Point botpoint, double botdx, double botdy, double* a, double* b, double* c);
void createLine(double x1, double y1, double x2, double y2, double* a, double* b, double* c);
void findIntersection(double a, double b, double c, Point* p1, Point* p2, Point* p_int);
void travelDist(int i, double* fdist, double* bdist, double a, double b, double c);


int num_polygons;
Polygon* polygons;

double detx, dety;
double detsize;
double detdist;
double detangle;

void usage() {
	fprintf(stderr, "Usage: ./counts detector-x detector-y detector-size detector-distance detector-angle polygon-file\n");
	exit(-1);
}
 
int main(int argc, char* argv[]) {
	unsigned int seed;

	if (argc != 7)
		usage();

	seed = time(0);
	srand(seed);
	FILE* pf = fopen(argv[6], "r+");

	if(pf == NULL) {
		perror("open");
		return -1;
	}	


	if(readPolygons(pf) < 0) {
		return -1;
	}

	fclose(pf);

	double rate;

	Point toppoint, botpoint;
	double topdx, topdy, botdx, botdy;
	double a, b, c;
	double fdist_traveled = 0;
	double bdist_traveled = 0;

	detx = atof(argv[1]);
	dety = atof(argv[2]);
	detsize = atof(argv[3]);
	detdist = atof(argv[4]);
	detangle = (M_PI/180) * atof(argv[5]);

	calcTopDet(&toppoint, &topdx, &topdy, detangle);
	calcBotDet(&botpoint, &botdx, &botdy, detangle);

//	printf("top (%g, %g) dx %g dy %g\n", toppoint.x, toppoint.y, topdx, topdy);
//	printf("bot (%g, %g) dx %g dy %g\n", botpoint.x, botpoint.y, botdx, botdy);
	rate = 0;
	int niter = 100000;
	for(int i = 0; i < niter; i++) {
		double fenergy, benergy;
		double angle;

		randRay(toppoint, topdx, topdy, botpoint, botdx, botdy, &a, &b, &c);
		angle = atan2(b, a);

//		printf("a %g b %g c %g angle %g\n", a, b, c, angle*(180/M_PI));
		

		fenergy = 2.0/fabs(cos(angle));
		benergy = 2.0/fabs(cos(angle));
		for(int j = 0; j < num_polygons; j++) {
			double be, fe;

			travelDist(j, &fdist_traveled, &bdist_traveled, a, b, c);

			fe = minEnergy(fdist_traveled, polygons[j].density);
			be = minEnergy(bdist_traveled, polygons[j].density);
			fenergy += fe;
			benergy += be;
//			printf("fdist %g bdist %g fen %g ben %g\n", fdist_traveled, bdist_traveled, fe, be);
		}

//		printf("final fenergy %g benergy %g\n", fenergy, benergy);
		rate += rateMuons(fenergy);
		rate += rateMuons(benergy);
	}

	printf("angle: %g rate: %g\n", detangle*(180/M_PI), rate/niter);
}
		


void calcTopDet(Point* tp, double* topdx, double* topdy, double angle) {
	tp->x = detx - (detsize/2.0) * (cos(angle)) - (detdist/2.0) * (sin(angle));
	tp->y = dety - (detsize/2.0) * (sin(angle)) + (detdist/2.0) * (cos(angle));

	*topdx = detsize*cos(angle);
	*topdy = detsize*sin(angle);
}

void calcBotDet(Point* bp, double* botdx, double* botdy, double angle) {
	bp->x = detx - (detsize/2.0) * (cos(angle)) + (detdist/2.0) * (sin(angle));
	bp->y = dety - (detsize/2.0) * (sin(angle)) - (detdist/2.0) * (cos(angle));

	*botdx = detsize*cos(angle);
	*botdy = detsize*sin(angle);
}

void randRay(Point toppoint, double topdx, double topdy, Point botpoint, double botdx, double botdy, double* a, double* b, double* c) {
	double p = ((double) rand())/RAND_MAX;
	double q = ((double) rand())/RAND_MAX;
	
	double tx_rand = toppoint.x + p*topdx;
	double ty_rand = toppoint.y + p*topdy;

	double bx_rand = botpoint.x + q*botdx;
	double by_rand = botpoint.y + q*botdy;

	createLine(tx_rand, ty_rand, bx_rand, by_rand, a, b, c);
//	printf("tx_rand: %g\tty_rand: %g\tbx_rand: %g\tby_rand: %g\n", tx_rand, ty_rand, bx_rand, by_rand);
}

void createLine(double x1, double y1, double x2, double y2, double* a, double* b, double* c) {
	if(x1 == x2) {
		*a = -1;
		*b = 0;
		*c = x1;
		return;
	}

	double m = (y1 - y2)/(x1 - x2);

	*a = -m;
	*b = 1;
	*c = m*x1 - y1;
}

void findIntersection(double a, double b, double c, Point* p1, Point* p2, Point* p_int) {
	double d, e, f;

	createLine(p1->x, p1->y, p2->x, p2->y, &d, &e, &f);

	p_int->y = (d*c - f*a)/(e*a - b*d);
	p_int->x = (-b*p_int->y - c)/a;
}

void travelDist(int i, double* fdist, double* bdist, double a, double b, double c) {
	Polygon *p = &polygons[i];
	Point iptprev;
	int in_polygon = 0;

	iptprev.x = NAN;
	iptprev.y = NAN;
	*fdist = 0;
	*bdist = 0;
	for(int j = 0; j < p->num; j++) {
		Point *pt, *ptnext;
		Point ipt;

		pt = &p->points[j];
		if (j+1 == p->num)
			ptnext = &p->points[0];
		else
			ptnext = &p->points[j+1];

		if ((a*pt->x + b*pt->y + c) * (a*ptnext->x + b*ptnext->y + c) > 0) {
			// the ray doesn't intersect the current line
			continue;
		}

		findIntersection(a, b, c, pt, ptnext, &ipt);
//		printf("Intersection-- polygon: %d\tx: %g\ty: %g\n", i, ipt.x, ipt.y);

		if(in_polygon) {
			double dist = sqrt( pow( (ipt.x - iptprev.x), 2) + pow( (ipt.y - iptprev.y), 2) );

			if(ipt.x < detx)
				*bdist += dist;
			else
				*fdist += dist;
		}

		in_polygon = !in_polygon;
		iptprev.x = ipt.x;
		iptprev.y = ipt.y;
	}

//	printf("polygon number: %d\tfdist: %g\tbdist: %g\n", i, *fdist, *bdist);
}

// calculate the percentage of muons that reach the earth compared to total muons (1 MeV to 100 GeV, 1 MeV increment)
double rateMuons(double minEnergy) {
	double total, higheg;
	double test_t, test_h;

	total = 0;
	higheg = 0;
	test_h = 0;
	test_t = 0;

	return pow( (minEnergy/0.01), (1 - M_E) );		// calculated after doing all the integrals, total rate from 10 MeV to infinite energy
}


double minEnergy(double distance, double density) {
//	double energy;
	return calcEnergyLoss(density, distance*100);

//	if (distance > 0) {
//		energy = energyloss + 2.0/fabs(cos(angle));
//	}
//	else {
//		energy = 2.0/fabs(cos(angle));
//	}

//	printf("angle: %g \t distance: %g \t energy: %g \n", angle, distance, energy);

//	return energy;
}


double calcEnergyLoss(double density, double distance) {
	double mip = 2;					// stopping power on minimum ionizing particles is 2 MeV/g/cm^2

	return 0.001 * density * distance * mip;	// density * mip gives the energy loss per cm, so multiply by distance (cm) and 0.001 to get total energy loss in GeV
}

void rangeOfAngles(double angle, double distance, double length, double* minangle, double* maxangle) {
	double a = atan((length/2) / (distance/2));

	*minangle = angle - a;
	*maxangle = angle + a;

	printf("angle: %f, minangle: %f, maxangle: %f\n", angle, *minangle, *maxangle);
}

double RandomAngle(double det_length, double det_dist, double angle) {
	srand(time(0));

	double point1 = ( ((double) rand())/RAND_MAX ) * det_length;
	double point2 = ( ((double) rand())/RAND_MAX ) * det_length;

	double rand_angle = angle + atan( (point1 - point2)/det_dist );

	return rand_angle;

}

int readPolygons(FILE* f) {
	if (fscanf(f, "%d\n", &num_polygons) != 1) {
		fprintf(stderr, "expecting number of polygons\n");

		return -1;
	}

	polygons = malloc(num_polygons*sizeof(Polygon));

	for(int i = 0; i < num_polygons; i++) {
		Polygon *p = &polygons[i];

		if (fscanf(f, "%lg", &p->density) != 1) {
			fprintf(stderr, "expecting density\n");

			return -1;
		}

		if (fscanf(f, "%d", &p->num) != 1) {
			fprintf(stderr, "expecting number of points\n");

			return -1;
		}

		p->points = malloc( p->num * sizeof(Point) );

		for(int j = 0; j < p->num; j++) {
			Point *pt = &p->points[j];

			if (fscanf(f, "%lg %lg", &pt->x, &pt->y) != 2) {
				fprintf(stderr, "expecting x and y values\n");

				return -1;
			}

		}


	}

	return 0;
}
