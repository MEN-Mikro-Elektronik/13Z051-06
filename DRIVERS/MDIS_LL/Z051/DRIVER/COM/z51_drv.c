/*********************  P r o g r a m  -  M o d u l e ***********************/
/*!
 *        \file  z51_drv.c
 *
 *      \author  ub/ls
 *        $Date: 2017/04/28 11:32:57 $
 *    $Revision: 1.4 $
 *
 *      \brief   Low-level driver for Z51 "Edmonton" DAC on F401 Rev.01
 *               Calibration done in software.
 *
 *     Required: OSS, DESC, DBG, ID libraries
 *
 *     \switches _ONE_NAMESPACE_PER_DRIVER_
 */
 /*
 *---------------------------------------------------------------------------
 * (c) Copyright 2004 by MEN Mikro Elektronik GmbH, Nuernberg, Germany
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
#define _NO_LL_HANDLE       /* ll_defs.h: don't define LL_HANDLE struct */

#include <MEN/men_typs.h>   /* system dependent definitions   */
#include <MEN/maccess.h>    /* hw access macros and types     */
#include <MEN/dbg.h>        /* debug functions                */
#include <MEN/oss.h>        /* oss functions                  */
#include <MEN/desc.h>       /* descriptor functions           */
#include <MEN/modcom.h>     /* ID PROM functions              */
#include <MEN/mdis_api.h>   /* MDIS global defs               */
#include <MEN/mdis_com.h>   /* MDIS common defs               */
#include <MEN/mdis_err.h>   /* MDIS error codes               */
#include <MEN/ll_defs.h>    /* low-level driver definitions   */

/*-----------------------------------------+
|  DEFINES                                 |
+-----------------------------------------*/
/* general defines */
#define CH_NUMBER           3           /**< number of device channels */
#define USE_IRQ             TRUE        /**< interrupt required  */
#define ADDRSPACE_COUNT     1           /**< nbr of required address spaces */
#define ADDRSPACE_SIZE      256         /**< size of address space */

#define OSH                 (llHdl->osHdl)

/* debug defines */
#define DBG_MYLEVEL         llHdl->dbgLevel   /**< debug level */
#define DBH                 llHdl->dbgHdl     /**< debug handle */


/* hardware specific defines */

/* register offsets */
#define DAC_CTRL_REG        0x00        /* DAC command register */
#define DAC_SCLK_REG        0x04        /* DAC serial clock register */
#define DAC_IRQ_REG         0x08        /* DAC interrupt request register */
#define DAC_IER_REG         0x0c        /* DAC interrupt enable register */

/* bit definitions for DAC_IRQ_REG and DAC_IER_REG */
#define DAC_IRQ_MASK        0x00000001  /* interrupt flag */

/* default values */
#define DAC_SCLK_DEFAULT    0x0002      /* 6 PCI clocks cycle time */

#define DAC_OFFSET_DEFAULT_0  0x1985    /* default offset value */
#define DAC_GAIN_DEFAULT_0    0xCE3E    /* default gain value */
#define DAC_OFFSET_DEFAULT_1  0x1951    /* default offset value */
#define DAC_GAIN_DEFAULT_1    0xCDD3    /* default gain value */

/* DAC commands */
#define DAC_CMD_LOAD_A      0x100000    /* set output A */
#define DAC_CMD_LOAD_B      0x200000    /* set output B */
#define DAC_CMD_LOAD_AB     0x300000    /* set outputs A and B */
#define DAC_CMD_BUF_A       0x000000    /* write to buffer A */
#define DAC_CMD_BUF_B       0x040000    /* write to buffer B */
#define DAC_CMD_PD_1K       0x010000    /* powerdown with out impedance 1kOhm*/
#define DAC_CMD_PD_100K     0x020000    /* powerdown 100kOhm */
#define DAC_CMD_PD_HIGHZ    0x030000    /* powerdown high impedance */


/*-----------------------------------------+
|  TYPEDEFS                                |
+-----------------------------------------*/
/** low-level handle */
typedef struct {
    /* general */
    int32           memAlloc;       /**< size allocated for the handle */
    OSS_HANDLE      *osHdl;         /**< oss handle */
    OSS_IRQ_HANDLE  *irqHdl;        /**< irq handle */
    DESC_HANDLE     *descHdl;       /**< desc handle */
    MACCESS         ma;             /**< hw access handle */
    MDIS_IDENT_FUNCT_TBL idFuncTbl; /**< id function table */
    /* debug */
    u_int32         dbgLevel;       /**< debug level */
    DBG_HANDLE      *dbgHdl;        /**< debug handle */
    /* misc */
    u_int32         irqCount;       /**< interrupt counter */
    /* device specific */
    u_int32         irqEnable;      /**< enable irq on driver init */
    u_int32         offset[2];      /**< offset parameter */
    u_int32         gain[2];        /**< gain parameter */
    u_int32         powerdown[2];   /**< powerdown mode */
    int             initDac;        /**< init data communication and IRQ */
    OSS_SIG_HANDLE  *hwSig;         /**< signal for hardware malfunction */
} LL_HANDLE;

