//
//	CS311 Project 1 MIPS Assembler
//	main.c
//	20200136 Insung Kim
//
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <assert.h>

extern char *strdup(const char *);

struct HashNode {
    char *key;
    uint32_t *value;
    struct HashNode *next;
};

struct HashMap {
    int size;
    struct HashNode **map;
};

void Reg2Bin(char reg[6]);
void Num2Bin(char num[16], int arr[32], int bits);
void Dec2Bin(int val, int arr[32], int bits);
struct HashMap *createMap(int size);
int HashCode(struct HashMap *h, char *key);
void HashMap_add(struct HashMap *h, char *key, uint32_t *value);
uint32_t *HashMap_getvalue(struct HashMap *h, char *key);

int main(int argc, char* argv[]) {
	char opcode[32], rs[6], rt[6], rd[6], shamt[6], imm[16], baddr[16], jaddr[32], wordkey[6];
	int num[32], word[10], size_data = 0, size_text = 0, if_word = 0, word_count = 0;
	int32_t pc = 0x00000000;
	if(argc != 2) {
		printf("Usage: ./runfile <assembly file>\n"); //Example) ./runfile /sample_input/example1.s
		printf("Example) ./runfile ./sample_input/example1.s\n");
		exit(0);
	} else {
		// Create a new hashmap
		struct HashMap *map = createMap(127);							// store label and address

		// For input file read (sample_input/example*.s)
		char *file = (char *)malloc(strlen(argv[1]) + 3);
		strncpy(file, argv[1], strlen(argv[1]));
		if(freopen(file, "r", stdin) == 0) {
			printf("File open Error!\n");
			exit(1);
		}

		// count size of text section and size of data section
		while(scanf("%s", opcode) != EOF) {
			if(strchr(opcode, ':') != NULL) {
				opcode[strlen(opcode) - 1] = '\0';
				uint32_t *pc_point = (uint32_t *)malloc(sizeof(uint32_t));
				*pc_point = pc;
				HashMap_add(map, opcode, pc_point);
			}
			if(strcmp(opcode, ".data") == 0) pc = 0x10000000;
			if(strcmp(opcode, ".text") == 0) pc = 0x400000;
			if(if_word == 1) {
				int wordnum;
				if(strncmp(opcode, "0x", 2) == 0) wordnum = strtol(opcode, NULL, 16);
				else wordnum = atoi(opcode);
				pc += 4;
				uint32_t *pc_point = (uint32_t *)malloc(sizeof(uint32_t));
				*pc_point = pc;
				word[word_count++] = wordnum;
				HashMap_add(map, opcode, pc_point);
				if_word = 0;
			}
			if(strcmp(opcode, ".word") == 0) {
				size_data++;
				if_word = 1;
			}
			else if(strcmp(opcode, "addiu") == 0) size_text++, pc += 4;
			else if(strcmp(opcode, "addu") == 0) size_text++, pc += 4;
			else if(strcmp(opcode, "and") == 0) size_text++, pc += 4;
			else if(strcmp(opcode, "andi") == 0) size_text++, pc += 4;
			else if(strcmp(opcode, "beq") == 0) size_text++, pc += 4;
			else if(strcmp(opcode, "bne") == 0) size_text++, pc += 4;
			else if(strcmp(opcode, "j") == 0) size_text++, pc += 4;
			else if(strcmp(opcode, "jal") == 0) size_text++, pc += 4;
			else if(strcmp(opcode, "jr") == 0) size_text++, pc += 4;
			else if(strcmp(opcode, "lui") == 0) size_text++, pc += 4;
			else if(strcmp(opcode, "lw") == 0) size_text++, pc += 4;
			else if(strcmp(opcode, "la") == 0) {
				scanf("%s", rt), scanf("%s", baddr);
				int *address = HashMap_getvalue(map, baddr);
				int immediate = *address;							// hashmap is storing address + 4
				if(immediate % (0x10000) == 0) { 
					size_text++;
					pc += 4;
				} else {
					size_text += 2;
					pc += 8;
				}
			}
			else if(strcmp(opcode, "nor") == 0) size_text++, pc += 4;
			else if(strcmp(opcode, "or") == 0) size_text++, pc += 4;
			else if(strcmp(opcode, "ori") == 0) size_text++, pc += 4;
			else if(strcmp(opcode, "sltiu") == 0) size_text++, pc += 4;
			else if(strcmp(opcode, "sltu") == 0) size_text++, pc += 4;
			else if(strcmp(opcode, "sll") == 0) size_text++, pc += 4;
			else if(strcmp(opcode, "srl") == 0) size_text++, pc += 4;
			else if(strcmp(opcode, "sw") == 0) size_text++, pc += 4;
			else if(strcmp(opcode, "subu") == 0) size_text++, pc += 4;
			else {}
		}
		size_data = size_data*4;
		size_text = size_text*4;
		fclose(stdin);

		// reopen the file
		if(freopen(file, "r", stdin) == 0) {
			printf("File open Error!\n");
			exit(1);
		}

		// For output file write 
		// You can see your code's output in the sample_input/example#.o 
		// So you can check what is the difference between your output and the answer directly if you see that file
		// make test command will compare your output with the answer
		file[strlen(file)-1] ='o';
		freopen(file,"w", stdout);

		// first line for size of text section
		// printf("%d", size_text);
		Dec2Bin(size_text, num, 32);
		for(int i = 0; i < 32; i++) printf("%d", num[i]);

		// second line for size of data section
		// printf("%d", size_data);
		Dec2Bin(size_data, num, 32);
		for(int i = 0; i < 32; i++) printf("%d", num[i]);

		pc = 0x00000000;
		// read instructions
		while(scanf("%s", opcode) != EOF) {	
			if(strcmp(opcode, ".text") == 0) pc = 0x400000;
			// opcode rd rs rt -> opcode rs rt rd shamt funct / opcode rs rt immediate / opcode address
			if(strcmp(opcode, "addiu") == 0) {							// addiu $17, $17, 0x1
				scanf("%s", rt), scanf("%s", rs), scanf("%s", imm);
				Reg2Bin(rt), Reg2Bin(rs), Num2Bin(imm, num, 16);
				pc += 4;
				printf("001001");										// opcode addiu
				printf("%s", rs), printf("%s", rt);
				for(int i = 0; i < 16; i++) printf("%d", num[i]);
			} else if(strcmp(opcode, "addu") == 0) {					// addu $5, $5, $31
				scanf("%s", rd), scanf("%s", rs), scanf("%s", rt);
				Reg2Bin(rd), Reg2Bin(rs), Reg2Bin(rt);
				pc += 4;
				printf("000000");										// opcode addu
				printf("%s", rs), printf("%s", rt), printf("%s", rd);
				printf("00000"); printf("100001");
			} else if(strcmp(opcode, "and") == 0) {						// and $17 $17 $0
				scanf("%s", rd), scanf("%s", rs), scanf("%s", rt);
				Reg2Bin(rd), Reg2Bin(rs), Reg2Bin(rt);
				pc += 4;
				printf("000000");										// opcode and
				printf("%s", rs), printf("%s", rt), printf("%s", rd);
				printf("00000"); printf("100100");
			} else if(strcmp(opcode, "andi") == 0) {					// andi $14, $4, 100
				scanf("%s", rt), scanf("%s", rs), scanf("%s", imm);
				Reg2Bin(rt), Reg2Bin(rs), Num2Bin(imm, num, 16);
				pc += 4;
				printf("001100");										// opcode addiu
				printf("%s", rs), printf("%s", rt);
				for(int i = 0; i < 16; i++) printf("%d", num[i]);
			} else if(strcmp(opcode, "beq") == 0) {						// beq $10, $8, lab5
				scanf("%s", rs), scanf("%s", rt), scanf("%s", baddr);
				Reg2Bin(rt), Reg2Bin(rs);
				pc += 4;
				int *address = HashMap_getvalue(map, baddr);
				int immediate = *address;
				Dec2Bin((immediate - pc) >> 2, num, 16);
				printf("000100");										// opcode beq
				printf("%s", rs), printf("%s", rt);
				for(int i = 0; i < 16; i++) printf("%d", num[i]);
			} else if(strcmp(opcode, "bne") == 0) {
				scanf("%s", rs), scanf("%s", rt), scanf("%s", baddr);
				Reg2Bin(rt), Reg2Bin(rs);
				pc += 4;
				int *address = HashMap_getvalue(map, baddr);
				int immediate = *address;
				Dec2Bin((immediate - pc) >> 2, num, 16);
				printf("000101");										// opcode bne
				printf("%s", rs), printf("%s", rt);
				for(int i = 0; i < 16; i++) printf("%d", num[i]);
			} else if(strcmp(opcode, "j") == 0) {						// j lab1
				scanf("%s", jaddr);
				pc += 4;
				int *address = HashMap_getvalue(map, jaddr);
				int immediate = *address;								// hashmap is storing address + 4
				Dec2Bin(immediate >> 2, num, 26);
				printf("000010");										// opcode j -
				for(int i = 0; i < 26; i++) printf("%d", num[i]);
			} else if(strcmp(opcode, "jal") == 0) {						// jal lab3
				scanf("%s", jaddr);
				pc += 4;
				int *address = HashMap_getvalue(map, jaddr);
				int immediate = *address;
				Dec2Bin(immediate >> 2, num, 26);
				printf("000011");										// opcode jal
				for(int i = 0; i < 26; i++) printf("%d", num[i]);
			} else if(strcmp(opcode, "jr") == 0) {						// jr $31
				scanf("%s", rs);
				pc += 4;
				Reg2Bin(rs);
				printf("000000");										// opcode jr -
				printf("%s", rs);
				printf("000000000000000001000");						// R format, all 0 except rs and function code
			} else if(strcmp(opcode, "lui") == 0) {						// lui $17, 100
				scanf("%s", rt), scanf("%s", imm);
				pc += 4;
				Reg2Bin(rt);
				Num2Bin(imm, num, 16);
				printf("001111");										// opcode lui
				printf("00000");										// no rs
				printf("%s", rt);
				for(int i = 0; i < 16; i++) printf("%d", num[i]);
			} else if(strcmp(opcode, "lw") == 0) {						// lw $5, 0($3)
				scanf("%s", rt), scanf("%s", imm);						// imm(rs) form
				pc += 4;
				int left = 0, right = 0, i;
				for(i = 0; imm[i] != '\0'; i++) {						// divide imm into imm and rs
					if(imm[i] == '(') left = i;
					if(imm[i] == ')') right = i;
				}
				i = left + 1;
				for(int j = 0; ; j++) {
					rs[j] = imm[i++];
					if(i == right) {
						rs[j + 1] = '\0';
						break;
					}
				}
				imm[left] = '\0';
				Reg2Bin(rt), Reg2Bin(rs), Num2Bin(imm, num, 16);
				printf("100011");										// opcode lw
				printf("%s", rs), printf("%s", rt);
				for(i = 0; i < 16; i++) printf("%d", num[i]);
			} else if(strcmp(opcode, "la") == 0) {						// la $8, data1   la $9, data2
				scanf("%s", rt), scanf("%s", baddr);
				int *address = HashMap_getvalue(map, baddr);
				int immediate = *address;								// hashmap is storing address + 4
				if(immediate % (0x10000) == 0) {						// la = lui
					Reg2Bin(rt);
					immediate = immediate / (0x10000);					// data1 0x1000 0000
					Dec2Bin(immediate, num, 16);
					pc += 4;
					printf("001111");									// opcode lui
					printf("00000");									// no rs
					printf("%s", rt);
					for(int i = 0; i < 16; i++) printf("%d", num[i]);
				} else {												// la = lui + ori
					int leftover = immediate % (0x10000);
					Reg2Bin(rt);
					immediate = immediate / (0x10000);					// data1 0x1000 0000
					Dec2Bin(immediate, num, 16);
					pc += 4;
					printf("001111");									// opcode lui
					printf("00000");									// no rs
					printf("%s", rt);
					for(int i = 0; i < 16; i++) printf("%d", num[i]);
					Dec2Bin(leftover, num, 16);
					pc += 4;
					printf("001101");									// opcode ori
					printf("%s", rt), printf("%s", rt);
					for(int i = 0; i < 16; i++) printf("%d", num[i]);
				}
			} else if(strcmp(opcode, "nor") == 0) {						// nor $16, $17, $18
				scanf("%s", rd), scanf("%s", rs), scanf("%s", rt);
				Reg2Bin(rd), Reg2Bin(rs), Reg2Bin(rt);
				pc += 4;
				printf("000000");										// opcode nor
				printf("%s", rs), printf("%s", rt), printf("%s", rd);
				printf("00000"); printf("100111");						// shamt, function code
			} else if(strcmp(opcode, "or") == 0) {						// or $4, $3, $2
				scanf("%s", rd), scanf("%s", rs), scanf("%s", rt);
				Reg2Bin(rd), Reg2Bin(rs), Reg2Bin(rt);
				pc += 4;
				printf("000000");										// opcode or
				printf("%s", rs), printf("%s", rt), printf("%s", rd);
				printf("00000"); printf("100101");						// shamt, function code
			} else if(strcmp(opcode, "ori") == 0) {						// ori $16, $16, 0xf0f0
				scanf("%s", rt), scanf("%s", rs), scanf("%s", imm);
				Reg2Bin(rt), Reg2Bin(rs), Num2Bin(imm, num, 16);
				pc += 4;
				printf("001101");										// opcode ori
				printf("%s", rs), printf("%s", rt);
				for(int i = 0; i < 16; i++) printf("%d", num[i]);
			} else if(strcmp(opcode, "sltiu") == 0) {					// sltiu $9, $10, 100
				scanf("%s", rt), scanf("%s", rs), scanf("%s", imm);
				Reg2Bin(rt), Reg2Bin(rs), Num2Bin(imm, num, 16);
				pc += 4;
				printf("001011");										// opcode sltiu
				printf("%s", rs), printf("%s", rt);
				for(int i = 0; i < 16; i++) printf("%d", num[i]);
			} else if(strcmp(opcode, "sltu") == 0) {					// sltu $4, $2, $3
				scanf("%s", rd), scanf("%s", rs), scanf("%s", rt);
				Reg2Bin(rd), Reg2Bin(rs), Reg2Bin(rt);
				pc += 4;
				printf("000000");										// opcode sltu
				printf("%s", rs), printf("%s", rt), printf("%s", rd);
				printf("00000"); printf("101011");						// shamt, function code
			} else if(strcmp(opcode, "sll") == 0) {						// sll $18, $17, 1
				scanf("%s", rt), scanf("%s", rs), scanf("%s", shamt);
				Reg2Bin(rt), Reg2Bin(rs);
				Num2Bin(shamt, num, 5);
				pc += 4;
				printf("000000");										// opcode sll
				printf("00000");										// no rs
				printf("%s", rs), printf("%s", rt);
				for(int i = 0; i < 5; i++) printf("%d", num[i]);
				printf("000000");										// function code
			} else if(strcmp(opcode, "srl") == 0) {						// srl $17, $18, 1 
				scanf("%s", rt), scanf("%s", rs), scanf("%s", shamt);
				Reg2Bin(rt), Reg2Bin(rs);
				Num2Bin(shamt, num, 5);
				pc += 4;
				printf("000000");										// opcode sll
				printf("00000");										// no rs
				printf("%s", rs), printf("%s", rt);
				for(int i = 0; i < 5; i++) printf("%d", num[i]);
				printf("000010");										// function code
			} else if(strcmp(opcode, "sw") == 0) {						// sw $5, 16($3)
				scanf("%s", rt), scanf("%s", imm);						// imm(rs) form
				pc += 4;
				int left = 0, right = 0, i;
				for(i = 0; imm[i] != '\0'; i++) {						// divide imm into imm and rs
					if(imm[i] == '(') left = i;
					if(imm[i] == ')') right = i;
				}
				i = left + 1;
				for(int j = 0; ; j++) {
					rs[j] = imm[i++];
					if(i == right) {
						rs[j + 1] = '\0';
						break;
					}
				}
				imm[left] = '\0';
				Reg2Bin(rt), Reg2Bin(rs), Num2Bin(imm, num, 16);
				printf("101011");										// opcode sw
				printf("%s", rs), printf("%s", rt);
				for(i = 0; i < 16; i++) printf("%d", num[i]);
			} else if(strcmp(opcode, "subu") == 0) {					// subu	$8, $7, $2
				scanf("%s", rd), scanf("%s", rs), scanf("%s", rt);
				Reg2Bin(rd), Reg2Bin(rs), Reg2Bin(rt);
				pc += 4;
				printf("000000");										// opcode subu
				printf("%s", rs), printf("%s", rt), printf("%s", rd);
				printf("00000"); printf("100011");						// shamt, function code
			} else {}
		}
		for(int i = 0; i < word_count; i++) {
			Dec2Bin(word[i], num, 32);
			for(int j = 0; j < 32; j++) printf("%d", num[j]);
		}
	}
	return 0;
}

