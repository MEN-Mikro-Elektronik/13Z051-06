/***********************  I n c l u d e  -  F i l e  ***********************/
/*!
 *        \file  z51_drv.h
 *
 *      \author  ub
 *
 *       \brief  Header file for Z51 driver containing
 *               Z51 specific status codes and
 *               Z51 function prototypes
 *
 *    \switches  _ONE_NAMESPACE_PER_DRIVER_
 *               _LL_DRV_
 */
 /*
 *---------------------------------------------------------------------------
 * Copyright (c) 2003-2019, MEN Mikro Elektronik GmbH
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

#ifndef _Z51_DRV_H
#define _Z51_DRV_H

#ifdef __cplusplus
      extern "C" {
#endif


/*-----------------------------------------+
|  TYPEDEFS                                |
+-----------------------------------------*/
/* none */

/*-----------------------------------------+
|  DEFINES                                 |
+-----------------------------------------*/
/** \name Z51 specific Getstat/Setstat standard codes 
 *  \anchor getstat_setstat_codes
 */
/**@{*/
#define Z51_OFFSET          M_DEV_OF+0x00   /**< G,S: Offset value for calibration */
#define Z51_GAIN            M_DEV_OF+0x01   /**< G,S: Gain value for calibration */
#define Z51_POWERDOWN       M_DEV_OF+0x02   /**< G,S: Power-down mode */
#define Z51_SET_SIGNAL      M_DEV_OF+0x03   /**<   S: Set signal sent on IRQ */
#define Z51_CLR_SIGNAL      M_DEV_OF+0x04   /**<   S: Uninstall signal */
/**@}*/


#ifndef  Z51_VARIANT
# define Z51_VARIANT       Z51
#endif

# define _Z51_GLOBNAME(var,name) var##_##name
#ifndef _ONE_NAMESPACE_PER_DRIVER_
# define Z51_GLOBNAME(var,name) _Z51_GLOBNAME(var,name)
#else
# define Z51_GLOBNAME(var,name) _Z51_GLOBNAME(Z51,name)
#endif

#define __Z51_GetEntry          Z51_GLOBNAME(Z51_VARIANT, GetEntry)

/*-----------------------------------------+
|  PROTOTYPES                              |
+-----------------------------------------*/
#ifdef _LL_DRV_
#ifndef _ONE_NAMESPACE_PER_DRIVER_
	extern void __Z51_GetEntry(LL_ENTRY* drvP);
#endif
#endif /* _LL_DRV_ */

/*-----------------------------------------+
|  BACKWARD COMPATIBILITY TO MDIS4         |
+-----------------------------------------*/
#ifndef U_INT32_OR_64
 /* we have an MDIS4 men_types.h and mdis_api.h included */
 /* only 32bit compatibility needed!                     */
 #define INT32_OR_64  int32
 #define U_INT32_OR_64 u_int32
 typedef INT32_OR_64  MDIS_PATH;
#endif /* U_INT32_OR_64 */

#ifdef __cplusplus
      }
#endif

#endif /* _Z51_DRV_H */

