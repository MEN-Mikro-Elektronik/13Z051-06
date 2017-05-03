#************************** MDIS5 device descriptor *************************
#
#        Author: ub
#         $Date: 2017/04/28 11:33:11 $
#     $Revision: 1.2 $
#
#   Description: Metadescriptor for Z51 DAC (swapped access)
#
#****************************************************************************

Z51_SW_1  {
	#------------------------------------------------------------------------
	#	general parameters (don't modify)
	#------------------------------------------------------------------------
    DESC_TYPE        = U_INT32  1           # descriptor type (1=device)
    HW_TYPE          = STRING   Z51_SW      # hardware name of device

	#------------------------------------------------------------------------
	#	reference to base board
	#------------------------------------------------------------------------
    BOARD_NAME       = STRING   CHAMF401    # device name of baseboard
    DEVICE_SLOT      = U_INT32  0           # used slot on baseboard (0..n)

	#------------------------------------------------------------------------
	#	device parameters
	#------------------------------------------------------------------------
}