/* include files which need LL_HANDLE */
#include <MEN/ll_entry.h>           /* low-level driver jump table  */
#include <MEN/z51_drv.h>            /* Z51 driver header file */

/*-----------------------------------------+
|  PROTOTYPES                              |
+-----------------------------------------*/
static int32 Z51_Init(DESC_SPEC *descSpec, OSS_HANDLE *osHdl,
                       MACCESS *ma, OSS_SEM_HANDLE *devSemHdl,
                       OSS_IRQ_HANDLE *irqHdl, LL_HANDLE **llHdlP);
static int32 Z51_Exit(LL_HANDLE **llHdlP );
static int32 Z51_Read(LL_HANDLE *llHdl, int32 ch, int32 *value);
static int32 Z51_Write(LL_HANDLE *llHdl, int32 ch, int32 value);
static int32 Z51_SetStat(LL_HANDLE *llHdl,int32 ch, int32 code, INT32_OR_64 value32_or_64);
static int32 Z51_GetStat(LL_HANDLE *llHdl, int32 ch, int32 code, INT32_OR_64 *value32_or_64P);
static int32 Z51_BlockRead(LL_HANDLE *llHdl, int32 ch, void *buf, int32 size,
                            int32 *nbrRdBytesP);
static int32 Z51_BlockWrite(LL_HANDLE *llHdl, int32 ch, void *buf, int32 size,
                             int32 *nbrWrBytesP);
static int32 Z51_Irq(LL_HANDLE *llHdl );
static int32 Z51_Info(int32 infoType, ... );

static char* Ident( void );
static int32 Cleanup(LL_HANDLE *llHdl, int32 retCode);
static u_int16 calibrate( LL_HANDLE *llHdl, u_int16 value, 
                          int offset, int gain );


/****************************** Z51_GetEntry ********************************/
/** Initialize driver's jump table
 *
 *  \param drvP     \OUT pointer to the initialized jump table structure
 */
#ifdef _ONE_NAMESPACE_PER_DRIVER_
    extern void LL_GetEntry( LL_ENTRY* drvP )
#else
    extern void __Z51_GetEntry( LL_ENTRY* drvP )
#endif
{
    drvP->init        = Z51_Init;
    drvP->exit        = Z51_Exit;
    drvP->read        = Z51_Read;
    drvP->write       = Z51_Write;
    drvP->blockRead   = Z51_BlockRead;
    drvP->blockWrite  = Z51_BlockWrite;
    drvP->setStat     = Z51_SetStat;
    drvP->getStat     = Z51_GetStat;
    drvP->irq         = Z51_Irq;
    drvP->info        = Z51_Info;
}

/******************************** Z51_Init **********************************/
/** Allocate and return low-level handle, initialize hardware
 *
 * The function initializes all channels with the definitions made
 * in the descriptor. The interrupt is disabled.
 *
 * The following descriptor keys are used:
 *
 * \code
 * Descriptor key        Default          Range
 * --------------------  ---------------  -------------
 * DEBUG_LEVEL_DESC      OSS_DBG_DEFAULT  see dbg.h
 * DEBUG_LEVEL           OSS_DBG_DEFAULT  see dbg.h
 * ID_CHECK              1                0..1
 * \endcode
 *
 *  \param descP      \IN  pointer to descriptor data
 *  \param osHdl      \IN  oss handle
 *  \param ma         \IN  hw access handle
 *  \param devSemHdl  \IN  device semaphore handle
 *  \param irqHdl     \IN  irq handle
 *  \param llHdlP     \OUT pointer to low-level driver handle
 *
 *  \return           \c 0 on success or error code
 */
