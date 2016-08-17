#pragma once
#ifndef KMEANS_H
#define KMEANS_H

#include <cstdio>
#include <cstdlib>
#include <cmath>

//typedef struct { double x, y; int group; } point_t, *point;

double randf(double m)
{
	return m * rand() / (RAND_MAX - 1.);
}

//double dist2(point a, point b){ return 0; }

template <class point, class distFunction>
inline int nearest(point pt, point cent, int n_cluster, double *d2, distFunction& dist2)
{
	int i, min_i;
	point c;
	double d, min_d;

#	define for_n for (c = cent, i = 0; i < n_cluster; i++, c++)
	min_d = HUGE_VAL;
	min_i = pt->group;
	for_n{
		if (min_d >(d = dist2(c, pt))) {
			min_d = d; min_i = i;
		}
	}
	if (d2) *d2 = min_d;
	return min_i;
}

template <class point, class distFunction>
void kpp(point pts, int len, point cent, int n_cent, distFunction& dist2)
{
#	define for_len for (j = 0, p = pts; j < len; j++, p++)
	int i, j;
	int n_cluster;
	double sum, *d = (double*)malloc(sizeof(double)* len);

	point p, c;
	cent[0] = pts[rand() % len];
	for (n_cluster = 1; n_cluster < n_cent; n_cluster++) {
		sum = 0;
		for_len{
			nearest(p, cent, n_cluster, d + j, dist2);
			sum += d[j];
		}
		sum = randf(sum);
		for_len{
			if ((sum -= d[j]) > 0) continue;
			cent[n_cluster] = pts[j];
			break;
		}
	}
	for_len p->group = nearest(p, cent, n_cluster, 0, dist2);
	free(d);
}

template <class point_t, class distFunction, class findCentroidFunction>
point_t* lloyd(point_t* pts, int len, int n_cluster, distFunction dist2, findCentroidFunction find_cent)
{
	typedef point_t* point;
	int i, j, min_i;
	int changed;

	point cent = new point_t[n_cluster], p, c;

	/* assign init grouping randomly */
	//for_len p->group = j % n_cluster;

	/* or call k++ init */
	kpp(pts, len, cent, n_cluster, dist2);

	do {
		find_cent(pts, len, cent, n_cluster);

		changed = 0;
		/* find closest centroid of each point */
		for_len{
			min_i = nearest(p, cent, n_cluster, 0, dist2);
			if (min_i != p->group) {
				changed++;
				p->group = min_i;
			}
		}
	} while (changed > len/100); /* stop when 99% of points are good */
	for_n{ c->group = i; }

	return cent;
}

#	undef for_n
#	undef for_len


#endif //KMEANS_H