volatile unsigned signature[4];
unsigned values[32];
void main(void) {
 unsigned sum=0, mix=0x12345678u;
 for(unsigned i=0;i<32;i++) values[i]=(i<<2)^0x135u;
 for(unsigned i=0;i<32;i++){sum+=values[i];mix=(mix<<5)^(mix>>27)^values[i];}
 signature[0]=sum;signature[1]=mix;signature[2]=values[0];signature[3]=values[31];
 __asm__ volatile("ebreak");
 for(;;){}
}
