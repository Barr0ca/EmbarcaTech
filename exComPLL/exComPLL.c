#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/clocks.h"
#include "hardware/pll.h"

int main()
{
    stdio_init_all();
    clocks_init();

    pll_init(pll_sys, 1, 1500*MHZ, 6*MHZ,1);

    clock_configure(clk_sys, 
    CLOCKS_CLK_SYS_CTRL_SRC_VALUE_CLKSRC_CLK_SYS_AUX, 
    CLOCKS_CLK_SYS_CTRL_AUXSRC_VALUE_CLKSRC_PLL_SYS, 
    125*MHZ, 125*MHZ);

    printf("CLK_SYS está operando a 125 MHz\n");

    while (true) 
    {

    }
}