void Reg2Bin(char reg[6]) {
	for(int i = 0; i < 6; i++) {					// erase comma
		if(reg[i] == ',') reg[i] = '\0';
	}
	if(strcmp(reg, "$0") == 0) strcpy(reg, "00000");
	else if(strcmp(reg, "$1") == 0) strcpy(reg, "00001");
	else if(strcmp(reg, "$2") == 0) strcpy(reg, "00010");
	else if(strcmp(reg, "$3") == 0) strcpy(reg, "00011");
	else if(strcmp(reg, "$4") == 0) strcpy(reg, "00100");
	else if(strcmp(reg, "$5") == 0) strcpy(reg, "00101");
	else if(strcmp(reg, "$6") == 0) strcpy(reg, "00110");
	else if(strcmp(reg, "$7") == 0) strcpy(reg, "00111");
	else if(strcmp(reg, "$8") == 0) strcpy(reg, "01000");
	else if(strcmp(reg, "$9") == 0) strcpy(reg, "01001");
	else if(strcmp(reg, "$10") == 0) strcpy(reg, "01010");
	else if(strcmp(reg, "$11") == 0) strcpy(reg, "01011");
	else if(strcmp(reg, "$12") == 0) strcpy(reg, "01100");
	else if(strcmp(reg, "$13") == 0) strcpy(reg, "01101");
	else if(strcmp(reg, "$14") == 0) strcpy(reg, "01110");
	else if(strcmp(reg, "$15") == 0) strcpy(reg, "01111");
	else if(strcmp(reg, "$16") == 0) strcpy(reg, "10000");
	else if(strcmp(reg, "$17") == 0) strcpy(reg, "10001");
	else if(strcmp(reg, "$18") == 0) strcpy(reg, "10010");
	else if(strcmp(reg, "$19") == 0) strcpy(reg, "10011");
	else if(strcmp(reg, "$20") == 0) strcpy(reg, "10100");
	else if(strcmp(reg, "$21") == 0) strcpy(reg, "10101");
	else if(strcmp(reg, "$22") == 0) strcpy(reg, "10110");
	else if(strcmp(reg, "$23") == 0) strcpy(reg, "10111");
	else if(strcmp(reg, "$24") == 0) strcpy(reg, "11000");
	else if(strcmp(reg, "$25") == 0) strcpy(reg, "11001");
	else if(strcmp(reg, "$26") == 0) strcpy(reg, "11010");
	else if(strcmp(reg, "$27") == 0) strcpy(reg, "11011");
	else if(strcmp(reg, "$28") == 0) strcpy(reg, "11100");
	else if(strcmp(reg, "$29") == 0) strcpy(reg, "11101");
	else if(strcmp(reg, "$30") == 0) strcpy(reg, "11110");
	else if(strcmp(reg, "$31") == 0) strcpy(reg, "11111");
	else {
		printf("Error: There are only 32 registers.\n");
		exit(0);
	}
	return;
}

