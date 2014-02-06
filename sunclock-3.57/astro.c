/*
 * Sun clock - astronomical routines.
 */

#include "sunclock.h"


/*  JDATE  --  Convert internal GMT date and time to Julian day
	       and fraction.  */

long
jdate(t)
struct tm *t;
{
	long c, m, y;

	y = t->tm_year + 1900;
	m = t->tm_mon + 1;
	if (m > 2)
	   m = m - 3;
	else {
	   m = m + 9;
	   y--;
	}
	c = y / 100L;		   /* Compute century */
	y -= 100L * c;
	return t->tm_mday + (c * 146097L) / 4 + (y * 1461L) / 4 +
	    (m * 153L + 2) / 5 + 1721119L;
}

/* JTIME --    Convert internal GMT  date  and	time  to  astronomical
	       Julian  time  (i.e.   Julian  date  plus  day fraction,
	       expressed as a double).	*/

double
jtime(t)
struct tm *t;
{
	return (jdate(t) - 0.5) + 
	   (((long) t->tm_sec) +
	     60L * (t->tm_min + 60L * t->tm_hour)) / 86400.0;
}

/*  GMST  --  Calculate Greenwich Mean Siderial Time for a given
	      instant expressed as a Julian date and fraction.	*/

double
gmst(jd)
double jd;
{
	double t, theta0;


	/* Time, in Julian centuries of 36525 ephemeris days,
	   measured from the epoch 1900 January 0.5 ET. */

	t = ((floor(jd + 0.5) - 0.5) - 2415020.0) / 36525.0;

	theta0 = 6.6460656 + 2400.051262 * t + 0.00002581 * t * t;

	t = (jd + 0.5) - (floor(jd + 0.5));

	theta0 += (t * 24.0) * 1.002737908;

	theta0 = (theta0 - 24.0 * (floor(theta0 / 24.0)));

	return theta0;
}

/*  KEPLER  --	Solve the equation of Kepler.  */

double
kepler(m, ecc)
double m, ecc;
{
	double e, delta;
#define EPSILON 1E-6

	e = m = torad(m);
	do {
	   delta = e - ecc * sin(e) - m;
	   e -= delta / (1 - ecc * cos(e));
	} while (abs(delta) > EPSILON);
	return e;
}

/*  SUNPOS  --	Calculate position of the Sun.	JD is the Julian  date
		of  the  instant for which the position is desired and
		APPARENT should be nonzero if  the  apparent  position
		(corrected  for  nutation  and aberration) is desired.
                The Sun's co-ordinates are returned  in  RA  and  DEC,
		both  specified  in degrees (divide RA by 15 to obtain
		hours).  The radius vector to the Sun in  astronomical
                units  is returned in RV and the Sun's longitude (true
		or apparent, as desired) is  returned  as  degrees  in
		SLONG.	*/

/* Obliquity of the ecliptic (radians) */
double epsrad;