static int32 Z51_Init(
    DESC_SPEC       *descP,
    OSS_HANDLE      *osHdl,
    MACCESS         *ma,
    OSS_SEM_HANDLE  *devSemHdl,
    OSS_IRQ_HANDLE  *irqHdl,
    LL_HANDLE       **llHdlP
)
{
    LL_HANDLE *llHdl = NULL;
    u_int32 gotsize;
    int32 error;
    u_int32 value;

    /*------------------------------+
    |  prepare the handle           |
    +------------------------------*/
    *llHdlP = NULL;     /* set low-level driver handle to NULL */

    /* alloc */
    if ((llHdl = (LL_HANDLE*)OSS_MemGet(
                    osHdl, sizeof(LL_HANDLE), &gotsize)) == NULL)
       return(ERR_OSS_MEM_ALLOC);

    /* clear */
    OSS_MemFill(osHdl, gotsize, (char*)llHdl, 0x00);

    /* init */
    llHdl->memAlloc   = gotsize;
    llHdl->osHdl      = osHdl;
    llHdl->irqHdl     = irqHdl;
    llHdl->ma         = *ma;

    /*------------------------------+
    |  init id function table       |
    +------------------------------*/
    /* driver's ident function */
    llHdl->idFuncTbl.idCall[0].identCall = Ident;
    /* library's ident functions */
    llHdl->idFuncTbl.idCall[1].identCall = DESC_Ident;
    llHdl->idFuncTbl.idCall[2].identCall = OSS_Ident;
    /* terminator */
    llHdl->idFuncTbl.idCall[3].identCall = NULL;

    /*------------------------------+
    |  prepare debugging            |
    +------------------------------*/
    DBG_MYLEVEL = OSS_DBG_DEFAULT;  /* set OS specific debug level */
    DBGINIT((NULL,&DBH));

    /*------------------------------+
    |  scan descriptor              |
    +------------------------------*/
    /* prepare access */
    if ((error = DESC_Init(descP, osHdl, &llHdl->descHdl)))
        return( Cleanup(llHdl,error) );

    /* DEBUG_LEVEL_DESC */
    if ((error = DESC_GetUInt32(llHdl->descHdl, OSS_DBG_DEFAULT,
                                &value, "DEBUG_LEVEL_DESC")) &&
        error != ERR_DESC_KEY_NOTFOUND)
        return( Cleanup(llHdl,error) );

    DESC_DbgLevelSet(llHdl->descHdl, value);    /* set level */

    /* DEBUG_LEVEL */
    if ((error = DESC_GetUInt32(llHdl->descHdl, OSS_DBG_DEFAULT,
                                &llHdl->dbgLevel, "DEBUG_LEVEL")) &&
        error != ERR_DESC_KEY_NOTFOUND)
        return( Cleanup(llHdl,error) );

    DBGWRT_1((DBH, "LL - Z51_Init\n"));

    /* IRQ_ENABLE */
    if ((error = DESC_GetUInt32(llHdl->descHdl, 0,
                                &llHdl->irqEnable, "IRQ_ENABLE")) &&
        error != ERR_DESC_KEY_NOTFOUND)
        return( Cleanup(llHdl,error) );

    /* Z51_OFFSET_0 */
    if ((error = DESC_GetUInt32(llHdl->descHdl, DAC_OFFSET_DEFAULT_0,
                                &llHdl->offset[0], "Z51_OFFSET_0")) &&
        error != ERR_DESC_KEY_NOTFOUND)
        return( Cleanup(llHdl,error) );

    /* Z51_GAIN_0 */
    if ((error = DESC_GetUInt32(llHdl->descHdl, DAC_GAIN_DEFAULT_0,
                                &llHdl->gain[0], "Z51_GAIN_0")) &&
        error != ERR_DESC_KEY_NOTFOUND)
        return( Cleanup(llHdl,error) );

    /* Z51_OFFSET_1 */
    if ((error = DESC_GetUInt32(llHdl->descHdl, DAC_OFFSET_DEFAULT_1,
                                &llHdl->offset[1], "Z51_OFFSET_1")) &&
        error != ERR_DESC_KEY_NOTFOUND)
        return( Cleanup(llHdl,error) );

    /* Z51_GAIN_1 */
    if ((error = DESC_GetUInt32(llHdl->descHdl, DAC_GAIN_DEFAULT_1,
                                &llHdl->gain[1], "Z51_GAIN_1")) &&
        error != ERR_DESC_KEY_NOTFOUND)
        return( Cleanup(llHdl,error) );

    /* tell write routine to init DAC communication and IRQ */
    llHdl->initDac = 1;
    llHdl->powerdown[0] = llHdl->powerdown[1] = 0;

    DBGWRT_3((DBH, "Z51_Init: offset=%d,%d  gain=%d,%d\n", 
              llHdl->offset[0],llHdl->offset[1],
              llHdl->gain[0], llHdl->gain[1] ));

    /*------------------------------+
    |  hardware init in Z51_Write() |
    +------------------------------*/

    *llHdlP = llHdl;    /* set low-level driver handle */

    return(ERR_SUCCESS);
}

