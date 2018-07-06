#include <iostream>
#include <stdlib.h>
#include <stdio.h> 
#include <sys/mman.h>
#include <unistd.h>
#include <fcntl.h>
#include "jz4740.h"
#include "cpu.h"

using namespace std;

inline int sdram_convert(unsigned int pllin,unsigned int *sdram_freq)
{
	register unsigned int ns, tmp;
 
	ns = 1000000000 / pllin;

	/* Set refresh registers */
	tmp = SDRAM_TREF/ns;
	tmp = tmp/64 + 1;
	if(tmp > 0xff)
		tmp = 0xff;
	*sdram_freq = tmp; 
	return 0;
}
 
void pll_init(unsigned int clock)
{
	register unsigned int cfcr, plcr1;
	unsigned int sdramclock = 0;
	int n2FR[33] = {
		0, 0, 1, 2, 3, 0, 4, 0, 5, 0, 0, 0, 6, 0, 0, 0,
		7, 0, 0, 0, 0, 0, 0, 0, 8, 0, 0, 0, 0, 0, 0, 0,
		9
	};
	//int div[5] = {1, 4, 4, 4, 4}; /* divisors of I:S:P:L:M */
  int div[5] = {1, 3, 3, 3, 3}; /* divisors of I:S:P:L:M */
	int nf, pllout2;

	cout << "pll_init ++" << endl;
	cfcr = CPM_CPCCR_CLKOEN | (n2FR[div[0]] << CPM_CPCCR_CDIV_BIT) | (n2FR[div[1]] << CPM_CPCCR_HDIV_BIT) | 
		(n2FR[div[2]] << CPM_CPCCR_PDIV_BIT) | (n2FR[div[3]] << CPM_CPCCR_MDIV_BIT) | (n2FR[div[4]] << CPM_CPCCR_LDIV_BIT);
	pllout2 = (cfcr & CPM_CPCCR_PCS) ? clock : (clock / 2);

	/* Init UHC clock */
	//REG_CPM_UHCCDR = pllout2 / 48000000 - 1;
	cout << "new clock: " << clock << endl;
	cout << "set jz_cpmregl 0x6c" << endl;
  //jz_cpmregl[0x6C >> 2] = (pllout2 / 48000000) - 1;
  jz_cpmregl[0x6C] = (pllout2 / 48000000) - 1;

	nf = clock * 2 / CFG_EXTAL;
	plcr1 = ((nf - 2) << CPM_CPPCR_PLLM_BIT) | /* FD */
		(0 << CPM_CPPCR_PLLN_BIT) |	    /* RD=0, NR=2 */
		(0 << CPM_CPPCR_PLLOD_BIT) |    /* OD=0, NO=1 */
		(0x20 << CPM_CPPCR_PLLST_BIT) | /* PLL stable time */
		CPM_CPPCR_PLLEN;                /* enable PLL */          

	/* init PLL */
	//REG_CPM_CPCCR = cfcr;
	//REG_CPM_CPPCR = plcr1;
	cout << "set jz_cpmregl 0x10" << endl;
  jz_cpmregl[0] = cfcr;
  jz_cpmregl[0x10 >> 2] = plcr1;
	
  sdram_convert(clock,&sdramclock);
  if(sdramclock > 0){
		//REG_EMC_RTCOR = sdramclock;
		//REG_EMC_RTCNT = sdramclock;	  
		cout << "new sdramclock: " << sdramclock << endl;
		cout << "set jz_cpmregl 0x8c" << endl;
    jz_emcregs[0x8C >> 1] = sdramclock;
    jz_emcregs[0x88 >> 1] = sdramclock;	
  }
	else{
  	cout << "sdram init fail!" << endl;
  	while(1);
  } 
	cout << "pll_init --" << endl;	
}

void jz_cpuspeed(unsigned clockspeed) 
{
	cout << "jz_cpuspeed ++" << endl;
	if (clockspeed >= 200 && clockspeed <= 430){
		jz_dev = open("/dev/mem", O_RDWR);
		cout << "jz_dev: " << jz_dev << endl; 
		if(jz_dev){
			jz_cpmregl = (unsigned long*)mmap(0, 0x80, PROT_READ | PROT_WRITE, MAP_SHARED, jz_dev, 0x10000000);
			jz_emcregl = (unsigned long*)mmap(0, 0x90, PROT_READ | PROT_WRITE, MAP_SHARED, jz_dev, 0x13010000);
			jz_emcregs = (unsigned short*)jz_emcregl;
			pll_init(clockspeed * 1000000);
			munmap((void*)jz_cpmregl, 0x80); 
			munmap((void*)jz_emcregl, 0x90); 	
			close(jz_dev);
		}
		else
			cout << "failed opening /dev/mem" << endl;
	}
	cout << "jz_cpuspeed --" << endl;
}