void
sunpos(jd, apparent, ra, dec, rv, slong)
double jd;
int apparent;
double *ra, *dec, *rv, *slong;
{
	double t, t2, t3, l, m, e, ea, v, theta, thetarad, omega, eps;

	/* Time, in Julian centuries of 36525 ephemeris days,
	   measured from the epoch 1900 January 0.5 ET. */

	t = (jd - 2415020.0) / 36525.0;
	t2 = t * t;
	t3 = t2 * t;

	/* Geometric mean longitude of the Sun, referred to the
	   mean equinox of the date. */

	l = fixangle(279.69668 + 36000.76892 * t + 0.0003025 * t2);

        /* Sun's mean anomaly. */

	m = fixangle(358.47583 + 35999.04975*t - 0.000150*t2 - 0.0000033*t3);

        /* Eccentricity of the Earth's orbit. */

	e = 0.01675104 - 0.0000418 * t - 0.000000126 * t2;

	/* Eccentric anomaly. */

	ea = kepler(m, e);

	/* True anomaly */

	v = fixangle(2 * todeg(atan(sqrt((1 + e) / (1 - e))  * tan(ea / 2))));

        /* Sun's true longitude. */

	theta = l + v - m;

	/* Obliquity of the ecliptic. */

	eps = 23.452294 - 0.0130125 * t - 0.00000164 * t2 + 0.000000503 * t3;

        /* Corrections for Sun's apparent longitude, if desired. */

	if (apparent) {
	   omega = fixangle(259.18 - 1934.142 * t);
	   theta = theta - 0.00569 - 0.00479 * sin(torad(omega));
	   eps += 0.00256 * cos(torad(omega));
	}

        /* Return Sun's longitude and radius vector */

	*slong = theta;
	*rv = (1.0000002 * (1 - e * e)) / (1 + e * cos(torad(v)));

	/* Determine solar co-ordinates. */

	epsrad = torad(eps);
	thetarad = torad(theta);
	*ra = atan2((cos(epsrad) * sin(thetarad)), cos(thetarad));
	*ra = fixangle(todeg(*ra));
	*dec = todeg(asin(sin(epsrad)) * sin(thetarad));
}

/*  Astronomical constants  */

#define epoch       2444238.5      /* 1980 January 0.0 */

/*  Constants defining the Sun's apparent orbit  */

#define elonge      278.833540     /* Ecliptic longitude of the Sun
                                      at epoch 1980.0 */
#define elongp      282.596403     /* Ecliptic longitude of the Sun at
                                      perigee */
#define eccent      0.016718       /* Eccentricity of Earth's orbit */
#define sunsmax     1.495985e8     /* Semi-major axis of Earth's orbit, km */
#define sunangsiz   0.533128       /* Sun's angular size, degrees, at
                                      semi-major axis distance */

/*  Elements of the Moon's orbit, epoch 1980.0  */

#define mmlong      64.975464      /* Moon's mean longitude at the epoch */
#define mmlongp     349.383063     /* Mean longitude of the perigee at the
                                      epoch */
#define mlnode      151.950429     /* Mean longitude of the node at the
                                      epoch */
#define minc        5.145396       /* Inclination of the Moon's orbit */
#define mecc        0.054900       /* Eccentricity of the Moon's orbit */
#define mangsiz     0.5181         /* Moon's angular size at distance a
                                      from Earth */
#define msmax       384401.0       /* Semi-major axis of Moon's orbit in km */
#define mparallax   0.9507         /* Parallax at distance a from Earth */
#define synmonth    29.53058868    /* Synodic month (new Moon to new Moon) */
#define lunatbase   2423436.0      /* Base date for E. W. Brown's numbered
                                      series of lunations (1923 January 16) */


/*  JYEAR  --  Convert Julian date to year, month, day, which are
               returned via integer pointers to integers.  */

void jyear(td, yy, mm, dd)
double td;
int *yy, *mm, *dd;
{
        double j, d, y, m;

        td += 0.5;                 /* Astronomical to civil */
        j = floor(td);
        j = j - 1721119.0;
        y = floor(((4 * j) - 1) / 146097.0);
        j = (j * 4.0) - (1.0 + (146097.0 * y));
        d = floor(j / 4.0);
        j = floor(((4.0 * d) + 3.0) / 1461.0);
        d = ((4.0 * d) + 3.0) - (1461.0 * j);
        d = floor((d + 4.0) / 4.0);
        m = floor(((5.0 * d) - 3) / 153.0);
        d = (5.0 * d) - (3.0 + (153.0 * m));
        d = floor((d + 5.0) / 5.0);
        y = (100.0 * y) + j;
        if (m < 10.0)
           m = m + 3;
        else {
           m = m - 9;
           y = y + 1;
        }
        *yy = y;
        *mm = m;
        *dd = d;
}

/*  JHMS  --  Convert Julian time to hour, minutes, and seconds.  */

