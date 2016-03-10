/*
 * Author: Merlin Webster, SmartRF Studio
 * Description: Configuration register settings for the transceivers, as exported from SmartRF Studio
 */

// Carrier frequency = 145.000000 
// Whitening = false 
// Manchester enable = false 
// Device address = 0 
// Bit rate = 1 
// Packet length mode = Variable 
// Deviation = 0.495911 
// Address config = No address check 
// RX filter BW = 15.625000 
// Modulation format = 2-FSK 
// Performance mode = High Performance 
// Symbol rate = 1 
// PA ramping = false 
// TX power = 5 
// Packet bit length = 0 
// Packet length = 255 

#ifndef CC112X_REG_H_
#define CC112X_REG_H_

const uint16_t cc112x_regSettings[][2] = 
{
    {CC112X_IOCFG3,                0xB0},        //GPIO3 IO Pin Configuration
    {CC112X_IOCFG2,                0x06},        //GPIO2 IO Pin Configuration
    {CC112X_IOCFG1,                0xB0},        //GPIO1 IO Pin Configuration
    {CC112X_IOCFG0,                0x40},        //GPIO0 IO Pin Configuration
    {CC112X_SYNC_CFG1,             0x08},        //Sync Word Detection Configuration Reg. 1
    {CC112X_DEVIATION_M,           0x68},        //Frequency Deviation Configuration
    {CC112X_MODCFG_DEV_E,          0x00},        //Modulation Format and Frequency Deviation Configur..
    {CC112X_DCFILT_CFG,            0x1C},        //Digital DC Removal Configuration
    {CC112X_FREQ_IF_CFG,           0x33},        //RX Mixer Frequency Configuration
    {CC112X_IQIC,                  0xC6},        //Digital Image Channel Compensation Configuration
    {CC112X_CHAN_BW,               0x10},        //Channel Filter Configuration
    {CC112X_MDMCFG0,               0x05},        //General Modem Parameter Configuration Reg. 0
    {CC112X_SYMBOL_RATE2,          0x3A},        //Symbol Rate Configuration Exponent and Mantissa [1..
    {CC112X_SYMBOL_RATE1,          0x36},        //Symbol Rate Configuration Mantissa [15:8]
    {CC112X_SYMBOL_RATE0,          0xE3},        //Symbol Rate Configuration Mantissa [7:0]
    {CC112X_AGC_REF,               0x20},        //AGC Reference Level Configuration
    {CC112X_AGC_CS_THR,            0x0C},        //Carrier Sense Threshold Configuration
    {CC112X_AGC_CFG1,              0xA0},        //Automatic Gain Control Configuration Reg. 1
    {CC112X_FIFO_CFG,              0x00},        //FIFO Configuration
    {CC112X_SETTLING_CFG,          0x03},        //Frequency Synthesizer Calibration and Settling Con..
    {CC112X_FS_CFG,                0x1B},        //Frequency Synthesizer Configuration
    {CC112X_WOR_CFG0,              0x20},        //eWOR Configuration Reg. 0
    {CC112X_WOR_EVENT0_MSB,        0x03},        //Event 0 Configuration MSB
    {CC112X_WOR_EVENT0_LSB,        0x7F},        //Event 0 Configuration LSB
    {CC112X_PKT_CFG0,              0x20},        //Packet Configuration Reg. 0
    {CC112X_RFEND_CFG0,            0x09},        //RFEND Configuration Reg. 0
    {CC112X_PA_CFG2,               0x29},        //Power Amplifier Configuration Reg. 2
    {CC112X_PA_CFG0,               0x7E},        //Power Amplifier Configuration Reg. 0
    {CC112X_PKT_LEN,               0xFF},        //Packet Length Configuration
    {CC112X_IF_MIX_CFG,            0x00},        //IF Mix Configuration
    {CC112X_FREQOFF_CFG,           0x22},        //Frequency Offset Correction Configuration
    {CC112X_FREQ2,                 0x57},        //Frequency Configuration [23:16]
    {CC112X_IF_ADC0,               0x05},        //Analog to Digital Converter Configuration Reg. 0
    {CC112X_FS_DIG1,               0x00},        //Frequency Synthesizer Digital Reg. 1
    {CC112X_FS_DIG0,               0x5F},        //Frequency Synthesizer Digital Reg. 0
    {CC112X_FS_CAL0,               0x0E},        //Frequency Synthesizer Calibration Reg. 0
    {CC112X_FS_DIVTWO,             0x03},        //Frequency Synthesizer Divide by 2
    {CC112X_FS_DSM0,               0x33},        //FS Digital Synthesizer Module Configuration Reg. 0
    {CC112X_FS_DVC0,               0x17},        //Frequency Synthesizer Divider Chain Configuration ..
    {CC112X_FS_PFD,                0x50},        //Frequency Synthesizer Phase Frequency Detector Con..
    {CC112X_FS_PRE,                0x6E},        //Frequency Synthesizer Prescaler Configuration
    {CC112X_FS_REG_DIV_CML,        0x14},        //Frequency Synthesizer Divider Regulator Configurat..
    {CC112X_FS_SPARE,              0xAC},        //Frequency Synthesizer Spare
    {CC112X_XOSC5,                 0x0E},        //Crystal Oscillator Configuration Reg. 5
    {CC112X_XOSC3,                 0xC7},        //Crystal Oscillator Configuration Reg. 3
    {CC112X_XOSC1,                 0x07}         //Crystal Oscillator Configuration Reg. 1
};

#endif /* CC112X_REG_H_ */