/****************************** Z51_Exit ************************************/
/** De-initialize hardware and clean up memory
 *
 *  The function deinitializes all channels by setting them to zero milivolts.
 *  The interrupt is disabled.
 *
 *  \param llHdlP      \IN  pointer to low-level driver handle
 *
 *  \return           \c 0 on success or error code
 */
static int32 Z51_Exit(
   LL_HANDLE    **llHdlP
)
{
    LL_HANDLE *llHdl = *llHdlP;
    MACCESS   ma = llHdl->ma;
    int32     error = 0;

    DBGWRT_1((DBH, "LL - Z51_Exit\n"));

    /*------------------------------+
    |  de-init hardware             |
    +------------------------------*/

    /* set DAC outputs to zero */
    MWRITE_D32( ma, DAC_CTRL_REG,
                DAC_CMD_BUF_A | 0 );
	OSS_MikroDelay(OSH, 1);
    MWRITE_D32( ma, DAC_CTRL_REG,
                DAC_CMD_LOAD_AB | DAC_CMD_BUF_B | 0 );
    OSS_Delay( OSH, 100 );

    /* disable interrupt */
    MWRITE_D32( ma, DAC_IER_REG, 0 );

    /* turn off outputs using F401 watchdog mechanism */
    MWRITE_D32( ma, DAC_SCLK_REG, 0 );

    /*------------------------------+
    |  clean up memory               |
    +------------------------------*/
    *llHdlP = NULL;     /* set low-level driver handle to NULL */
    error = Cleanup(llHdl,error);

    return(error);
}

/****************************** Z51_Read ************************************/
/** Read a value from the device
 *
 *  Dummy function.
 *
 *  \param llHdl      \IN  low-level handle
 *  \param ch         \IN  current channel
 *  \param valueP     \OUT read value
 *
 *  \return           \c 0 on success or error code
 */
static int32 Z51_Read(
    LL_HANDLE *llHdl,
    int32 ch,
    int32 *valueP
)
{
    DBGWRT_1((DBH, "LL - Z51_Read: ch=%d\n",ch));

    return(ERR_LL_ILL_FUNC);
}

/****************************** Z51_Write ***********************************/
/** Description:  Write a value to the device
 *
 *  The function writes a value to the current channel.
 *
 *  \param llHdl      \IN  low-level handle
 *  \param ch         \IN  current channel
 *  \param value      \IN read value
 *
 *  \return           \c 0 on success or error code
 */
static int32 Z51_Write(
    LL_HANDLE *llHdl,
    int32 ch,
    int32 value
)
{
    MACCESS   ma = llHdl->ma;

    DBGWRT_1((DBH, "LL - Z51_Write: ch=%d val=0x%x\n",ch, value));

    if( !IN_RANGE( ch, 0, CH_NUMBER-1 ) )
        return( ERR_LL_ILL_CHAN );

    /*
     * If this is the first write we have to initialize the communication
     * between the FPGA and the DAC. This also connects the DAC's outputs to
     * the output drivers of the module.
     * So before the first access we have always 0mA output current (on F401).
     */
    if( llHdl->initDac ) {
        MWRITE_D32( ma, DAC_SCLK_REG, DAC_SCLK_DEFAULT );

        /*
         * In order to avoid an unwanted interrupt we have to wait for the
         * watchdog circuit to release the IRQ input before we can enable
         * the interrupt.
         */
        if( llHdl->irqEnable ) {
            DBGWRT_3((DBH, "delay for watchdog to come up...\n"));
            OSS_Delay( OSH, 1010 );  /* worst case = 1000ms */
            MWRITE_D32( ma, DAC_IER_REG, DAC_IRQ_MASK );
        }
        llHdl->initDac = 0;
    }

    /* dependant on channel set output A, B or both */
    switch( ch ) {
        case 0:
            MWRITE_D32( ma, DAC_CTRL_REG,
                        DAC_CMD_LOAD_A | DAC_CMD_BUF_A |
                        calibrate( llHdl, (u_int16)value, 
                                   llHdl->offset[0], llHdl->gain[0] ));
            break;

        case 1:
            MWRITE_D32( ma, DAC_CTRL_REG, 
                        DAC_CMD_LOAD_B | DAC_CMD_BUF_B |
                        calibrate( llHdl, (u_int16)value, 
                                   llHdl->offset[1], llHdl->gain[1] ));
            break;

        default:
            MWRITE_D32( ma, DAC_CTRL_REG,
                        DAC_CMD_BUF_A |
                        calibrate( llHdl, (u_int16)value, 
                                   llHdl->offset[0], llHdl->gain[0] ) );
                        
			OSS_MikroDelay(OSH, 1);

            MWRITE_D32( ma, DAC_CTRL_REG,
                        DAC_CMD_LOAD_AB | DAC_CMD_BUF_B | 
                        calibrate( llHdl, (u_int16)((u_int32)value >> 16),
                                   llHdl->offset[1], llHdl->gain[1] ) );
    }

    return(ERR_SUCCESS);
}