void jhms(j, h, m, s)
double j;
int *h, *m, *s;
{
        long ij;

        j += 0.5;                  /* Astronomical to civil */
        ij = (j - floor(j)) * 86400.0;
        *h = ij / 3600L;
        *m = (ij / 60L) % 60L;
        *s = ij % 60L;
}

/*  MEANPHASE  --  Calculates mean phase of the Moon for a given
                   base date and desired phase:
                       0.0   New Moon
                       0.25  First quarter
                       0.5   Full moon
                       0.75  Last quarter
                    Beware!!!  This routine returns meaningless
                    results for any other phase arguments.  Don't
                    attempt to generalise it without understanding
                    that the motion of the moon is far more complicated
                    that this calculation reveals. */

double meanphase(sdate, phase, usek)
double sdate, phase;
double *usek;
{
        int yy, mm, dd;
        double k, t, t2, t3, nt1;

        jyear(sdate, &yy, &mm, &dd);

        k = (yy + ((mm - 1) * (1.0 / 12.0)) - 1900) * 12.3685;

        /* Time in Julian centuries from 1900 January 0.5 */
        t = (sdate - 2415020.0) / 36525;
        t2 = t * t;                /* Square for frequent use */
        t3 = t2 * t;               /* Cube for frequent use */

        *usek = k = floor(k) + phase;
        nt1 = 2415020.75933 + synmonth * k
              + 0.0001178 * t2
              - 0.000000155 * t3
              + 0.00033 * dsin(166.56 + 132.87 * t - 0.009173 * t2);

        return nt1;
}

/*  TRUEPHASE  --  Given a K value used to determine the
                   mean phase of the new moon, and a phase
                   selector (0.0, 0.25, 0.5, 0.75), obtain
                   the true, corrected phase time.  */

double truephase(k, phase)
double k, phase;
{
        double t, t2, t3, pt, m, mprime, f;
        int apcor = 0;

        k += phase;                /* Add phase to new moon time */
        t = k / 1236.85;           /* Time in Julian centuries from
                                      1900 January 0.5 */
        t2 = t * t;                /* Square for frequent use */
        t3 = t2 * t;               /* Cube for frequent use */
        pt = 2415020.75933         /* Mean time of phase */
             + synmonth * k
             + 0.0001178 * t2
             - 0.000000155 * t3
             + 0.00033 * dsin(166.56 + 132.87 * t - 0.009173 * t2);

        m = 359.2242               /* Sun's mean anomaly */
            + 29.10535608 * k
            - 0.0000333 * t2
            - 0.00000347 * t3;
        mprime = 306.0253          /* Moon's mean anomaly */
            + 385.81691806 * k
            + 0.0107306 * t2
            + 0.00001236 * t3;
        f = 21.2964                /* Moon's argument of latitude */
            + 390.67050646 * k
            - 0.0016528 * t2
            - 0.00000239 * t3;
        if ((phase < 0.01) || (abs(phase - 0.5) < 0.01)) {

           /* Corrections for New and Full Moon */

           pt +=     (0.1734 - 0.000393 * t) * dsin(m)
                    + 0.0021 * dsin(2 * m)
                    - 0.4068 * dsin(mprime)
                    + 0.0161 * dsin(2 * mprime)
                    - 0.0004 * dsin(3 * mprime)
                    + 0.0104 * dsin(2 * f)
                    - 0.0051 * dsin(m + mprime)
                    - 0.0074 * dsin(m - mprime)
                    + 0.0004 * dsin(2 * f + m)
                    - 0.0004 * dsin(2 * f - m)
                    - 0.0006 * dsin(2 * f + mprime)
                    + 0.0010 * dsin(2 * f - mprime)
                    + 0.0005 * dsin(m + 2 * mprime);
           apcor = !0;
        } else if ((abs(phase - 0.25) < 0.01 || (abs(phase - 0.75) < 0.01))) {
           pt +=     (0.1721 - 0.0004 * t) * dsin(m)
                    + 0.0021 * dsin(2 * m)
                    - 0.6280 * dsin(mprime)
                    + 0.0089 * dsin(2 * mprime)
                    - 0.0004 * dsin(3 * mprime)
                    + 0.0079 * dsin(2 * f)
                    - 0.0119 * dsin(m + mprime)
                    - 0.0047 * dsin(m - mprime)
                    + 0.0003 * dsin(2 * f + m)
                    - 0.0004 * dsin(2 * f - m)
                    - 0.0006 * dsin(2 * f + mprime)
                    + 0.0021 * dsin(2 * f - mprime)
                    + 0.0003 * dsin(m + 2 * mprime)
                    + 0.0004 * dsin(m - 2 * mprime)
                    - 0.0003 * dsin(2 * m + mprime);
           if (phase < 0.5)
              /* First quarter correction */
              pt += 0.0028 - 0.0004 * dcos(m) + 0.0003 * dcos(mprime);
           else
              /* Last quarter correction */
              pt += -0.0028 + 0.0004 * dcos(m) - 0.0003 * dcos(mprime);
           apcor = !0;
        }
        if (!apcor) {
           fprintf(stderr, "TRUEPHASE called with invalid phase selector.\n");
           abort();
        }
        return pt;
}

