#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#define DATASEG_ADDR 0x10000000
#define TEXTSEG_ADDR 0x00400000


unsigned int regs[32];
unsigned int pc = TEXTSEG_ADDR;

int* TEXT_SEGMENT;
int* DATA_SEGMENT;
	
int strToInt(char* str) {
	char* hex = "Xx";
	return (strchr(hex, str[1])) ? (int)strtol(str, NULL, 16) : atoi(str);
}


// sb
int saveByteToMem(unsigned int rs, unsigned int rt, unsigned int uimm) {
	unsigned int addr = regs[rs] + uimm - DATASEG_ADDR;
	unsigned int data = regs[rt] & 255;
	
	unsigned int offset = addr % 4;
	unsigned int offset_arr[4] = {0x00ffffff , 0xff00ffff , 0xffff00ff, 0xffffff00};
	
	DATA_SEGMENT[addr/4] &= offset_arr[offset];
	DATA_SEGMENT[addr/4] |= (data << ((3 - offset) * 8));
}

// lb
int loadByteFormMem(unsigned int rs, unsigned int rt, int simm) {
	unsigned int addr = regs[rs] + simm - DATASEG_ADDR;
	unsigned int offset = addr % 4;
	
	unsigned int word = DATA_SEGMENT[addr/4];
	unsigned int byte = (word >> ((3-offset) * 8)) & 255;
	
	regs[rt] = byte;
}
						

int printRegs() {
	printf("Current register values :\n");
	printf("-------------------------------------------\n");
	printf("PC: 0x%x\n", pc);
	printf("Registers:\n");
	for(int i =0; i< 32; i++) {
		printf("R%d: 0x%x\n", i, regs[i]);
	}
	printf("\n");
}

int printDataSeg(int* dataseg, int* textseg,int dataSize, int textSize, int* option_m) {
	int start = option_m[0];
	int end = option_m[1];
	printf("Memory content [0x%x..0x%x] :\n", option_m[0], option_m[1]);
	printf("-------------------------------------------\n");
	
	for(unsigned int addr = start; addr <= end; addr=addr+4) {
		if(TEXTSEG_ADDR <= addr & addr < TEXTSEG_ADDR + textSize){
			printf("0x%x: 0x%x\n", addr, textseg[(addr - TEXTSEG_ADDR)/4]);
		}
		else if(DATASEG_ADDR <= addr & addr < DATASEG_ADDR + dataSize){
			printf("0x%x: 0x%x\n", addr, dataseg[(addr - DATASEG_ADDR)/4]);
		}
		else{
			printf("0x%x: 0x0\n", addr);
		}
	}
}

int printNullDataSeg(int* option_m) {
	int start = option_m[0];
	int end = option_m[1];
	
	printf("Memory content [0x%x..0x%x]:\n", option_m[0], option_m[1]);
	printf("----------------------------------\n");
	for(unsigned int addr = start; addr <= end; addr=addr+4) {
		printf("0x%x: 0x0\n", addr);
	}
}