/****************************** Z51_SetStat *********************************/
/** Set the driver status
 *
 *  The driver supports \ref getstat_setstat_codes "these status codes"
 *  in addition to the standard codes (see mdis_api.h).
 *
 *  \param llHdl         \IN  low-level handle
 *  \param code          \IN  \ref getstat_setstat_codes "status code"
 *  \param ch            \IN  current channel
 *  \param value32_or_64 \IN  data or
 *                            pointer to block data structure (M_SG_BLOCK) for
 *                            block status codes
 *  \return           \c 0 on success or error code
 */
static int32 Z51_SetStat(
    LL_HANDLE *llHdl,
    int32  code,
    int32  ch,
    INT32_OR_64 value32_or_64
)
{
    MACCESS   ma = llHdl->ma;
    int32     error = ERR_SUCCESS;
    int32     value  = (int32)value32_or_64; /* 32bit value     */  

    DBGWRT_1((DBH, "LL - Z51_SetStat: ch=%d code=0x%04x value=0x%x\n",
              ch,code,value));

    /* SetStat is only allowed on channels 0 and 1 */
    if( !IN_RANGE( ch, 0, 1 ) )
        return( ERR_LL_ILL_CHAN );

    switch(code) {
        /*--------------------------+
        |  debug level              |
        +--------------------------*/
        case M_LL_DEBUG_LEVEL:
            llHdl->dbgLevel = value;
            break;

        /*--------------------------+
        |  enable interrupts        |
        +--------------------------*/
        case M_MK_IRQ_ENABLE:
            DBGWRT_2((DBH, " %sable irq\n", value ? "en":"dis"));

            if( value == 0 ) {
                MWRITE_D32( ma, DAC_IER_REG, 0 );
            }
            else {
                /* enable IRQ on next write access */
                llHdl->initDac = 1;
            }

            break;

        /*--------------------------+
        |  set irq counter          |
        +--------------------------*/
        case M_LL_IRQ_COUNT:
            llHdl->irqCount = value;
            break;
        /*--------------------------+
        |  channel direction        |
        +--------------------------*/
        case M_LL_CH_DIR:
            if( value != M_CH_OUT ) {
                error = ERR_LL_ILL_DIR;
            }

            break;

        /*--------------------------+
        |  DAC offset parameter     |
        +--------------------------*/
        case Z51_OFFSET:
            llHdl->offset[ch] = value;
            break;

        /*--------------------------+
        |  DAC gain parameter       |
        +--------------------------*/
        case Z51_GAIN:
            llHdl->gain[ch] = value;
            break;

        /*--------------------------+
        |  current powerdown mode   |
        +--------------------------*/
        case Z51_POWERDOWN:
        {
            u_int32 pdMode;

            /* select powerdown mode */
            switch( value ) {
                case 1:  pdMode = DAC_CMD_PD_1K;
                case 2:  pdMode = DAC_CMD_PD_100K;
                case 3:  pdMode = DAC_CMD_PD_HIGHZ;
                default: pdMode = 0;
            }

            /* dependant on channel turn off output A or B */
            switch( ch ) {
                case 0:
                    MWRITE_D32( ma, DAC_CTRL_REG,
                                DAC_CMD_LOAD_A | DAC_CMD_BUF_A | pdMode );

                case 1:
                    MWRITE_D32( ma, DAC_CTRL_REG,
                                DAC_CMD_LOAD_B | DAC_CMD_BUF_B | pdMode );
            }

            llHdl->powerdown[ch] = value;
        }
        break;

        /*--------------------------+
        |  register signal          |
        +--------------------------*/
        case Z51_SET_SIGNAL:

            /* signal already installed ? */
            if( llHdl->hwSig ) {
                error = ERR_OSS_SIG_SET;
                break;
            }

            error = OSS_SigCreate( OSH, value, &llHdl->hwSig );
            break;

        /*--------------------------+
        |  unregister signal        |
        +--------------------------*/
        case Z51_CLR_SIGNAL:

            /* signal already installed ? */
            if( llHdl->hwSig == NULL ) {
                error = ERR_OSS_SIG_CLR;
                break;
            }

            error = OSS_SigRemove( OSH, &llHdl->hwSig );
            break;

        /*--------------------------+
        |  (unknown)                |
        +--------------------------*/
        default:
            error = ERR_LL_UNK_CODE;
    }

    return(error);
}

