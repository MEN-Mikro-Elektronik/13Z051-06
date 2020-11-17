/*********************  P r o g r a m  -  M o d u l e ***********************/
/*!
 *        \file  z51_doc.c
 *
 *      \author  ub
 *
 *      \brief   User documentation for Z51 DAC "Edmonton" driver
 *
 *     Required: -
 *
 *     \switches -
 */
 /*
 *---------------------------------------------------------------------------
 * Copyright 2004-2020, MEN Mikro Elektronik GmbH
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

/*! \mainpage
    This is the documentation of the MDIS5 low-level driver for the DAC8532
    digital-to-analog-converter connected to the system via the 16Z051
    "Edmonton" interface implemented in FPGA.

    \section Variants Variants
    In order to support different CPU/carrier board combinations and to
    achieve maximum performance, the Z51 driver can be built in some variants
    at compilation time:

    \code
    Driver              Variant Description
    --------            --------------------------------
    Standard            D32 register access, non-swapped
    _sw                 D32 register access, swapped
    \endcode

    Here are some combinations of MEN CPU and carrier boards together with the
    required variants:

    \code
    CPU                    Carrier Board          Driver Variant
    ----------------       -------------          --------------
    MEN EM04 (PPC)         (none)                 _sw
    MEN F7 (Pentium CPCI)   F401                  Standard
    \endcode

    \n \section FuncDesc Functional Description

    \n \subsection General General

    The DAC8532 features two independant channels with a resolution of 16 bits
    each.  The output value of the channels can be set either sequencially or
    simultaneously both at-once. The DAC8532 has a built-in SPI interface,
    which is handled by the 16Z051 FPGA unit. The driver uses its registers to
    send commands and data to the DAC.


    \n \subsection channels Logical channels

    The driver supports three logical channels:
    - Channel 0: DAC channel A
    - Channel 1: DAC channel B
    - Channel 2: DAC channels A and B

    Note: M_setstat() and M_getstat() can only be used for channel 0 or 1.

    Using M_write() a 16-bit value can be written on channels 0 and 1 which
    determines the voltage/current on DAC channel A or B respectively.
    M_write() on channel 2 expects a 32-bit argument which is composed of the
    16-bit values that will be written to channels A and B:

    \code
    M_write( (ch_b_value << 16) | ch_a_value );
    \endcode

    Writing on channel 2 causes both DAC channels to be updated simultaneously.


    \n \subsection calibration Calibration
    Calibration of the DACs is done by default values for gain and offset
    which are compiled into the driver. If necessary these can be overwritten
    using the descriptor keys Z51_GAIN/Z51_OFFSET. Using SetStat calls
    Z51_GAIN and Z51_OFFSET the driver can be forced to use the given values
    for calibration. The calibration itself is calculated using this formula:

    \code
    y = Z51_OFFSET + x * Z51_GAIN / 0xFFFF
    \endcode

    where y is the value written into the DAC's input register; x is the value
    set by M_write(). Z51_OFFSET is the value that must be written to the DAC
    to get the minimum intended voltage/current at the output, Z51_GAIN is the
    DAC's value for the maximum voltage/current.

    \n If Z51_GAIN is zero, then no calibration is done. In this case the
    values given by M_write() are written to the DAC without change.


    \n \subsection interrupt Interrupt handling

    In case of hardware malfunction the FPGA can generate an interrupt. The
    driver catches this interrupt and sends an user defineable signal to the
    application. Interrupt generation and handling can be enabled using
    descriptor key Z51_INTERRUPT. By default it is disabled.

    \n The signal must be activated via the Z51_SIG_SET SetStat code and can
    be cleared through SetStat Z51_SIG_CLR.


    \n \subsection powerdown Power Down

    The driver allows to set each of the DAC's channels into one of four
    supported power-down modes. This is done using SetStat Z51_POWERDOWN on
    the respective logical channel (0 or 1).

    \warning On Power Down the outputs of the DAC8532 are set High-Z or tied
    to ground. If this feature is useful or even applicable depends on the
    circuity at the DAC's output on your board. <b> Use with care !</b>

    \n Power Down modes:
    - 0: Output active
    - 1: Output High-Z
    - 2: Output tied to ground by internal 1kOhm resistor
    - 3: Output tied to ground by internal 100kOhm resistor

    The current power-down mode can be obtained using SetStat Z51_POWERDOWN.


    \n \subsection watchdog Watchdog

    The F401 board contains a watchdog circuit which supervises the DAC. If no
    SPI communication is running between the FPGA and the DAC 8532 then the
    DAC's output is disconnected from the board's output drivers. In this
    driver this mechanism is used to ensure an output curent of 0 mA when the
    driver is not initialized. \n

    The communication is established on the first M_write() access and turned
    off when the last instance of the driver is closed. \n

    The watchdog generates also an interrupt if no communication is detected.
    Therefore when the first access is done and IRQs are enabled the driver
    waits about 1000ms for the watchdog circuit to release the IRQ input
    before it enables the interrupt. This avoids an unwanted interrupt which
    else would happen on first access.  If the driver's interrupt is used
    (descriptor key IRQ_ENABLE) this causes a delay on the first M_write().

    If the interrupts triggers, which happens in case of a hardware
    malfunction, a signal is optionally sended to the application and the
    interrupt is disabled until the next M_write() call.

    \n \subsection locking Locking Mode
    This driver uses call-locking.

    \n \section api_functions Supported API Functions

    <table border="0">
    <tr>
        <td><b>API function</b></td>
        <td><b>Functionality</b></td>
        <td><b>Corresponding low level function</b></td></tr>

    <tr><td>M_open()</td><td>Open device</td><td>Z51_Init()</td></tr>

    <tr><td>M_close()     </td><td>Close device             </td>
    <td>Z51_Exit())</td></tr>
    <tr><td>M_read()      </td><td>Read from device         </td>
    <td>Z51_Read()</td></tr>
    <tr><td>M_write()     </td><td>Write to device          </td>
    <td>Z51_Write()</td></tr>
    <tr><td>M_setstat()   </td><td>Set device parameter     </td>
    <td>Z51_SetStat()</td></tr>
    <tr><td>M_getstat()   </td><td>Get device parameter     </td>
    <td>Z51_GetStat()</td></tr>
    <tr><td>M_getblock()  </td><td>Block read from device   </td>
    <td>Z51_BlockRead()</td></tr>
    <tr><td>M_setblock()  </td><td>Block write from device  </td>
    <td>Z51_BlockWrite()</td></tr>
    <tr><td>M_errstringTs() </td><td>Generate error message </td>
    <td>-</td></tr>
    </table>

    \n \section descriptor_entries Descriptor Entries

    The low-level driver initialization routine decodes the following entries
    ("keys") in addition to the general descriptor keys:

    <table border="0">
    <tr><td><b>Descriptor entry</b></td>
        <td><b>Description</b></td>
        <td><b>Values</b></td>
    </tr>

    <tr><td>Z51_INTERRUPT</td>
        <td>Interrupt active</td>
        <td>0..1, default: 0</td>
    </tr>
    <tr><td>Z51_OFFSET_0</td>
        <td>Offset value for calibration</td>
        <td>0..0xffff, default: 0x1985</td>
    </tr>
    <tr><td>Z51_GAIN_0</td>
        <td>Gain value for calibration</td>
        <td>0..0xffff, default: 0xCE3E</td>
    </tr>
    <tr><td>Z51_OFFSET_1</td>
        <td>Offset value for calibration</td>
        <td>0..0xffff, default: 0x1951</td>
    </tr>
    <tr><td>Z51_GAIN_1</td>
        <td>Gain value for calibration</td>
        <td>0..0xffff, default: 0xCDD3</td>
    </tr>
    </table>


    \n \section codes Z51 specific Getstat/Setstat codes
    see \ref getstat_setstat_codes "section about Getstat/Setstat codes"

    \n \section Documents Overview of all Documents

    \subsection z51_simp  Simple example for using the driver
    z51_simp.c (see example section)
*/

/** \example tmpl_simp.c
Simple example for driver usage
*/

/*! \page z51dummy MEN logo
  \menimages
*/

