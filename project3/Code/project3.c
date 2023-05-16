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
	
	
typedef struct IFID_StateRegister {
	int val; // 0: noop
	int stall; // should stall
	
	int pc; // pc running in 
	int NPC; // next instr for jump
	int instr; // instr running in
} IFID;



typedef struct IDEX_StateRegister {	
	int val; // 0: noop
	int stall;
	
	int pc; // pc running in 
	int NPC; // next instr for jump
	
	int op;
	int rs; 
	int rt; 
	int rd; 
	int simm; // sign extended imm
	int uimm; // unsign extended imm
	
	int control; // control bit = MemtoReg ,RegWrite / OneByte, Brch, MemRead, MemWrite / 
} IDEX;

typedef struct EXMEM_StateRegister {
	int val;
	int pc; // pc running in 
	
	int ALUOUT;
	int rt;
	int BRTarget;
	int REGDst;
	
	int control; // control bit = MemtoReg ,RegWrite / OneByte, Brch, MemRead, MemWrite / 
} EXMEM;

typedef struct MEMWB_StateRegister {
	int val;	
	int pc; // pc running in 
	
	int ALUOUT; 
	int MEMOUT; 
	int REGDst;
	
	int control; // control bit = MemtoReg ,RegWrite
} MEMWB;


int strToInt(char* str) {
	char* hex = "Xx";
	return (strchr(hex, str[1])) ? (int)strtol(str, NULL, 16) : atoi(str);
}


// sb
int saveByteToMem(unsigned int addr, unsigned int rt) {
	unsigned int data = regs[rt] & 255;
	
	unsigned int offset = addr % 4;
	unsigned int offset_arr[4] = {0x00ffffff , 0xff00ffff , 0xffff00ff, 0xffffff00};
	
	DATA_SEGMENT[addr/4] &= offset_arr[offset];
	DATA_SEGMENT[addr/4] |= (data << ((3 - offset) * 8));
}

// lb
int loadByteFormMem(unsigned int addr) {
	unsigned int offset = addr % 4;
	
	unsigned int word = DATA_SEGMENT[addr/4];
	int byte = (word >> ((3-offset) * 8)) & 255;
	
	return byte;
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
	
	printf("Memory content [0x%x..0x%x] :\n", option_m[0], option_m[1]);
	printf("-------------------------------------------\n");
	for(unsigned int addr = start; addr <= end; addr=addr+4) {
		printf("0x%x: 0x0\n", addr);
	}
}

int printPipelineState(int TEXTSEG_SIZE, int* pipeline) {
	printf("Current pipeline PC state :\n{");
	
	if(TEXTSEG_SIZE > (pipeline[0]-TEXTSEG_ADDR) && pipeline[0] >= TEXTSEG_ADDR) { printf("%#x", pipeline[0]); } printf("|");
	if(pipeline[1]) { printf("%#x", pipeline[1]); } printf("|");
	if(pipeline[2]) { printf("%#x", pipeline[2]); } printf("|");
	if(pipeline[3]) { printf("%#x", pipeline[3]); } printf("|");
	if(pipeline[4]) { printf("%#x", pipeline[4]); }
	printf("}\n");

}