void Num2Bin(char num[16], int arr[32], int bits) {
	int val, idx = 0;
	// hexadecimal vs decimal
	if(strncmp(num, "0x", 2) == 0) {
		// hexadecimal to binary
		val = strtol(num, NULL, 16);
		Dec2Bin(val, arr, bits);
	} else {
		// string to int : num to val
		val = atoi(num);
		Dec2Bin(val, arr, bits);
		return;
	}
}

void Dec2Bin(int val, int arr[32], int bits) {
	int temp, minus = 0;;
	// if value is negative value, first calculate -val
	if(val < 0) {
		val = (-1)*val;
		minus = 1;
	}
	// decimal to binary
	for(int i = 0; i < bits; i++) {
		arr[i] = val % 2;
		val = val / 2;
	}
	// should reverse arr => 56 : 0001110000000000
	for(int i = 0; i < bits / 2; i++) {
		temp = arr[i];
		arr[i] = arr[bits - i - 1];
		arr[bits - i - 1] = temp;
	}
	// if value was negative
	if(minus == 1) {
		for(int i = 0; i < bits; i++) {
			if(arr[i] == 0) arr[i] = 1;
			else if(arr[i] == 1) arr[i] = 0;
			else {}
		}
		for(int i = bits - 1; i >= 0; i--) {
			if(arr[i] == 1) arr[i] = 0;
			else if(arr[i] == 0) {
				arr[i] = 1;
				break;
			}
		}
	}
	return;
}

