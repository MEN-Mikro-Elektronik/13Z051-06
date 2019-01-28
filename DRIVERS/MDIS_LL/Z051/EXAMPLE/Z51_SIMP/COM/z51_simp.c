/****************************************************************************
 ************                                                    ************
 ************                   Z51_SIMP                         ************
 ************                                                    ************
 ****************************************************************************/
/*!
 *         \file z51_simp.c
 *       \author ub
 *        $Date: 2017/04/28 11:33:13 $
 *    $Revision: 1.5 $
 *
 *       \brief  Simple example program for the Z51 driver
 *
 *               Causes the Z51 to set its outputs either on a constant 
 *               current/voltage or output a sawtooth wave on both outputs.
 *               See usage info.
 *
 *     Required: libraries: mdis_api, usr_oss
 *     \switches (none)
 */
 /*
 *---------------------------------------------------------------------------
 * (c) Copyright 2003 by MEN mikro elektronik GmbH, Nuernberg, Germany
 ****************************************************************************/
/*
* This program is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 2 of the License, or
* (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <MEN/men_typs.h>
#include <MEN/mdis_api.h>
#include <MEN/usr_oss.h>
#include <MEN/z51_drv.h>

/*--------------------------------------+
|   DEFINES                             |
+--------------------------------------*/
#define FAIL_UNLESS(expression) \
 if( !(expression)) {\
     printf("\n*** Error during: %s\nfile %s\nline %d\n", \
      #expression,__FILE__,__LINE__);\
      printf("%s\n",M_errstring(UOS_ErrnoGet()));\
    ret = 1; \
     goto ABORT;\
 }

/*--------------------------------------+
|   TYPDEFS                             |
+--------------------------------------*/
/* none */

/*--------------------------------------+
|   EXTERNALS                           |
+--------------------------------------*/
/* none */

/*--------------------------------------+
|   GLOBALS                             |
+--------------------------------------*/
static int G_sigCount;

/*--------------------------------------+
|   PROTOTYPES                          |
+--------------------------------------*/
static void __MAPILIB SignalHandler( u_int32 sig );


/********************************* main ************************************/
/** Program main function
 *
 *  \param argc       \IN  argument counter
 *  \param argv       \IN  argument vector
 *
 *  \return	          success (0) or error (1)
 */
int main(int argc, char *argv[])
{
	int32		chan, i, oldSigCount, value;
	int32		delay = -1, time = -1, step = 1;
	u_int32		startTime, currTime, deltaTime;
	MDIS_PATH 	path;
	char		*device;
	int			ret=0;


	if (argc < 4 || strcmp(argv[1],"-?")==0) {
		printf("Syntax: z51_simp <device> <chan> <value> [<delay>] [<step>] [<time>]\n");
		printf("Function: Z51 example for outputting analogue values\n");
		printf("Options:\n");
		printf("    device       device name\n");
		printf("    chan         channel number (0..2)\n");
		printf("                   0,1=channel 0 or 1\n");
		printf("                     2=channel 0 and 1\n");
		printf("    value        output value (0..65535)\n");
		printf("                   -1 to generate sawtooth wave\n");
		printf("    delay        time [ms] to output each value\n");
		printf("                   if omitted fast as possible\n");
		printf("    step         step width for sawtooth waves\n");
		printf("                   if omitted increment one\n");
		printf("    time         time [ms] to generated sawtooth waves\n");
		printf("                   if omitted output endless until ctrl-C\n");
		printf("\n");
		return(1);
	}

	device = argv[1];
	chan   = atoi( argv[2] );

        value = strtol( argv[3], NULL, 0 );

    if( argc > 4 )
        delay = strtol( argv[4], NULL, 0 );

    if( argc > 5 )
        step = strtol( argv[5], NULL, 0 );

    if( argc > 6 )
        time = strtol( argv[6], NULL, 0 );

	/*--------------------+
    |  open path          |
    +--------------------*/
	if ((path = M_open(device)) < 0) {
        printf( "open failed: %s\n",M_errstring(UOS_ErrnoGet()) );
		return(1);
	}

	/*--------------------+
    |  config             |
    +--------------------*/
	/* channel number */
	FAIL_UNLESS( (M_setstat(path, M_MK_CH_CURRENT, chan)) == 0 );

    /* initialize signal handler */
    UOS_SigInit( SignalHandler );
    UOS_SigInstall( UOS_SIG_USR1 );
    M_setstat( path, Z51_SET_SIGNAL, UOS_SIG_USR1 );
    oldSigCount = G_sigCount = 0;


	/*--------------------+
    |  output values      |
    +--------------------*/
    if( value == -1 ) {

		if( time != -1 ){
	    	startTime = UOS_MsecTimerGet();
	    }
   	
        for( ;; ) {
            for( i=0; i<65535; i+=step ) {

                /* for channel 2 set both outputs to same value */
                if( chan >= 2 ) {
                    FAIL_UNLESS( M_write( path, (i<<16) + i ) == 0 );
                }
                else {
                    FAIL_UNLESS( M_write( path, i ) == 0 );
                }

				if( delay != -1 )
			    	UOS_Delay( delay );
            }

            /* report signal */
            if( oldSigCount != G_sigCount ) {
                printf( "Got signal (count=%d)\n", G_sigCount );
                oldSigCount = G_sigCount;
            }
            
       		if( time != -1 ){
		    	currTime = UOS_MsecTimerGet();
		    	
		    	if( currTime < startTime )
		    		deltaTime = (0xffffffff - startTime) + currTime;
		    	else
		    		deltaTime = currTime - startTime;
		    	
		    	if( deltaTime > (u_int32)time )
		    		goto ABORT;
		    }
        }
    }
    else {
        if( chan >= 2 ) {
            FAIL_UNLESS( M_write( path, (value<<16) + value ) == 0 );
        }
        else {
            FAIL_UNLESS( M_write( path, value ) == 0 );
        }

		if( delay != -1 ){
	    	UOS_Delay( delay );
    }
	    else{
    printf( "Hit any key to finish program\n" );
    getchar();
	    }
    }

	/*--------------------+
    |  cleanup            |
    +--------------------*/
 ABORT:
    M_setstat( path, Z51_CLR_SIGNAL, 0 );
    UOS_SigRemove( UOS_SIG_USR1 );
    UOS_SigExit();

	if (M_close(path) < 0)
        printf( "close failed: %s\n",M_errstring(UOS_ErrnoGet()) );

	return(ret);
}

/**********************************************************************/
/** Signal handler
 *
 *  \param  sig    \IN   received signal
 */
static void __MAPILIB SignalHandler( u_int32 sig )
{
    if( sig == UOS_SIG_USR1 ) {
        ++G_sigCount;
    }
}