/****************************** Z51_GetStat *********************************/
/** Get the driver status
 *
 *  The driver supports \ref getstat_setstat_codes "these status codes"
 *  in addition to the standard codes (see mdis_api.h).
 *
 *  \param llHdl          \IN  low-level handle
 *  \param code           \IN  \ref getstat_setstat_codes "status code"
 *  \param ch             \IN  current channel
 *  \param value32_or_64P \IN  pointer to block data structure (M_SG_BLOCK) for
 *                             block status codes
 *  \param value32_or_64P \OUT data pointer or pointer to block data structure
 *                             (M_SG_BLOCK) for block status codes
 *
 *  \return           \c 0 on success or error code
 */
static int32 Z51_GetStat(
    LL_HANDLE *llHdl,
    int32  code,
    int32  ch,
    INT32_OR_64 *value32_or_64P
)
{
    int32      error = ERR_SUCCESS;
    int32       *valueP = (int32*)value32_or_64P; /* pointer to 32bit value  */
    INT32_OR_64 *value64P = value32_or_64P;       /* stores 32/64bit pointer  */

    DBGWRT_1((DBH, "LL - Z51_GetStat: ch=%d code=0x%04x\n",
              ch,code));

    /* GetStat is only allowed on channels 0 and 1 */
    if( !IN_RANGE( ch, 0, 1 ) )
        return( ERR_LL_ILL_CHAN );

    switch(code)
    {
        /*--------------------------+
        |  debug level              |
        +--------------------------*/
        case M_LL_DEBUG_LEVEL:
            *valueP = llHdl->dbgLevel;
            break;
        /*--------------------------+
        |  number of channels       |
        +--------------------------*/
        case M_LL_CH_NUMBER:
            *valueP = CH_NUMBER;
            break;
        /*--------------------------+
        |  channel direction        |
        +--------------------------*/
        case M_LL_CH_DIR:
            *valueP = M_CH_OUT;
            break;
        /*--------------------------+
        |  channel length [bits]    |
        +--------------------------*/
        case M_LL_CH_LEN:
            *valueP = 32;
            break;
        /*--------------------------+
        |  channel type info        |
        +--------------------------*/
        case M_LL_CH_TYP:
            *valueP = M_CH_ANALOG;
            break;
        /*--------------------------+
        |  irq counter              |
        +--------------------------*/
        case M_LL_IRQ_COUNT:
            *valueP = llHdl->irqCount;
            break;
        /*--------------------------+
        |  ID PROM check enabled    |
        +--------------------------*/
        case M_LL_ID_CHECK:
            *valueP = 0;
            break;
        /*--------------------------+
        |   ident table pointer     |
        |   (treat as non-block!)   |
        +--------------------------*/
        case M_MK_BLK_REV_ID:
           *value64P = (INT32_OR_64)&llHdl->idFuncTbl;
           break;

        /*--------------------------+
        |  DAC offset parameter     |
        +--------------------------*/
        case Z51_OFFSET:
            *valueP = llHdl->offset[ch];
            break;

        /*--------------------------+
        |  DAC gain parameter       |
        +--------------------------*/
        case Z51_GAIN:
            *valueP = llHdl->gain[ch];
            break;

        /*--------------------------+
        |  current powerdown mode   |
        +--------------------------*/
        case Z51_POWERDOWN:
            *valueP = llHdl->powerdown[ch];
            break;

        /*--------------------------+
        |  (unknown)                |
        +--------------------------*/
        default:
            error = ERR_LL_UNK_CODE;
    }

    return(error);
}

/******************************* Z51_BlockRead ******************************/
/** Read a data block from the device
 *
 *  \param llHdl       \IN  low-level handle
 *  \param ch          \IN  current channel
 *  \param buf         \IN  data buffer
 *  \param size        \IN  data buffer size
 *  \param nbrRdBytesP \OUT number of read bytes
 *
 *  \return            \c 0 on success or error code
 */
