#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#define MAX 0x100
#define REGMAX 4
#define BYTE uint16_t
#define WORD uint32_t

#define print(...) printf(__VA_ARGS__)
enum OPCODES {
  NOP = 0x00,
  START,  // Defines the start module
  HALT,   // Defines a halt - end of program
  MOVE,   // Moves a value to a register [MOVE AX 1]
  ADD,    // Adds a value to a register  [ADD AX BX]
  MODULE, // Defines a module [MODULE, 0x01 ... MODULE]
  RUN,    // Runs a module [RUN, 0x01]
  RET,    // Returns from a module w/data [RET, ]
  CID,    // Caller ID - used for returning to the previous module when done
  LD,     // Loads a value in to a register from memory [LD AX 0x01]
  LDC,    // Loads a value in to a register from module cache [LDC AX 0x01]
  OUT,    // Outputs a value to the screen [OUT AX]
};

enum REGS {
  AX = 0x01,
  BX = 0x02,
  CX = 0x03,
  DX = 0x04,
};

struct RAM {
  BYTE data[MAX];
};

struct MODULE {
  WORD cache[REGMAX];
  int occupied : 1;
  int entry : 1;
  WORD cid, id, entryLoc;
  struct RAM ram;
};
void initRAM(struct RAM *ram) {
  for (int i = 0; i < MAX; i++) {
    ram->data[i] = 0x00;
  }
}

struct CPU {
  WORD pc;
  WORD sp;
  BYTE reg[REGMAX];
  struct RAM ram;
  ssize_t moduleArrayIndex;
  struct MODULE modules[MAX];
  struct MODULE *current;
};

void printRAM(struct RAM *ram) {
  for (int i = 0; i < MAX; i++) {
    if (ram->data[i] == NOP) {
      continue;
    }
    printf("0x%02x ", ram->data[i]);
  }
  printf("\n");
}

void printCPU(struct CPU *cpu) {
  printf("PC: 0x%04x\n", cpu->pc);
  printf("SP: 0x%04x\n", cpu->sp);
  for (int i = 0; i < REGMAX; i++) {
    printf("Reg[%d]: 0x%02x\n", i, cpu->reg[i]);
  }
  printf("\n");
}

void resetCPU(struct CPU *cpu) {
  cpu->pc = 0x00;
  cpu->sp = 0x00;
  for (int i = 0; i < REGMAX; i++) {
    cpu->reg[i] = 0x00;
  }
}

void resetRegisters(struct CPU *cpu) {
  for (int i = 0; i < REGMAX; i++) {
    cpu->reg[i] = 0x00;
  }
}

void loadProgram(struct CPU *cpu, const BYTE *program) {
  for (int i = 0; i < MAX; i++) {
    cpu->ram.data[i] = program[i];
  }
}
void loadModulesFromProgram(struct CPU *cpu) {
  for (int i = 0; i < MAX; i++) {
    if (cpu->ram.data[i] == MODULE) {
      int moduleId = cpu->ram.data[i + 1];
      printf("Module found at 0x%04x with id %d\n", i, moduleId);

      cpu->moduleArrayIndex++;
      struct MODULE *currentModule = &cpu->modules[moduleId];
      currentModule->occupied++;
      for (int instructionIndex = i + 2; instructionIndex < MAX;
           instructionIndex++) {
        uint8_t instruction = cpu->ram.data[instructionIndex];
        if (instruction == RET) {
          break; // End of module
        }
        currentModule->ram.data[instructionIndex] =
            instruction; // Store the instruction
        printf("Stored instruction (0x%02x) at 0x%04x\n", instruction,
               instructionIndex);
        currentModule->id = moduleId;
      }
    }
  }
}

void initModules(struct CPU *cpu) {
  for (int i = 0; i < MAX; i++) {
    initRAM(&cpu->modules[i].ram);
    cpu->modules[i].occupied = 0;
    cpu->modules[i].entry = 0;
    cpu->modules[i].cid = 0;
  }
}

void printModules(struct CPU *cpu) {
  for (int i = 0; i < MAX; i++) {
    if (cpu->modules[i].occupied == 0) {
      continue;
    }
    printf("Module with an id %d\n", i);
    printRAM(&cpu->modules[i].ram);
  }
}

BYTE fetch(struct CPU *cpu) { return cpu->ram.data[cpu->pc++]; }

void endProgram() { exit(0); }

void executeInstructions(struct CPU *cpu) {
  while (cpu->pc < MAX) {
    BYTE opcode = fetch(cpu);
    switch (opcode) {
    case NOP:
      break;
    case START: {
      BYTE id = fetch(cpu);
      struct MODULE *module = &cpu->modules[id];
      cpu->current = module;
      for (int i = 0; i < REGMAX; i++) {
        cpu->current->cache[i] = cpu->reg[i];
      }
      cpu->current->entry ^= 1;
      cpu->current->entryLoc = cpu->pc;
      resetRegisters(cpu);
      executeInstructions(cpu);

    } break;
    case HALT:
      return;
    case MOVE: {
      BYTE reg = fetch(cpu);
      BYTE value = fetch(cpu);
      cpu->reg[reg] = value;
    } break;
    case ADD: {
      BYTE reg = fetch(cpu);
      BYTE value = fetch(cpu);
      printf("Add at 0x%02x { 0x%02x + 0x%02x }\n", cpu->pc, cpu->reg[reg],
             cpu->reg[value]);
      cpu->reg[reg] += cpu->reg[value];
    } break;
    case MODULE:
      break;
    case RUN:
      break;
    case RET:
      printCPU(cpu);
      /* cpu->pc = cpu->current->entryLoc; */
      BYTE cid = cpu->current->cid;
      if (cid == -1)
        endProgram();
      cpu->current = &cpu->modules[cid];
      executeInstructions(cpu);

      break;
    case CID:
      break;
    case LD:
      break;
    case LDC:
      break;
    case OUT:
      break;
    }
  }
}

void run(struct CPU *cpu) {
  initModules(cpu);
  loadModulesFromProgram(cpu);
  executeInstructions(cpu);
}

int main(void) {
  // Program
  BYTE program[MAX] = {
      NOP,    MOVE, AX,   3,

      START,  0x00,

      MODULE, 0x00, MOVE, AX, 2, MOVE, BX, 3, ADD, AX, BX, START, 0x01, RET,

      MODULE, 0x01, MOVE, AX, 4, MOVE, BX, 9, ADD, AX, BX, RET

  };
  struct CPU cpu;
  initRAM(&cpu.ram);
  loadProgram(&cpu, program);
  run(&cpu);
}