/*  PHASEHUNT  --  Find time of phases of the moon which surround
                   the current date.  Five phases are found, starting
                   and ending with the new moons which bound the
                   current lunation.  */

void phasehunt(sdate, phases)
double sdate;
double phases[5];
{
        double adate, k1, k2, nt1, nt2;

        adate = sdate - 45;
        nt1 = meanphase(adate, 0.0, &k1);
        while (1) {
           adate += synmonth;
           nt2 = meanphase(adate, 0.0, &k2);
           if (nt1 <= sdate && nt2 > sdate)
              break;
           nt1 = nt2;
           k1 = k2;
        }
        phases[0] = truephase(k1, 0.0);
        phases[1] = truephase(k1, 0.25);
        phases[2] = truephase(k1, 0.5);
        phases[3] = truephase(k1, 0.75);
        phases[4] = truephase(k2, 0.0);
}

/*  PHASE  --  Calculate phase of moon as a fraction:

        The argument is the time for which the phase is requested,
        expressed as a Julian date and fraction.  Returns the terminator
        phase angle as a percentage of a full circle (i.e., 0 to 1),
        and stores into pointer arguments the illuminated fraction of
        the Moon's disc, the Moon's age in days and fraction, the
        distance of the Moon from the centre of the Earth, and the
        angular diameter subtended by the Moon as seen by an observer
        at the centre of the Earth.

*/