int main(int argc, char* argv[]) {
	// Register initialize
	for(int i=0; i < 32; i++) { regs[i] = 0; }
	
	int option_m[2] = {-1, -1};
	int option_d = 0;
	int option_n = -1;
	
	// set argument
	for(int i = 1; i < argc-1; i++) {
		if(!strcmp(argv[i], "-m")) {
			char* arg = argv[++i];
			option_m[0] = strToInt(strtok(arg, ":"));
			option_m[1] = strToInt(strtok(NULL, ":"));
		} 
		else if(!strcmp(argv[i], "-d")) {
			option_d = 1;
		} 
		else if(!strcmp(argv[i], "-n")) {
			option_n = strToInt(argv[++i]);
		}
	}
	
	// load file
	FILE* fp;
	fp = fopen(argv[argc-1], "r");
	
	fseek(fp, 0, SEEK_END);
	int fileSize = ftell(fp);
	
	char* buf =  (char*)malloc(sizeof(char)*fileSize);
	memset(buf, 0, fileSize);
	
	fseek(fp, 0, SEEK_SET);
	fread(buf, 1, fileSize, fp);
	
	fclose(fp);
	
	
	// if filesize is zero, then just print
	if(!fileSize) {
		if(!option_d) {
			printRegs();
			if(option_m[0] != -1){
				printNullDataSeg(option_m);
			}
		}
		
		free(buf);
		return 0;
	}
	
	
	// Load textseg, dataseg
	int TEXTSEG_SIZE = strToInt(strtok(buf, "\n"));
	int DATASEG_SIZE = strToInt(strtok(NULL, "\n"));
	
	DATA_SEGMENT = (int*)malloc(DATASEG_SIZE);
	TEXT_SEGMENT = (int*)malloc(TEXTSEG_SIZE);
		
	for(int i = 0; i < TEXTSEG_SIZE/4; i++) {
		TEXT_SEGMENT[i] = strToInt(strtok(NULL, "\n"));
	}
	for(int i = 0; i < DATASEG_SIZE/4; i++) {
		DATA_SEGMENT[i] = strToInt(strtok(NULL, "\n"));
	}
	


	while(1) {
		if(option_n == 0) { break; }
		if(TEXTSEG_SIZE <= (pc-TEXTSEG_ADDR)) { break; }
			
		int instruction = TEXT_SEGMENT[(pc-TEXTSEG_ADDR)/4];
		
		// PC update
		pc = pc + 4;
		
		int op = (instruction >> 26) & 63; // op
		
		// R format
		if(op == 0) {
			unsigned int rs = (instruction >> 21) & 31;
			unsigned int rt = (instruction >> 16) & 31;
			unsigned int rd = (instruction >> 11) & 31;
			int shamt = (instruction >> 6) & 31;
			int funct = instruction & 63;
			
			switch(funct) {
				case 0x21: // addu
					regs[rd] = (regs[rs] + regs[rt]) & 0xffffffff;
					break;
				case 0x24: // and
					regs[rd] = regs[rs] & regs[rt];
					break;
				case 0x2b: // sltu
					regs[rd] = (regs[rs] < regs[rt]) ? 1 : 0;
					break;
				case 0: // sll
					regs[rd] = regs[rt] << shamt;
					break;
				case 2: // srl
					regs[rd] = regs[rt] >> shamt;
					break;
				case 0x25: // or
					regs[rd] = regs[rs] | regs[rt];
					break;
				case 0x27: // nor
					regs[rd] = ~(regs[rs] | regs[rt]);
					break;
				case 0x23: // subu
					regs[rd] = regs[rs] - regs[rt];
					break;
				case 8: // jr
					pc = regs[rs];
					break;
			}
		}
		else {
			if(op == 2 | op == 3) {
				// J format
				unsigned int target = instruction & (int)(pow(2, 26) - 1);
				
				if(op == 3) {
					// jal
					regs[31] = pc;
				}
				pc = (pc & (15 << 28)) | (target*4);
			}
			else {
				// I format
				unsigned int uimm = instruction & (int)(pow(2, 16) - 1);
				int simm = (uimm >> 15) ? 0xffff0000 | uimm : uimm;
				unsigned int rs = (instruction >> 21) & 31;
				unsigned int rt = (instruction >> 16) & 31;
				
				switch(op) {
					case 9: // addiu(signed extended)
						regs[rt] = regs[rs] + simm;
						break;
					case 0xc: // andi
						regs[rt] = regs[rs] & uimm;
						break;
					case 4: // beq
						if(regs[rs] == regs[rt]) {
							pc += (simm * 4);
						}
						break;
					case 5: // bne
						if(regs[rs] != regs[rt]) {
							pc += (simm * 4);
						}
						break;
					case 0x2b: // sw
						DATA_SEGMENT[(regs[rs] + uimm - DATASEG_ADDR)/4] = regs[rt];
						// save regs[rt] in rs or rs+offset
						break;
					case 0x28: // sb
						saveByteToMem(rs, rt, uimm);
						// save regs[rt] 1byte in rs or rs+offset 
						break;
					case 0xf: // lui
						regs[rt] = uimm << 16;
						break;
					case 0x23: // lw
						regs[rt] = DATA_SEGMENT[(regs[rs] + simm - DATASEG_ADDR)/4];
						break;
					case 0x20: // lb
						loadByteFormMem(rs, rt, simm);
						break;
					case 0xd: // ori
						regs[rt] = regs[rs] | uimm;
						break;
					case 0xb: // sltiu(signed extended)
						regs[rt] = (regs[rs] < simm) ? 1 : 0;
						break;
				}
			}
		}
		option_n -= 1;
		if(option_d) {
			printRegs();
			if(option_m[0] != -1){
				printDataSeg(DATA_SEGMENT, TEXT_SEGMENT, DATASEG_SIZE, TEXTSEG_SIZE, option_m);
			}
		}
	}
	
	if(!option_d) {
		printRegs();
		if(option_m[0] != -1){
			printDataSeg(DATA_SEGMENT, TEXT_SEGMENT, DATASEG_SIZE, TEXTSEG_SIZE, option_m);
		}
	}

	free(DATA_SEGMENT);
	free(TEXT_SEGMENT);
	
	return 0;
}