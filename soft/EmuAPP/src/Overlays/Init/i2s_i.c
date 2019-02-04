#include "i2s_i.h"
#include "tv.h"

#include "slc_register.h"
#include "pin_mux_register.h"

#define AT_OVL __attribute__((section(".ovl3_i.text")))

#ifndef i2c_bbpll
    #define i2c_bbpll                           0x67
    #define i2c_bbpll_en_audio_clock_out        4
    #define i2c_bbpll_en_audio_clock_out_msb    7
    #define i2c_bbpll_en_audio_clock_out_lsb    7
    #define i2c_bbpll_hostid                    4
    
    #define i2c_writeReg_Mask(     block, host_id, reg_add, Msb, Lsb, indata)  rom_i2c_writeReg_Mask (block, host_id,        reg_add, Msb,           Lsb,           indata)
    #define i2c_readReg_Mask(      block, host_id, reg_add, Msb, Lsb)          rom_i2c_readReg_Mask  (block, host_id,        reg_add, Msb,           Lsb)
    #define i2c_writeReg_Mask_def( block,          reg_add,           indata)      i2c_writeReg_Mask (block, block##_hostid, reg_add, reg_add##_msb, reg_add##_lsb, indata)
    #define i2c_readReg_Mask_def(  block,          reg_add)                        i2c_readReg_Mask  (block, block##_hostid, reg_add, reg_add##_msb, reg_add##_lsb)
#endif

