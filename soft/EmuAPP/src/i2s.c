#include "i2s.h"
#include "slc_register.h"
#include "tv.h"

volatile struct sdio_queue i2sBufDesc [I2S_N_BUFS];

void i2s_int (void)
{
    // Get INT status
    uint32_t slc_intr_status = READ_PERI_REG (SLC_INT_STATUS);
    
    // Clear all intr flags
    WRITE_PERI_REG (SLC_INT_CLR, 0xffffffff);//slc_intr_status);
    
    if (slc_intr_status & SLC_RX_EOF_INT_ST)
    {
        // Есть буфер для заполнения
        struct sdio_queue *desc = (struct sdio_queue*) READ_PERI_REG (SLC_RX_EOF_DES_ADDR);
        desc->buf_ptr = (uint32_t) tv_i2s_cb ();
    }
}
