#***************************  M a k e f i l e  *******************************
#
#         Author: ub
#          $Date: 2004/11/29 09:03:07 $
#      $Revision: 1.1 $
#
#    Description: Makefile definitions for the Z51 example program
#
#-----------------------------------------------------------------------------
#   Copyright (c) 2000-2019, MEN Mikro Elektronik GmbH
#*****************************************************************************

MAK_NAME=z51_simp

MAK_LIBS=$(LIB_PREFIX)$(MEN_LIB_DIR)/mdis_api$(LIB_SUFFIX)	\
		 $(LIB_PREFIX)$(MEN_LIB_DIR)/usr_oss$(LIB_SUFFIX)	\

MAK_INCL=$(MEN_INC_DIR)/z51_drv.h	\
         $(MEN_INC_DIR)/men_typs.h	\
         $(MEN_INC_DIR)/mdis_api.h	\
         $(MEN_INC_DIR)/usr_oss.h	\

MAK_INP1=z51_simp$(INP_SUFFIX)

MAK_INP=$(MAK_INP1)
