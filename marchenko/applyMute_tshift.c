#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <assert.h>

#ifndef MAX
#define MAX(x,y) ((x) > (y) ? (x) : (y))
#endif
#ifndef MIN
#define MIN(x,y) ((x) < (y) ? (x) : (y))
#endif
#define NINT(x) ((int)((x)>0.0?(x)+0.5:(x)-0.5))
#ifndef COMPLEX
typedef struct _complexStruct { /* complex number */
    float r,i;
} complex;
#endif/* complex */


void applyMute_tshift( float *data, int *mute, int smooth, int above, int Nfoc, int nxs, int nt, int *ixpos, int npos, int shift, int iter, int *tsynW)
{
    int i, j, l, isyn;
    float *costaper, scl, *Nig;
    int imute, tmute, ts;

    if (smooth) {
        costaper = (float *)malloc(smooth*sizeof(float));
        scl = M_PI/((float)smooth);
        for (i=0; i<smooth; i++) {
            costaper[i] = 0.5*(1.0+cos((i+1)*scl));
        }
    }

    Nig = (float *)malloc(nt*sizeof(float));

    for (isyn = 0; isyn < Nfoc; isyn++) {
        for (i = 0; i < npos; i++) {
            if (iter % 2 == 0) { 
                for (j = 0; j < nt; j++) {
                    Nig[j]   = data[isyn*nxs*nt+i*nt+j];
                }
            }
            else { // reverse back in time
                j=0;
                Nig[j]   = data[isyn*nxs*nt+i*nt+j];
                for (j = 1; j < nt; j++) {
                    Nig[j]   = data[isyn*nxs*nt+i*nt+nt-j];
                }
            }
            if (above==1) {
                imute = ixpos[i];
                tmute = mute[isyn*nxs+imute];
				ts = tsynW[isyn*nxs+imute];
                for (j = 0; j < MAX(0,tmute-shift-smooth); j++) {
                    Nig[j] = 0.0;
                }
                for (j = MAX(0,tmute-shift-smooth),l=0; j < MAX(0,tmute-shift); j++,l++) {
                    Nig[j] *= costaper[smooth-l-1];
                }
            }
            else if (above==0){
                imute = ixpos[i];
                tmute = mute[isyn*nxs+imute];
				ts = tsynW[isyn*nxs+imute];
                if (tmute >= nt/2) {
                    memset(&Nig[0],0, sizeof(float)*nt);
                    continue;
                }
                for (j = MAX(0,tmute-shift),l=0; j < MAX(0,tmute-shift+smooth); j++,l++) {
                    Nig[j] *= costaper[l];
                }
                for (j = MAX(0,tmute-shift+smooth+1); j < MIN(nt,nt+1-tmute+2*ts+shift-smooth); j++) {
                    Nig[j] = 0.0;
                }
                for (j = MIN(nt-1,nt-tmute+2*ts+shift-smooth),l=0; j < MIN(nt,nt-tmute+shift); j++,l++) {
                    Nig[j] *= costaper[smooth-l-1];
                }
            }
            else if (above==-1) {
                imute = ixpos[i];
                tmute = mute[isyn*nxs+imute];
				ts = tsynW[isyn*nxs+imute];
				//ts = tsynW[isyn*nxs+ixpos[npos-1]];
                for (j = ts+tmute-shift,l=0; j < ts+tmute-shift+smooth; j++,l++) {
                    Nig[j] *= costaper[l];
                }
                for (j = ts+tmute-shift+smooth; j < nt; j++) {
                    Nig[j] = 0.0;
                }
            }
            else if (above==4) { //Psi gate which is the inverse of the Theta gate (above=0)
                imute = ixpos[i];
                tmute = mute[isyn*nxs+imute];
				ts = tsynW[isyn*nxs+imute];
                for (j = -2*ts+tmute-shift-smooth,l=0; j < -2*ts+tmute-shift; j++,l++) {
                    Nig[j] *= costaper[smooth-l-1];
                }
                for (j = 0; j < -2*ts+tmute-shift-smooth-1; j++) {
                    Nig[j] = 0.0;
                }
                for (j = nt+1-tmute+shift+smooth; j < nt; j++) {
                    Nig[j] = 0.0;
                }
                for (j = nt-tmute+shift,l=0; j < nt-tmute+shift+smooth; j++,l++) {
                    Nig[j] *= costaper[l];
                }
            }
/* To Do above==2 is not yet adapated for plane-waves */
            else if (above==2){//Separates the direct part of the wavefield from the coda
                imute = ixpos[i];
                tmute = mute[isyn*nxs+imute];
                for (j = 0; j < tmute-shift-smooth; j++) {
                    data[isyn*nxs*nt+i*nt+j] = 0.0;
                }
                for (j = tmute-shift-smooth,l=0; j < tmute-shift; j++,l++) {
                    data[isyn*nxs*nt+i*nt+j] *= costaper[smooth-l-1];
                }
                for (j = tmute+shift,l=0; j < tmute+shift+smooth; j++,l++) {
                    data[isyn*nxs*nt+i*nt+j] *= costaper[l];
                }
                for (j = tmute+shift+smooth; j < nt; j++) {
                    data[isyn*nxs*nt+i*nt+j] = 0.0;
                }
            }

            if (iter % 2 == 0) { 
                for (j = 0; j < nt; j++) {
                    data[isyn*nxs*nt+i*nt+j] = Nig[j];
                }
            }
            else { // reverse back in time
                j=0;
                data[isyn*nxs*nt+i*nt+j] = Nig[j];
                for (j = 1; j < nt; j++) {
                    data[isyn*nxs*nt+i*nt+j] = Nig[nt-j];
                }
            }
        } /* end if ipos */
    }

    if (smooth) free(costaper);
	free(Nig);

    return;
}