double
phase(gtime, lat, lon, pphase, mage, dist, angdia, sudist, suangdia)
time_t gtime;
double *lat;                       /* Latitude in degrees/Earth */
double *lon;                       /* Longitude in degrees/Earth */
double *pphase;                    /* Illuminated fraction */
double *mage;                      /* Age of moon in days */
double *dist;                      /* Distance in kilometres */
double *angdia;                    /* Angular diameter in degrees */
double *sudist;                    /* Distance to Sun */
double *suangdia;                  /* Sun's angular diameter */
{
        struct tm ctp;
	double pdate, xg, yg, zg, xe, ye, ze, ra;
        double Day, N, M, Msin, Ec, 
               Lambdasun, ml, MM, MN, Ev, Ae, A3, MmP, MmPrad,
               mEc, A4, lP, V, lPP, lPPnode, NP, Lambdamoon, BetaM,
               MoonAge, MoonPhase,
               MoonDist, MoonDFrac, MoonAng, MoonPar,
               F, SunDist, SunAng;

        /* Calculation of the Sun's position */

        ctp = *gmtime(&gtime);
        pdate = jtime(&ctp);

        Day = pdate - epoch;        /* Date within epoch */
        N = fixangle((360 / 365.2422) * Day); /* Mean anomaly of the Sun */
        M = fixangle(N + elonge - elongp);    /* Convert from perigee
                                       co-ordinates to epoch 1980.0 */
	Msin = sin(torad(M));
        Ec = kepler(M, eccent);     /* Solve equation of Kepler */
        Ec = sqrt((1 + eccent) / (1 - eccent)) * tan(Ec / 2);
        Ec = 2 * todeg(atan(Ec));   /* True anomaly */
        Lambdasun = fixangle(Ec + elongp);  /* Sun's geocentric ecliptic
                                               longitude */
        /* Orbital distance factor */
        F = ((1 + eccent * cos(torad(Ec))) / (1 - eccent * eccent));
        SunDist = sunsmax / F;      /* Distance to Sun in km */
        SunAng = F * sunangsiz;     /* Sun's angular size in degrees */


        /* Calculation of the Moon's position */

        /* Moon's mean longitude */
        ml = fixangle(13.1763966 * Day + mmlong);

        /* Moon's mean anomaly */
        MM = fixangle(ml - 0.1114041 * Day - mmlongp);

        /* Moon's ascending node mean longitude */
        MN = fixangle(mlnode - 0.0529539 * Day);

        /* Evection */
        Ev = 1.2739 * sin(torad(2 * (ml - Lambdasun) - MM));

        /* Annual equation */
        Ae = 0.1858 * Msin;

        /* Correction term */
        A3 = 0.37 * Msin;

        /* Corrected anomaly */
        MmP = MM + Ev - Ae - A3;
	MmPrad = torad(MmP);

        /* Correction for the equation of the centre */
        mEc = 6.2886 * sin(MmPrad);

        /* Another correction term */
        A4 = 0.214 * sin(2 * MmPrad);

        /* Corrected longitude */
        lP = ml + Ev + mEc - Ae + A4;

        /* Variation */
        V = 0.6583 * sin(torad(2 * (lP - Lambdasun)));

        /* True longitude */
        lPP = lP + V;

        /* Corrected longitude of the node */
        NP = MN - 0.16 * Msin;

	/* Relative Longitude with respect to the node */
        lPPnode = torad(lPP - NP);

        /* x, y, z ecliptic coordinates with respect to node */
        xe = cos(lPPnode);
        ye = sin(lPPnode) * cos(torad(minc));
	ze = sin(lPPnode) * sin(torad(minc));

        /* Ecliptic latitude */
        BetaM = todeg(asin(ze));

        /* Ecliptic longitude */
        Lambdamoon = todeg(atan2(ye, xe));
        Lambdamoon += NP;

        /* x, y, z sidereal ecliptic coordinates  */
        xe = cos(torad(Lambdamoon));
        ye = sin(torad(Lambdamoon));

        /* Calculation of the phase of the Moon */

        /* Age of the Moon in degrees */
        MoonAge = lPP - Lambdasun;

        /* Phase of the Moon */
        MoonPhase = (1 - cos(torad(MoonAge))) / 2;

        /* Calculate distance of moon from the centre of the Earth */

        MoonDist = (msmax * (1 - mecc * mecc)) /
           (1 + mecc * cos(torad(MmP + mEc)));

        /* Calculate Moon's angular diameter */

        MoonDFrac = MoonDist / msmax;
        MoonAng = mangsiz / MoonDFrac;

        /* Calculate Moon's parallax */

        MoonPar = mparallax / MoonDFrac;

        *pphase = MoonPhase;

        *mage = synmonth * (fixangle(MoonAge) / 360.0);
        *dist = MoonDist;
        *angdia = MoonAng;
        *sudist = SunDist;
        *suangdia = SunAng;

       /* Rotate to equatorial geocentric coords
          taking into account obliquity of ecliptic of date */

        xg = xe;
        yg = ye * cos(epsrad) - ze * sin(epsrad);
        zg = ye * sin(epsrad) + ze * cos(epsrad);

        ra = todeg(atan2(yg, xg));
        *lat = todeg(asin(zg));
        *lon = fixangle((ra + 180.0 - (gmst(pdate) * 15.0))) - 180.0;

        return fixangle(MoonAge) / 360.0;
}
