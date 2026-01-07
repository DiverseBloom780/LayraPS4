#ifndef IO_EMULATOR_H
#define IO_EMULATOR_H
class IOEmulator {
public:
    void init();
    void handleControllerInput();
    void update();
private:
    IOState* io_state;
};

#endif  // IO_EMULATOR_H