struct HashMap *createMap(int size) {
    struct HashMap *h = (struct HashMap *)malloc(sizeof(struct HashMap));
    assert(size >= 0);
    h->size = size;
    h->map = (struct HashNode **)malloc(sizeof(struct HashNode *)*size);
    for(int i = 0; i < size; i++) h->map[i] = NULL;
    return h;
}

int HashCode(struct HashMap *h, char *key) {
    int value = 0;
	for(int i = 0; i < strlen(key); i++) value = (value << 8) + key[i];
	return value % (h->size);
}

void HashMap_add(struct HashMap *h, char *key, uint32_t *value) {
    int index = HashCode(h, key);
    struct HashNode *map = h->map[index];
    struct HashNode *new = (struct HashNode *)malloc(sizeof(struct HashNode));
    struct HashNode *pos = map;
    while(pos != NULL) {
        if(strcmp(pos->key, key) == 0) {
            pos->value = value;
            return;
        }
        pos = pos->next;
    }
    new->key = strdup(key), new->value = value, new->next = map;
    h->map[index] = new;
}

uint32_t *HashMap_getvalue(struct HashMap *h, char *key) {
	int index = HashCode(h, key);
	struct HashNode *map = h->map[index];
	while(strcmp(key, map->key) != 0 && map != NULL && map->key != NULL) map = map->next;
	if(strcmp(key, map->key) == 0) return map->value;
	else {
		printf("Error: Failed to get value from HashMap.\n");
		exit(0);
	}
}