//Initialize I2S subsystem for DMA circular buffer use
void AT_OVL i2s_init (int size)
{
    //Reset DMA
    SET_PERI_REG_MASK   (SLC_CONF0, SLC_RXLINK_RST);//|SLC_TXLINK_RST);
    CLEAR_PERI_REG_MASK (SLC_CONF0, SLC_RXLINK_RST);//|SLC_TXLINK_RST);
    
    //Clear DMA int flags
    SET_PERI_REG_MASK   (SLC_INT_CLR,  0xffffffff);
    CLEAR_PERI_REG_MASK (SLC_INT_CLR,  0xffffffff);
    
    //Enable and configure DMA
    CLEAR_PERI_REG_MASK (SLC_CONF0, (SLC_MODE << SLC_MODE_S));
    SET_PERI_REG_MASK   (SLC_CONF0, (1 << SLC_MODE_S));
    SET_PERI_REG_MASK   (SLC_RX_DSCR_CONF, SLC_INFOR_NO_REPLACE | SLC_TOKEN_NO_REPLACE);
    CLEAR_PERI_REG_MASK (SLC_RX_DSCR_CONF, SLC_RX_FILL_EN | SLC_RX_EOF_MODE | SLC_RX_FILL_MODE);
    
    // Buffers in loop
    int i;
    for (i = 0; i < I2S_N_BUFS; i++)
    {
        i2sBufDesc [i].owner         = 1;
        i2sBufDesc [i].eof           = 1;
        i2sBufDesc [i].sub_sof       = 0;
        i2sBufDesc [i].datalen       = size;   //Size (in bytes)
        i2sBufDesc [i].blocksize     = size; //Size (in bytes)
        i2sBufDesc [i].buf_ptr       = (uint32_t) tv_i2s_cb ();
        i2sBufDesc [i].unused        = 0;
        i2sBufDesc [i].next_link_ptr = (uint32_t) &i2sBufDesc [(i + 1) & (I2S_N_BUFS - 1)];
    }
    
    
    //CLEAR_PERI_REG_MASK(SLC_TX_LINK,SLC_TXLINK_DESCADDR_MASK);
    //SET_PERI_REG_MASK(SLC_TX_LINK, ((uint32_t)&i2sBufDescZeroes) & SLC_TXLINK_DESCADDR_MASK); //any random desc is OK, we don't use TX but it needs something valid
    
    CLEAR_PERI_REG_MASK (SLC_RX_LINK, SLC_RXLINK_DESCADDR_MASK);
    SET_PERI_REG_MASK   (SLC_RX_LINK, ((uint32_t) &i2sBufDesc [0]) & SLC_RXLINK_DESCADDR_MASK);
    
    //Attach the DMA interrupt
//  ets_isr_attach(ETS_SLC_INUM, i2s_int, 0);
    //Enable DMA operation intr
    WRITE_PERI_REG (SLC_INT_ENA,  SLC_RX_EOF_INT_ENA);
    //clear any interrupt flags that are set
    WRITE_PERI_REG (SLC_INT_CLR, 0xffffffff);
    ///enable DMA intr in cpu
    ets_isr_unmask (1 << ETS_SLC_INUM);

    //Start transmission
    //SET_PERI_REG_MASK(SLC_TX_LINK, SLC_TXLINK_START);
    SET_PERI_REG_MASK (SLC_RX_LINK, SLC_RXLINK_START);
    
    //Init pins to i2s functions
    PIN_FUNC_SELECT (PERIPHS_IO_MUX_U0RXD_U, FUNC_I2SO_DATA);
    //PIN_FUNC_SELECT (PERIPHS_IO_MUX_GPIO2_U, FUNC_I2SO_WS);
    //PIN_FUNC_SELECT (PERIPHS_IO_MUX_MTDO_U, FUNC_I2SO_BCK);
    
    //Enable clock to i2s subsystem
    i2c_writeReg_Mask_def (i2c_bbpll, i2c_bbpll_en_audio_clock_out, 1);
    
    //Reset I2S subsystem
    CLEAR_PERI_REG_MASK (I2SCONF,I2S_I2S_RESET_MASK);
    SET_PERI_REG_MASK   (I2SCONF,I2S_I2S_RESET_MASK);
    CLEAR_PERI_REG_MASK (I2SCONF,I2S_I2S_RESET_MASK);
    
    //Select 16bits per channel (FIFO_MOD=0), no DMA access (FIFO only)
    CLEAR_PERI_REG_MASK (I2S_FIFO_CONF, I2S_I2S_DSCR_EN | (I2S_I2S_RX_FIFO_MOD << I2S_I2S_RX_FIFO_MOD_S) | (I2S_I2S_TX_FIFO_MOD << I2S_I2S_TX_FIFO_MOD_S));
    
    //Enable DMA in i2s subsystem
    SET_PERI_REG_MASK (I2S_FIFO_CONF, I2S_I2S_DSCR_EN);
    
    //tx/rx binaureal
    //CLEAR_PERI_REG_MASK(I2SCONF_CHAN, (I2S_TX_CHAN_MOD<<I2S_TX_CHAN_MOD_S)|(I2S_RX_CHAN_MOD<<I2S_RX_CHAN_MOD_S));
    
    //Clear int
    SET_PERI_REG_MASK   (I2SINT_CLR, I2S_I2S_RX_WFULL_INT_CLR | I2S_I2S_PUT_DATA_INT_CLR | I2S_I2S_TAKE_DATA_INT_CLR);
    CLEAR_PERI_REG_MASK (I2SINT_CLR, I2S_I2S_RX_WFULL_INT_CLR | I2S_I2S_PUT_DATA_INT_CLR | I2S_I2S_TAKE_DATA_INT_CLR);
    
    
    //trans master&rece slave,MSB shift,right_first,msb right
    CLEAR_PERI_REG_MASK (I2SCONF, I2S_TRANS_SLAVE_MOD | (I2S_BITS_MOD << I2S_BITS_MOD_S) | (I2S_BCK_DIV_NUM << I2S_BCK_DIV_NUM_S) | (I2S_CLKM_DIV_NUM << I2S_CLKM_DIV_NUM_S));
    SET_PERI_REG_MASK   (I2SCONF, I2S_RIGHT_FIRST | I2S_MSB_RIGHT | I2S_RECE_SLAVE_MOD | I2S_RECE_MSB_SHIFT | I2S_TRANS_MSB_SHIFT |
                        (((WS_I2S_BCK) & I2S_BCK_DIV_NUM ) << I2S_BCK_DIV_NUM_S) |
                        (((WS_I2S_DIV) & I2S_CLKM_DIV_NUM) << I2S_CLKM_DIV_NUM_S));
    
    
    //No idea if ints are needed...
    //clear int
    SET_PERI_REG_MASK   (I2SINT_CLR, I2S_I2S_RX_WFULL_INT_CLR | I2S_I2S_PUT_DATA_INT_CLR | I2S_I2S_TAKE_DATA_INT_CLR);
    CLEAR_PERI_REG_MASK (I2SINT_CLR, I2S_I2S_RX_WFULL_INT_CLR | I2S_I2S_PUT_DATA_INT_CLR | I2S_I2S_TAKE_DATA_INT_CLR);
    //enable int
    SET_PERI_REG_MASK (I2SINT_ENA, I2S_I2S_RX_REMPTY_INT_ENA | I2S_I2S_RX_TAKE_DATA_INT_ENA);
}

void AT_OVL i2s_start (void)
{
    //Start transmission
    SET_PERI_REG_MASK (I2SCONF,I2S_I2S_TX_START);
}
