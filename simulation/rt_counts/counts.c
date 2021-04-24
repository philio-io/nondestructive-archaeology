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

void rateMuons(double* rate, double minEnergy);
double minEnergy(double distance, double density, double angle);
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
	fprintf(stderr, "Usage: ./counts detector-x detector-y detector-size detector-distance detector-angle\n");
	exit(-1);
}
 
int main(int argc, char* argv[]) {
	if (argc != 6)
		usage();

	FILE* pf = fopen("polygons.txt", "r+");

	if(pf == NULL) {
		perror("open");
		return -1;
	}	


	if(readPolygons(pf) < 0) {
		return -1;
	}

	fclose(pf);

	double tuff = 1.57;
	double frate;
	double brate;
	double temp_rate;

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


	for(int i = 0; i < 100000; i++) {
		randRay(toppoint, topdx, topdy, botpoint, botdx, botdy, &a, &b, &c);
		for(int j = 0; j < num_polygons; j++) {
			temp_rate = 0;
			travelDist(j, &fdist_traveled, &bdist_traveled, a, b, c);

			double fenergy = minEnergy(fdist_traveled, tuff, atan2(-a, b) );
			rateMuons(&temp_rate, fenergy);
			frate += temp_rate;

			temp_rate = 0;
			double benergy = minEnergy(bdist_traveled, tuff, atan2(-a, b) );
			rateMuons(&temp_rate, benergy);
			brate += temp_rate;
		}			
	}

	printf("angle: %g\t frate: %g\t brate: %g\n", detangle, frate, brate);

}
		


void calcTopDet(Point* tp, double* topdx, double* topdy, double angle) {
	tp->x = detx - (detsize/2.0) * (cos(angle)) - (detdist/2.0) * (sin(angle));
	tp->y = dety - (detsize/2.0) * (sin(angle)) + (detdist/2.0) * (cos(angle));

	*topdx = detsize*cos(angle);
	*topdy = detdist*sin(angle);
}

void calcBotDet(Point* bp, double* botdx, double* botdy, double angle) {
	bp->x = detx - (detsize/2.0) * (cos(angle)) + (detdist/2.0) * (sin(angle));
	bp->y = dety - (detsize/2.0) * (sin(angle)) - (detdist/2.0) * (cos(angle));

	*botdx = detsize*cos(angle);
	*botdy = detdist*sin(angle);
}

void randRay(Point toppoint, double topdx, double topdy, Point botpoint, double botdx, double botdy, double* a, double* b, double* c) {
	srand(time(0));
	
	double p = (double) rand()/RAND_MAX;
	double q = (double) rand()/RAND_MAX;

	double tx_rand = toppoint.x + p*topdx;
	double ty_rand = toppoint.y + p*topdy;

	double bx_rand = botpoint.x + q*botdx;
	double by_rand = botpoint.y + q*botdy;

	createLine(tx_rand, ty_rand, bx_rand, by_rand, a, b, c);
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
	*c = x1 - y1;
}

void findIntersection(double a, double b, double c, Point* p1, Point* p2, Point* p_int) {
	double d, e, f;

	createLine(p1->x, p1->y, p2->x, p2->y, &d, &e, &f);

	p_int-> y = (d*c - f*a)/(e*a - b*d);
	p_int->x = (-b*p_int->y - c)/a;
}

void travelDist(int i, double* fdist, double* bdist, double a, double b, double c) {
	Polygon *p = &polygons[i];
	Point temp;
	int in_polygon = 0;

	for(int j = 0; j < p->num; j++) {
		Point pt;

		if(in_polygon) {
			findIntersection(a, b, c, &temp, &p->points[j], &pt);

			if(pt.x < detx)
				*bdist += sqrt( pow( (pt.x - temp.x), 2) + pow( (pt.y - temp.y), 2) );
			else
				*fdist += sqrt( pow( (pt.x - temp.x), 2) + pow( (pt.y - temp.y), 2) );
		}

		in_polygon = !in_polygon;
		temp.x = pt.x;
		temp.y = pt.y;
	}
}

// calculate the percentage of muons that reach the earth compared to total muons (1 MeV to 100 GeV, 1 MeV increment)
void rateMuons(double* rate, double minEnergy) {
	double total, higheg;
	double test_t, test_h;

	total = 0;
	higheg = 0;
	test_h = 0;
	test_t = 0;

	*rate = pow( (minEnergy/0.01), (1 - M_E) );		// calculated after doing all the integrals, total rate from 10 MeV to infinite energy
}


double minEnergy(double distance, double density, double angle) {
	double energy;
	double energyloss = calcEnergyLoss(density, distance*100);

	if (distance > 0) {
		energy = energyloss + 2.0/fabs(cos(angle));
	}
	else {
		energy = 2.0/fabs(cos(angle));
	}

//	printf("angle: %g \t distance: %g \t energy: %g \n", angle, distance, energy);

	return energy;
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

