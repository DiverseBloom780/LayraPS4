#ifndef CPU_EMULATOR_H
#define CPU_EMULATOR_H
class CPUEmulator {
public:
    void init();
    void executeInstruction(uint32_t instruction);
    void update();
private:
    CPUState* cpu_state;
};

#endif  // CPU_EMULATOR_H
