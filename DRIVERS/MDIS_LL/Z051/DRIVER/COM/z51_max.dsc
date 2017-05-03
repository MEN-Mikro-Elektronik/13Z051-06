#************************** MDIS5 device descriptor *************************
#
#        Author: ub
#         $Date: 2017/04/28 11:33:04 $
#     $Revision: 1.3 $
#
#   Description: Metadescriptor for Z51 DAC
#
#****************************************************************************

Z51_1  {
	#------------------------------------------------------------------------
	#	general parameters (don't modify)
	#------------------------------------------------------------------------
    DESC_TYPE        = U_INT32  1           # descriptor type (1=device)
    HW_TYPE          = STRING   Z51         # hardware name of device

	#------------------------------------------------------------------------
	#	reference to base board
	#------------------------------------------------------------------------
    BOARD_NAME       = STRING   CHAMF401    # device name of baseboard
    DEVICE_SLOT      = U_INT32  0           # used slot on baseboard (0..n)

	#------------------------------------------------------------------------
	#	debug levels (optional)
	#   this keys have only effect on debug drivers
	#------------------------------------------------------------------------
    DEBUG_LEVEL      = U_INT32  0xc0008000  # LL driver
    DEBUG_LEVEL_MK   = U_INT32  0xc0008000  # MDIS kernel
    DEBUG_LEVEL_OSS  = U_INT32  0xc0008000  # OSS calls
    DEBUG_LEVEL_DESC = U_INT32  0xc0008000  # DESC calls
    DEBUG_LEVEL_MBUF = U_INT32  0xc0008000  # MBUF calls

	#------------------------------------------------------------------------
	#	device parameters
	#------------------------------------------------------------------------
    IRQ_ENABLE       = U_INT32  0           # irq enabled after init
    Z51_OFFSET_0     = U_INT32  0x1985      # offset parameter ch. 0
    Z51_GAIN_0       = U_INT32  0xce3e      # gain parameter ch. 0
    Z51_OFFSET_1     = U_INT32  0x1951      # offset parameter ch. 1
    Z51_GAIN_1       = U-INT32  0xcdd3      # gain parameter ch. 1
}