void timeShift(float *data, int nsam, int nrec, float dt, float shift, float fmin, float fmax)
{
	int 	optn, iom, iomin, iomax, nfreq, ix, it;
	float	omin, omax, deltom, om, tom, df, *trace, scl;
	complex *ctrace, ctmp;

	optn = optncr(nsam);
	nfreq = optn/2+1;
	df    = 1.0/(optn*dt);

	ctrace = (complex *)malloc(nfreq*sizeof(complex));
	if (ctrace == NULL) verr("memory allocation error for ctrace");

	trace = (float *)malloc(optn*sizeof(float));
	if (trace == NULL) verr("memory allocation error for rdata");

	deltom = 2.*M_PI*df;
	omin   = 2.*M_PI*fmin;
	omax   = 2.*M_PI*fmax;
	iomin  = (int)MIN((omin/deltom), (nfreq));
	iomax  = MIN((int)(omax/deltom), (nfreq));
    scl = 1.0/(float)optn;

	for (ix = 0; ix < nrec; ix++) {
        for (it=0;it<nsam;it++)    trace[it]=data[ix*nsam+it];
        for (it=nsam;it<optn;it++) trace[it]=0.0;
	    /* Forward time-frequency FFT */
	    rc1fft(&trace[0], &ctrace[0], optn, -1);

		for (iom = 0; iom < iomin; iom++) {
			ctrace[iom].r = 0.0;
			ctrace[iom].i = 0.0;
		}
		for (iom = iomax; iom < nfreq; iom++) {
			ctrace[iom].r = 0.0;
			ctrace[iom].i = 0.0;
		}
		for (iom = iomin ; iom < iomax ; iom++) {
			om = deltom*iom;
			tom = om*shift;
			ctmp = ctrace[iom];
			ctrace[iom].r = ctmp.r*cos(-tom) - ctmp.i*sin(-tom);
			ctrace[iom].i = ctmp.i*cos(-tom) + ctmp.r*sin(-tom);
		}
        /* Inverse frequency-time FFT and scale result */
        cr1fft(ctrace, trace, optn, 1);
        for (it=0;it<nsam;it++) data[ix*nsam+it]=trace[it]*scl;
	}


    free(ctrace);
    free(trace);

    return;
}
