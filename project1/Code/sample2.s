#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#define DATASEG_ADDR 0x10000000
#define TEXTSEG_ADDR 0x00400000


int strToInt(char* str) {
	char* hex = "Xx";
	return (strchr(hex, str[1])) ? (int)strtol(str, NULL, 16) : atoi(str);
}

int limitBitSize(int integer, int bit) {
	return (integer & (int)(pow(2, bit) - 1));
}

int atoiWithBit(char* a, int bit) {
	int integer = strToInt(a); 
	return limitBitSize(integer, bit);
}


int main(int argc, char* argv[]) {
	// get file data
	FILE* fp;
	fp = fopen(argv[1], "r");
	
	fseek(fp, 0, SEEK_END);
	int fileSize = ftell(fp);
	
	char* buf =  (char*)malloc(sizeof(char)*fileSize);
	memset(buf, 0, fileSize);
	
	fseek(fp, 0, SEEK_SET);
	fread(buf, 1, fileSize, fp);
	
	fclose(fp);
	
	
	char* ptext = strstr(buf, ".text"); // pointer of .text
	
	
	// GET label count of DATASEG, TEXTSEG
	int nDatalabel = 0;
	int nTextlabel = 0;
	char* plabel = strstr(buf, ":");
	while(plabel != NULL) {
		(plabel < ptext) ? nDatalabel++ : nTextlabel++;
		plabel = strstr(plabel+1, ":");
	}


	// GET MAX_SIZE of DATASEG, TEXTSEG
	int dataseg_max_size = 0;
	int textseg_max_size = 0;
	char* plinebreak = strstr(buf, "\n");
	while(plinebreak != NULL) {
		(plinebreak < ptext) ? dataseg_max_size++ : textseg_max_size++;
		plinebreak = strstr(plinebreak+1, "\n");
	}
	
	
	char* delim = " \n,$()";
	char* token = strtok(buf, delim); // .data
	
	
	// data seg
	int dataValue[dataseg_max_size];
	char* dataLabel[nDatalabel];
	int dataLabelAddr[nDatalabel];
	
	int DataSegSize; 
	int cntWord = 0; // cnt of .word
	int cntDataLabel = 0; // cnt of label
	token = strtok(NULL, delim); // firt label of data seg
	while(token != NULL) {
		if(!strcmp(token, ".text")) {
			// end of data section
			break;
		} 
	
		if(!strcmp(token, ".word")) {
			// token result = .word
			token = strtok(NULL, delim);
			dataValue[cntWord++] = strToInt(token);
		}
		else {
			int ptrLen = strlen(token);
			if(!strcmp(token + ptrLen-1, ":")){
				// label
				token[ptrLen-1] = '\0';
				dataLabel[cntDataLabel] = token;
				dataLabelAddr[cntDataLabel] = (cntWord*4) + DATASEG_ADDR;
				cntDataLabel++;
			}
		}
		
		token = strtok(NULL, delim);	
	}
	
	DataSegSize = cntWord;
	
	
	// parsing text seg
	char* instructionTokens[textseg_max_size][4];
	char* textLabel[nTextlabel];
	int textLabelAddr[nTextlabel];
	
	int TextSegSize = 0;
	
	int nText;
	int cntIns = 0; // cnt of instruction
	int cntTextLabel = 0; // cnt of label
	
	char* ins_1[] = {"j", "jal", "jr"};
	char* ins_2[] = {"lui", "la"};
	char* ins_3[] = {"addu", "and", "nor", "or", "sltu", "sll", "srl", "subu", "addiu", "andi", "beq", "bne", "ori", "sltiu", "lb", "lw", "sw", "sb"};
	while(token != NULL) {
		int tokenlen = strlen(token);
		if(!strcmp(token + tokenlen-1, ":")){
			// label
			token[tokenlen-1] = '\0';
			textLabel[cntTextLabel] = token;
			textLabelAddr[cntTextLabel] = (cntIns*4) + TEXTSEG_ADDR;
			cntTextLabel++;
		}
		else {
			for(int i = 0; i < 3; i++) {
				if(!strcmp(ins_1[i], token)){
					instructionTokens[cntIns][0] = token;
					for(int j=1; j<2; j++){
						token = strtok(NULL, delim);
						instructionTokens[cntIns][j] = token;
					}
					instructionTokens[cntIns][2] = "";
					instructionTokens[cntIns][3] = "";
					cntIns++;
				}
			}
			
			for(int i = 0; i < 2; i++) {
				if(!strcmp(ins_2[i], token)){
					instructionTokens[cntIns][0] = token;
					for(int j=1; j<3; j++){
						token = strtok(NULL, delim);
						instructionTokens[cntIns][j] = token;
					}
					instructionTokens[cntIns][3] = "";
					
					// la should do ori 
					if(!strcmp("la", instructionTokens[cntIns][0])){
						for(int k = 0; k < cntDataLabel; k++) { 
							if(!strcmp(dataLabel[k], token)) {
								int lowerAddr = limitBitSize(dataLabelAddr[k], 4);
								if(lowerAddr) { TextSegSize++; }
							}
						}
					}
				
					cntIns++;
				}
			}
			
			for(int i = 0; i < 18; i++) {
				if(!strcmp(ins_3[i], token)){
					instructionTokens[cntIns][0] = token;
					for(int j=1; j<4; j++){
						token = strtok(NULL, delim);
						instructionTokens[cntIns][j] = token;
					}
					cntIns++;
				}
			}
			
	
		}

		token = strtok(NULL, delim);
	}
	
	nText = cntIns;
	TextSegSize += cntIns;
	

	// object file
	char* ofile = argv[1];
	ofile[strlen(ofile) -1] = 'o';
	
	fp = fopen(ofile, "w");
	
	// print section size
	fprintf(fp, "0x%x\n", TextSegSize*4);
	fprintf(fp, "0x%x\n", DataSegSize*4);
	
	// print instruction
	char* rFormat[] = {"addu", "and", "jr", "nor", "or", "sltu", "sll", "srl", "subu"};
	char* iFormat[] = {"addiu", "andi", "beq", "bne", "lui", "lw", "ori", "sltiu", "sw", "lb", "sb", "la"};
	char* jFormat[] = {"j", "jal"};
	
	int Rformat_funct[] = {33, 36, 8, 39, 37, 43, 0, 2, 35};
	int Iformat_op[] = {9, 12, 4, 5, 15, 35, 13, 11, 43, 32, 40, -1};
	int Jformat_op[] = {2, 3};
	
	char* op;
	int isFindFormat;
	int instruction;
	
	for(int i = 0; i < nText; i++) {
		op = instructionTokens[i][0];
		isFindFormat = 0;
		instruction = 0;
		
		// rFormat
		for(int j = 0; j < 9; j++) {
			if(!strcmp(rFormat[j], op)){
				// skip op. in R, it is only 0
				switch(j){
				case 2: // jr
					instruction |= (strToInt(instructionTokens[i][1]) << 21); // rs
					instruction |= Rformat_funct[j]; // funct
					break;
				case 6: // sll
				case 7: // srl
					instruction |= (strToInt(instructionTokens[i][1]) << 11); // rd
					instruction |= (strToInt(instructionTokens[i][2]) << 16); // rt
					instruction |= (strToInt(instructionTokens[i][3]) << 6); // shamt : unsinged int
					instruction |= Rformat_funct[j]; // funct
					break;
				default: // addu and nor or sltu subu
					instruction |= (strToInt(instructionTokens[i][1]) << 11); // rd
					instruction |= (strToInt(instructionTokens[i][2]) << 21); // rs
					instruction |= (strToInt(instructionTokens[i][3]) << 16); // rt
					instruction |= Rformat_funct[j]; // funct*/
				}
				fprintf(fp, "0x%x\n", instruction);
				isFindFormat++;
				break;
			}
		}
		if(isFindFormat) { continue; }
		
		// iFormat
		for(int j = 0; j < 12; j++) {
			if(!strcmp(iFormat[j], op)){
				instruction |= (Iformat_op[j] << 26); // op
				
				switch(j){
				case 2: // beq
				case 3: // bne
					instruction |= (strToInt(instructionTokens[i][1]) << 21); // rs
					instruction |= (strToInt(instructionTokens[i][2]) << 16); // rt
					char* label = instructionTokens[i][3];
					for(int k = 0; k < cntTextLabel; k++) { 
						if(!strcmp(label, textLabel[k])) {
							int offset = (textLabelAddr[k] - (TEXTSEG_ADDR + 4*i + 4)) >> 2;
							instruction |= (limitBitSize(offset, 16)); // offset label -> unsigned int
							break;
						}
					}
					
					break;
				case 4: // lui
					instruction |= (strToInt(instructionTokens[i][1]) << 16); // rt
					instruction |= atoiWithBit(instructionTokens[i][2], 16); // imm int
					
					break;
				case 5: // lw
				case 8: // lb
				case 9: // sw
				case 10: // sb
					instruction |= (strToInt(instructionTokens[i][1]) << 16); // rt
					instruction |= strToInt(instructionTokens[i][2]); // offset
					instruction |= (atoiWithBit(instructionTokens[i][3], 16) << 21); // rs
					
					break;
				case 11: // la
					for(int k = 0; k < cntDataLabel; k++) { 
						if(!strcmp(instructionTokens[i][2], dataLabel[k])) {
							// lui(la)
							instruction = 0;
							instruction |= (Iformat_op[4] << 26); // op
							instruction |= (strToInt(instructionTokens[i][1]) << 16); // rt
							instruction |= limitBitSize(dataLabelAddr[k] >> 16, 16); // imm
							
							int lowerAddr = limitBitSize(dataLabelAddr[k], 4);
							if(lowerAddr) {
								// ori(la)
								fprintf(fp, "0x%x\n", instruction);
								
								instruction = 0;
								instruction |= (Iformat_op[6] << 26); // op
								instruction |= (strToInt(instructionTokens[i][1]) << 21); // rs
								instruction |= (strToInt(instructionTokens[i][1]) << 16); // rt
								instruction |= limitBitSize(lowerAddr, 16); // imm
							}
							break;
						}
					}
					
					break;
				default: // addiu andi ori sltiu sw
					instruction |= (strToInt(instructionTokens[i][1]) << 16); // rt
					instruction |= (strToInt(instructionTokens[i][2]) << 21); // rs
					instruction |= atoiWithBit(instructionTokens[i][3], 16); // imm
					
				}
				fprintf(fp, "0x%x\n", instruction);
				
				isFindFormat++;
				break;
			}
		}
		if(isFindFormat) { continue; }
		
		// jFormat
		for(int j = 0; j < 2; j++) {
			if(!strcmp(jFormat[j], op)){
				char* target = instructionTokens[i][1];
				
				instruction |= (Jformat_op[j] << 26); // op
				for(int k = 0; k < cntTextLabel; k++) { 
					if(!strcmp(target, textLabel[k])) {
						instruction |= (limitBitSize(textLabelAddr[k] >> 2, 26)); // target
						break;
					}
				}
				
				fprintf(fp, "0x%x\n", instruction);
				isFindFormat++;
				break;
			
			}
		}
		if(isFindFormat) { continue; }
	}
	
	//print word label
	for(int i = 0; i < cntWord; i++) {
		fprintf(fp, "0x%x\n", dataValue[i]);
 	}

	

	fclose(fp);
	free(buf);
	
	return 0;
}