static int32 Z51_BlockRead(
     LL_HANDLE *llHdl,
     int32     ch,
     void      *buf,
     int32     size,
     int32     *nbrRdBytesP
)
{
    DBGWRT_1((DBH, "LL - Z51_BlockRead: ch=%d, size=%d\n",ch,size));

    /* return number of read bytes */
    *nbrRdBytesP = 0;

    return(ERR_LL_ILL_FUNC);
}

/****************************** Z51_BlockWrite *****************************/
/** Write a data block from the device
 *
 *  \param llHdl       \IN  low-level handle
 *  \param ch          \IN  current channel
 *  \param buf         \IN  data buffer
 *  \param size        \IN  data buffer size
 *  \param nbrWrBytesP \OUT number of written bytes
 *
 *  \return            \c 0 on success or error code
 */
static int32 Z51_BlockWrite(
     LL_HANDLE *llHdl,
     int32     ch,
     void      *buf,
     int32     size,
     int32     *nbrWrBytesP
)
{
    DBGWRT_1((DBH, "LL - Z51_BlockWrite: ch=%d, size=%d\n",ch,size));

    /* return number of written bytes */
    *nbrWrBytesP = 0;

    return(ERR_LL_ILL_FUNC);
}


/****************************** Z51_Irq ************************************/
/** Interrupt service routine
 *
 *  The interrupt is triggered when the FPGA detects a hardware malfunction.
 *
 *  If the driver can detect the interrupt's cause it returns
 *  LL_IRQ_DEVICE or LL_IRQ_DEV_NOT, otherwise LL_IRQ_UNKNOWN.
 *
 *  \param llHdl       \IN  low-level handle
 *  \return LL_IRQ_DEVICE   irq caused by device
 *          LL_IRQ_DEV_NOT  irq not caused by device
 *          LL_IRQ_UNKNOWN  unknown
 */
static int32 Z51_Irq(
   LL_HANDLE *llHdl
)
{
    MACCESS ma = llHdl->ma;
    u_int32 irqReg = MREAD_D32( ma, DAC_IRQ_REG );

    /* interrupt came from my device ? */
    if( (irqReg & DAC_IRQ_MASK) != DAC_IRQ_MASK )
        return( LL_IRQ_DEV_NOT );

    IDBGWRT_1((DBH, ">>> Z51_Irq:\n"));

    /* 
     * The interrupt must be disabled here, because on hardware malfunction
     * the watchdog circuit will never release the IRQ line.
     */
    MWRITE_D32( ma, DAC_IER_REG, 0 );

    /* clear interrupt */
    MWRITE_D32( ma, DAC_IRQ_REG, DAC_IRQ_MASK );

    /* try to enable on next write */
    llHdl->initDac = 1;

    /* if requested send signal to application */
    if( llHdl->hwSig ) {
        OSS_SigSend( OSH, llHdl->hwSig );
    }

    llHdl->irqCount++;

    return( LL_IRQ_DEVICE );
}

/****************************** Z51_Info ***********************************/
/** Get information about hardware and driver requirements
 *
 *  The following info codes are supported:
 *
 * \code
 *  Code                      Description
 *  ------------------------  -----------------------------
 *  LL_INFO_HW_CHARACTER      hardware characteristics
 *  LL_INFO_ADDRSPACE_COUNT   nr of required address spaces
 *  LL_INFO_ADDRSPACE         address space information
 *  LL_INFO_IRQ               interrupt required
 *  LL_INFO_LOCKMODE          process lock mode required
 * \endcode
 *
 *  The LL_INFO_HW_CHARACTER code returns all address and
 *  data modes (ORed) which are supported by the hardware
 *  (MDIS_MAxx, MDIS_MDxx).
 *
 *  The LL_INFO_ADDRSPACE_COUNT code returns the number
 *  of address spaces used by the driver.
 *
 *  The LL_INFO_ADDRSPACE code returns information about one
 *  specific address space (MDIS_MAxx, MDIS_MDxx). The returned
 *  data mode represents the widest hardware access used by
 *  the driver.
 *
 *  The LL_INFO_IRQ code returns whether the driver supports an
 *  interrupt routine (TRUE or FALSE).
 *
 *  The LL_INFO_LOCKMODE code returns which process locking
 *  mode the driver needs (LL_LOCK_xxx).
 *
 *  \param infoType    \IN  info code
 *  \param ...         \IN  argument(s)
 *
 *  \return            \c 0 on success or error code
 */
