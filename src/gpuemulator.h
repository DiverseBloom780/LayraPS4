#ifndef GPU_EMULATOR_H
#define GPU_EMULATOR_H
class GPUEmulator {
public:
    void init();
    void renderFrame();
    void update();
private:
    GPUState* gpu_state;
};

#endif  // GPU_EMULATOR_H