int main(int argc, char* argv[]) {
	// Register initialize
	for(int i=0; i < 32; i++) { regs[i] = 0; }
	
	int option_taken = 1;
	int option_m[2] = {-1, -1};
	int option_d = 0;
	int option_p = 0;
	int option_n = -1;
	
	// set argument
	if(!strcmp(argv[1], "-atp")) {
		option_taken = 1;
	} else if(!strcmp(argv[1], "-antp")) {
		option_taken = 0;
	} else {
		printf("input error. check antp or atp");
		return 1;
	}
	for(int i = 2; i < argc-1; i++) {
		if(!strcmp(argv[i], "-m")) {
			char* arg = argv[++i];
			option_m[0] = strToInt(strtok(arg, ":"));
			option_m[1] = strToInt(strtok(NULL, ":"));
		} 
		else if(!strcmp(argv[i], "-d")) {
			option_d = 1;
		} 
		else if(!strcmp(argv[i], "-p")) {
			option_p = 1;
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
	if(!fileSize || fileSize == 1) {
		printf("==== Completion cycle : %d ====\n\n", 0);
		
		printf("Current pipeline PC state :\n");
		printf("{||||}\n");
		
		printf("\n");
		printRegs();
		if(option_m[0] != -1){
			printNullDataSeg(option_m);
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
	
	
	IFID _IFID = {0, };
	IDEX _IDEX = {0, };
	EXMEM _EXMEM = {0, };
	MEMWB _MEMWB = {0, };
	
	int cycles = 0;
	int pipeline_output[5] = {0, 0, 0, 0, 0};
	while(1) {
		if(option_n == 0) { break; }
		if(TEXTSEG_SIZE <= (pc-TEXTSEG_ADDR) && !(_IFID.val) && !(_IDEX.val) && !(_EXMEM.val) && !(_MEMWB.val)) { break; }
			
		IFID next_IFID = {0, };
		IDEX next_IDEX = {0, };
		EXMEM next_EXMEM = {0, };
		MEMWB next_MEMWB = {0, };
	   	for(int i=0; i<5; i++) {pipeline_output[i] = 0;}
		unsigned int jumptarget = 0;
		unsigned int ID_BRTarget = 0;
		unsigned int MEM_Taken = 0;
		
		// WB
		if(_MEMWB.val){       
        		
			pipeline_output[4] = _MEMWB.pc;
			if(_MEMWB.control == 3) { // MemtoReg, RegWrite
				regs[_MEMWB.REGDst] = _MEMWB.MEMOUT;
			}
			else if(_MEMWB.control == 1) { // RegWrite
				regs[_MEMWB.REGDst] = _MEMWB.ALUOUT;
			}
			
			option_n -= 1;
		}
		
		// MEM
		if(_EXMEM.val){
			int rt_value = regs[_EXMEM.rt];
			// forward MEMWB to MEM for sb, sw
			if((_MEMWB.control & 1) && (_MEMWB.REGDst != 0)){
				if(_MEMWB.REGDst == _EXMEM.rt) {
					rt_value = regs[_MEMWB.REGDst];
				}
			}
			 
			
			pipeline_output[3] = _EXMEM.pc;
			switch(_EXMEM.control) {
				case 1:  //sw
					DATA_SEGMENT[_EXMEM.ALUOUT/4] = regs[_EXMEM.rt];
					break;
				case 5:  //sb
					saveByteToMem(_EXMEM.ALUOUT, _EXMEM.rt);
					break;
				case 98:  // lw
					next_MEMWB.MEMOUT = DATA_SEGMENT[_EXMEM.ALUOUT/4];
					break;
				case 102: // lb
					next_MEMWB.MEMOUT = loadByteFormMem(_EXMEM.ALUOUT);
					break;
				case 16: // beq
					if(!_EXMEM.ALUOUT) { MEM_Taken = 1; }
					break;
				case 8: // bne
					if(_EXMEM.ALUOUT) { MEM_Taken = 1; }
					break;
			}
			
			next_MEMWB.pc = _EXMEM.pc;
			next_MEMWB.ALUOUT = _EXMEM.ALUOUT;
			next_MEMWB.REGDst = _EXMEM.REGDst;
			next_MEMWB.control = _EXMEM.control >> 5;
			next_MEMWB.val = 1;
			
			
			
			if(_EXMEM.control == 16 || _EXMEM.control == 8) {
				if((option_taken && !MEM_Taken) || (!option_taken && MEM_Taken)) {
					// prediction failed
					pc = (option_taken) ? _EXMEM.pc + 4:_EXMEM.BRTarget;
					
					next_EXMEM.val = 0;
					next_EXMEM.pc = 0;
					next_IDEX.val = 0;
					next_IDEX.pc = 0;
					next_IFID.val = 0;
					next_IFID.pc = 0;
					
					_EXMEM = next_EXMEM;
					_IDEX = next_IDEX;
					_IFID = next_IFID;
					
					_MEMWB = next_MEMWB;
					
					
					printf("==== Cycle %d ====\n\n", ++cycles);
					if(option_p) {
						printPipelineState(TEXTSEG_SIZE, pipeline_output);
						printf("\n");
					}
					
					// print
					if(option_d) {
						printRegs();
						if(option_m[0] != -1){
							printDataSeg(DATA_SEGMENT, TEXT_SEGMENT, DATASEG_SIZE, TEXTSEG_SIZE, option_m);
						}
					}
					
					continue;
				}
			}
			
		}
		
		// EX
		if(_IDEX.val){
		
			pipeline_output[2] = _IDEX.pc;
			
			int rs_value = regs[_IDEX.rs];
			int rt_value = regs[_IDEX.rt];
			int rd_value = regs[_IDEX.rd];
			
			
			// forward EXMEM to EX
			if((_EXMEM.control & 32) && (_EXMEM.REGDst != 0)){
				if(_EXMEM.REGDst == _IDEX.rs) { //forward rs
					rs_value = _EXMEM.ALUOUT;
				}
				if(_EXMEM.REGDst == _IDEX.rt) { //forward rt
					rt_value = _EXMEM.ALUOUT;
				}
			}
			
			// forward MEMWB to EX
			if((_MEMWB.control & 1) && (_MEMWB.REGDst != 0) && (_EXMEM.REGDst != _EXMEM.REGDst)){
				if(_MEMWB.REGDst == _IDEX.rs) {
					rs_value = _MEMWB.ALUOUT; //forward rs
				}
				if(_MEMWB.REGDst == _IDEX.rt) {
					rt_value = _MEMWB.ALUOUT; //forward rt
				}
			}
			
			
			// ALU 
			if(_IDEX.op == 0) {
				next_EXMEM.REGDst = _IDEX.rd;
				int shamt = (_IDEX.uimm >> 6) & 31;
				int funct = _IDEX.uimm & 63;
				switch(funct) {
					case 0x21: // addu
						next_EXMEM.ALUOUT = (rs_value + rt_value) & 0xffffffff;
						break;
					case 0x24: // and
						next_EXMEM.ALUOUT = rs_value & rt_value;
						break;
					case 0x2b: // sltu
						next_EXMEM.ALUOUT = (rs_value < rt_value) ? 1 : 0;
						break;
					case 0: // sll
						next_EXMEM.ALUOUT = rt_value << shamt;
						break;
					case 2: // srl
						next_EXMEM.ALUOUT = rt_value >> shamt;
						break;
					case 0x25: // or
						next_EXMEM.ALUOUT = rs_value | rt_value;
						break;
					case 0x27: // nor
						next_EXMEM.ALUOUT = ~(rs_value | rt_value);
						break;
					case 0x23: // subu
						next_EXMEM.ALUOUT = rs_value - rt_value;
						break;
					case 8: // jr
						next_EXMEM.ALUOUT = rs_value;
						break;
				}
			}
			else if(_IDEX.op == 3) {
				next_EXMEM.ALUOUT = _IDEX.NPC;
				next_EXMEM.REGDst = 31;
			}
			else{
			// I format
				next_EXMEM.REGDst = _IDEX.rt;
				switch(_IDEX.op) {
					case 9: // addiu(signed extended)
						next_EXMEM.ALUOUT = rs_value + _IDEX.simm;
						break;
					case 0xc: // andi
						next_EXMEM.ALUOUT = rs_value & _IDEX.uimm;
						break;
					case 0xf: // lui
						next_EXMEM.ALUOUT = _IDEX.uimm << 16;
						break;
					case 0xd: // ori
						next_EXMEM.ALUOUT = rs_value | _IDEX.uimm;
						break;
					case 0xb: // sltiu(signed extended)
						next_EXMEM.ALUOUT = (rs_value < _IDEX.simm) ? 1 : 0;
						break;
					case 4: // beq
					case 5: // bne
						next_EXMEM.BRTarget = _IDEX.NPC + (_IDEX.simm * 4);
						next_EXMEM.ALUOUT = rs_value - rt_value;
						break;
					case 0x23: // lw
					case 0x20: // lb
						next_EXMEM.ALUOUT = rs_value + _IDEX.simm - DATASEG_ADDR;
						break;
					case 0x2b: // sw
					case 0x28: // sb
						next_EXMEM.ALUOUT = rs_value + _IDEX.uimm - DATASEG_ADDR;
						break;
				}
			}
			
			
			next_EXMEM.pc = _IDEX.pc;
			next_EXMEM.rt = _IDEX.rt;
			next_EXMEM.control = _IDEX.control;
			next_EXMEM.val = 1;
			
			
		}
		
		// ID
		if(_IFID.val) {
			pipeline_output[1] = _IFID.pc;
			int instruction = _IFID.instr;
			int op = (instruction >> 26) & 63; // op
			unsigned int rs = (instruction >> 21) & 31;
			unsigned int rt = (instruction >> 16) & 31;
			unsigned int rd = (instruction >> 11) & 31;
			
			unsigned int uimm = instruction & (int)(pow(2, 16) - 1);
			int simm = (uimm >> 15) ? 0xffff0000 | uimm : uimm;
			
			
			// stall by loaduse
			if((_IDEX.val) && (_IDEX.control & 2) && ((_IDEX.rt == rs) || (_IDEX.rt == rt))){
				pipeline_output[1] = 0;
				pipeline_output[0] = _IFID.pc;

				// cycle end
				next_IDEX.val = 0;
				next_IDEX.pc = 0;
				_IDEX = next_IDEX;
				
				_EXMEM = next_EXMEM;
				_MEMWB = next_MEMWB;
				
				printf("==== Cycle %d ====\n\n", ++cycles);
				if(option_p) {
					printPipelineState(TEXTSEG_SIZE, pipeline_output);
					printf("\n");
				}
				
				// print
				if(option_d) {
					printRegs();
					if(option_m[0] != -1){
						printDataSeg(DATA_SEGMENT, TEXT_SEGMENT, DATASEG_SIZE, TEXTSEG_SIZE, option_m);
					}
				}
				
				
				continue;
			}
			
			// control bit = MemtoReg ,RegWrite / eqBrch, neBrch, OneByte, MemRead, MemWrite / 
			switch(op) {
				case 0: // R type
					next_IDEX.control = 32; // RegWrite
					int funct = instruction & 63;
					if(funct == 8) { // jr
						next_IDEX.control = 0; 
						jumptarget = instruction & (int)(pow(2, 26) - 1);
						jumptarget = regs[rs];
					} 
					break;
				case 2: // j
					next_IDEX.control = 0;
					jumptarget = instruction & (int)(pow(2, 26) - 1);
					jumptarget = (pc & (15 << 28)) | (jumptarget*4);
					break;
				case 3: // jal
					next_IDEX.control = 32; // RegWrite
					jumptarget = instruction & (int)(pow(2, 26) - 1);
					jumptarget = (pc & (15 << 28)) | (jumptarget*4);
					break;
				case 9: // addiu(signed extended)
				case 0xc: // andi
				case 0xf: // lui
				case 0xd: // ori
				case 0xb: // sltiu(signed extended)
					next_IDEX.control = 32; // RegWrite
					break;
				case 4: // beq
					next_IDEX.control = 16;
					ID_BRTarget = _IFID.NPC + (simm * 4);
					break;
				case 5: // bne
					next_IDEX.control = 8;
					ID_BRTarget = _IFID.NPC + (simm * 4);
					break;
				case 0x2b: // sw
					next_IDEX.control = 1;
					break;
				case 0x28: // sb
					next_IDEX.control = 5;
					break;
				case 0x23: // lw
					next_IDEX.control = 98;
					break;
				case 0x20: // lb
					next_IDEX.control = 102;
					break;
			}
			
			next_IDEX.pc = _IFID.pc;
			next_IDEX.NPC = _IFID.NPC;
			
			next_IDEX.op = op;
			next_IDEX.rs = rs;
			next_IDEX.rt = rt;
			next_IDEX.rd = rd;
			next_IDEX.uimm = uimm;
			next_IDEX.simm = simm;
			
			next_IDEX.val = 1;
			
		}	
		
		
		// IF
		if(TEXTSEG_SIZE > (pc-TEXTSEG_ADDR) && pc >= TEXTSEG_ADDR) {
			pipeline_output[0] = pc;
			next_IFID.pc = pc;
			next_IFID.NPC = pc+4;
			next_IFID.instr = TEXT_SEGMENT[(pc-TEXTSEG_ADDR)/4];
			next_IFID.val = 1;
			
			pc = pc + 4;
		}
		
		if(jumptarget) { 
			pc = jumptarget; 
			pipeline_output[0] = 0;
			next_IFID.val = 0;
			next_IFID.pc = 0;
		}
		if(ID_BRTarget && option_taken) {
			pc = ID_BRTarget; 
			pipeline_output[0] = 0;
			next_IFID.val = 0;
			next_IFID.pc = 0;
		}
		

		// cycle end
		_IFID = next_IFID;
		_IDEX = next_IDEX;
		_EXMEM = next_EXMEM;
		_MEMWB = next_MEMWB;
		
		printf("==== Cycle %d ====\n\n", ++cycles);
		if(option_p) {
			printPipelineState(TEXTSEG_SIZE, pipeline_output);
			printf("\n");
		}
		
		// print
		if(option_d) {
			printRegs();
			if(option_m[0] != -1){
				printDataSeg(DATA_SEGMENT, TEXT_SEGMENT, DATASEG_SIZE, TEXTSEG_SIZE, option_m);
			}
		}
	}
	
	
	printf("==== Completion cycle : %d ====\n\n", cycles);
	int last_pipline[5] = {pc, _IFID.pc, _IDEX.pc, _EXMEM.pc, _MEMWB.pc};
	if(option_n == 0) { printPipelineState(TEXTSEG_SIZE, pipeline_output); }
	else { 
		int last_pipline[5] = {pc, _IFID.pc, _IDEX.pc, _EXMEM.pc, _MEMWB.pc};
		printPipelineState(TEXTSEG_SIZE, last_pipline); 
	}
	
	printf("\n");
	printRegs();
	if(option_m[0] != -1){
		printDataSeg(DATA_SEGMENT, TEXT_SEGMENT, DATASEG_SIZE, TEXTSEG_SIZE, option_m);
	}
	
	

	free(DATA_SEGMENT);
	free(TEXT_SEGMENT);
	
	return 0;
}