static int32 Z51_Info(
   int32  infoType,
   ...
)
{
    int32   error = ERR_SUCCESS;
    va_list argptr;

    va_start(argptr, infoType );

    switch(infoType) {
        /*-------------------------------+
        |  hardware characteristics      |
        |  (all addr/data modes ORed)   |
        +-------------------------------*/
        case LL_INFO_HW_CHARACTER:
        {
            u_int32 *addrModeP = va_arg(argptr, u_int32*);
            u_int32 *dataModeP = va_arg(argptr, u_int32*);

            *addrModeP = MDIS_MA08;
            *dataModeP = MDIS_MD08 | MDIS_MD16;
            break;
        }
        /*-------------------------------+
        |  nr of required address spaces |
        |  (total spaces used)           |
        +-------------------------------*/
        case LL_INFO_ADDRSPACE_COUNT:
        {
            u_int32 *nbrOfAddrSpaceP = va_arg(argptr, u_int32*);

            *nbrOfAddrSpaceP = ADDRSPACE_COUNT;
            break;
        }
        /*-------------------------------+
        |  address space type            |
        |  (widest used data mode)       |
        +-------------------------------*/
        case LL_INFO_ADDRSPACE:
        {
            u_int32 addrSpaceIndex = va_arg(argptr, u_int32);
            u_int32 *addrModeP = va_arg(argptr, u_int32*);
            u_int32 *dataModeP = va_arg(argptr, u_int32*);
            u_int32 *addrSizeP = va_arg(argptr, u_int32*);

            if (addrSpaceIndex >= ADDRSPACE_COUNT)
                error = ERR_LL_ILL_PARAM;
            else {
                *addrModeP = MDIS_MA08;
                *dataModeP = MDIS_MD32;
                *addrSizeP = ADDRSPACE_SIZE;
            }

            break;
        }
        /*-------------------------------+
        |   interrupt required           |
        +-------------------------------*/
        case LL_INFO_IRQ:
        {
            u_int32 *useIrqP = va_arg(argptr, u_int32*);

            *useIrqP = USE_IRQ;
            break;
        }
        /*-------------------------------+
        |   process lock mode            |
        +-------------------------------*/
        case LL_INFO_LOCKMODE:
        {
            u_int32 *lockModeP = va_arg(argptr, u_int32*);

            *lockModeP = LL_LOCK_CALL;
            break;
        }
        /*-------------------------------+
        |   (unknown)                    |
        +-------------------------------*/
        default:
          error = ERR_LL_ILL_PARAM;
    }

    va_end(argptr);
    return(error);
}

/*******************************  Ident  ***********************************/
/** Return ident string
 *
 *  \return            pointer to ident string
 */
static char* Ident( void )
{
    return( "Z51 - Z51 low level driver: "
#ifdef MAC_BYTESWAP
    "swapped "
#else
    "non swapped "
#endif
"$Id: z51_drv.c,v 1.4 2017/04/28 11:32:57 DPfeuffer Exp $" );
}

/********************************* Cleanup *********************************/
/** Close all handles, free memory and return error code
 *
 *  \warning The low-level handle is invalid after this function is called.
 *
 *  \param llHdl      \IN  low-level handle
 *  \param retCode    \IN  return value
 *
 *  \return           \IN   retCode
 */
static int32 Cleanup(
   LL_HANDLE    *llHdl,
   int32        retCode
)
{
    /*------------------------------+
    |  close handles                |
    +------------------------------*/
    /* clean up desc */
    if (llHdl->descHdl)
        DESC_Exit(&llHdl->descHdl);

    /* clean up debug */
    DBGEXIT((&DBH));

    /*------------------------------+
    |  free memory                  |
    +------------------------------*/
    /* free my handle */
    OSS_MemFree(llHdl->osHdl, (int8*)llHdl, llHdl->memAlloc);

    /*------------------------------+
    |  return error code            |
    +------------------------------*/
    return(retCode);
}


/**********************************************************************/
/** Correct output value according to calibration values
 *
 *  The output value is computed using the formula
 *    new = offset + value * gain / 0xffff
 *
 *  If gain is zero then no correction is done and the input value 
 *  is returned unchanged.
 *
 *  \param value      \IN  input value
 *  \param offset     \IN  offset
 *  \param gain       \IN  gain
 *
 *  \return corrected value
 */
static u_int16 calibrate( 
    LL_HANDLE *llHdl,
    u_int16   value,
    int       offset,
    int       gain )
{
    u_int16 tmp;

    if( gain == 0 )
        return( value );

    tmp =  (u_int16)(offset + (value * gain) / 0xffff);
    DBGWRT_3((DBH, "%d -> %d (o=0x%x, g=0x%x)\n", 
              value, tmp, offset, gain ));

    return( tmp );
}

