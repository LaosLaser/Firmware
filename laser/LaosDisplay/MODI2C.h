/* 

                            .       .
                           / `.   .' \
                   .---.  <    > <    >  .---.
                   |    \  \ - ~ ~ - /  /    |
                    ~-..-~             ~-..-~
                \~~~\.'                    `./~~~/
                 \__/                        \__/
                  /                  .-    .  \
           _._ _.-    .-~ ~-.       /       }   \/~~~/
       _.-'q  }~     /       }     {        ;    \__/
      {'__,  /      (       /      {       /      `. ,~~|   .     .
       `''''='~~-.__(      /_      |      /- _      `..-'   \\   //
                   / \   =/  ~~--~~{    ./|    ~-.     `-..__\\_//_.-'
                  {   \  +\         \  =\ (        ~ - . _ _ _..---~
                  |  | {   }         \   \_\
                 '---.o___,'       .o___,'     "Stegosaurus"


Face it, dinos much cooler than copyright notices.*/

#include "mbed.h"


#ifndef MODI2C_H
#define MODI2C_H


#define I2C_ENABLE          6
#define I2C_START           5
#define I2C_STOP            4
#define I2C_FLAG            3
#define I2C_ASSERT_ACK      2

#define IRQ_I2C_BOTH        0
#define IRQ_I2C_READ        1
#define IRQ_I2C_WRITE       2



#ifndef I2C_BUFFER
#define I2C_BUFFER          10
#endif


/** Library that allows interrupt driven communication with I2C devices
  *
  * For now this is all in beta, so if you got weird results while the mbed I2C library works, it is probably my fault.
  * Similar to googles definition of beta, it is unlikely it will ever come out of beta.
  *
  * Example:
  * @code
  * #include "mbed.h"
  * #include "MODI2C.h"
  * #include "MPU6050.h"
  * 
  * Serial pc(USBTX, USBRX); // tx, rx
  * MODI2C mod(p9, p10);
  * 
  * int main() {
  *     char registerAdd = MPU6050_WHO_AM_I_REG;
  *     char registerResult;
  *     int status;
  * 
  *     while (1) {
  *         mod.write(MPU6050_ADDRESS*2, &registerAdd, 1, true);
  *         mod.read_nb(MPU6050_ADDRESS*2, &registerResult, 1, &status);
  * 
  *         while (!status) wait_us(1);
  *         pc.printf("Register holds 0x%02X\n\r", registerResult);
  *         wait(2);
  *     }
  * }
  * @endcode
  */
class MODI2C {
public:
    /**
    * Constructor.
    *
    * @param sda - mbed pin to use for the SDA I2C line.
    * @param scl - mbed pin to use for the SCL I2C line.
    */
    MODI2C(PinName sda, PinName scl);

    /**
    * Write data on the I2C bus.
    *
    * This function should generally be a drop-in replacement for the standard mbed write function.
    * However this function is completely non-blocking, so it barely takes time. This can cause different behavior.
    * Everything else is similar to read_nb.
    *
    * @param address - I2C address of the slave (7 bit address << 1).
    * @param data - pointer to byte array that holds the data
    * @param length - amount of bytes that need to be sent
    * @param repeated - determines if it should end with a stop condition (default false)
    * @param status - (optional) pointer to integer where the final status code of the I2C transmission is placed. (0x28 is success)
    * @param return - returns zero
    */
    int write(int address, char *data, int length, bool repeated = false, int *status = NULL);
    int write(int address, char *data, int length, int *status);

    /**
    * Read data non-blocking from the I2C bus.
    *
    * Reads data from the I2C bus, completely non-blocking. Aditionally it will always return zero, since it does
    * not wait to see how the transfer goes. A pointer to an integer can be added as argument which returns the status.
    *
    * @param address - I2C address of the slave (7 bit address << 1).
    * @param data - pointer to byte array where the data will be stored
    * @param length - amount of bytes that need to be received
    * @param repeated - determines if it should end with a stop condition
    * @param status - (optional) pointer to integer where the final status code of the I2C transmission is placed. (0x58 is success)
    * @param return - returns zero
    */
    int read_nb(int address, char *data, int length, bool repeated = false, int *status=NULL);
    int read_nb(int address, char *data, int length, int *status);
    
    /**
    * Read data from the I2C bus.
    *
    * This function should should be a drop-in replacement for the standard mbed function.
    *
    * @param address - I2C address of the slave (7 bit address << 1).
    * @param data - pointer to byte array where the data will be stored
    * @param length - amount of bytes that need to be received
    * @param repeated - determines if it should end with a stop condition
    * @param return - returns zero on success, LPC status code on failure
    */
    int read(int address, char *data, int length, bool repeated = false);

    /**
    * Sets the I2C bus frequency
    *
    * @param hz - the bus frequency in herz
    */
    void frequency(int hz);

    /**
    * Creates a start condition on the I2C bus
    *
    * If you use this function you probably break something (but mbed also had it public)
    */
    void start ( void );

    /**
    * Creates a stop condition on the I2C bus
    *
    * If you use this function you probably break something (but mbed also had it public)
    */
    void stop ( void );
    
    /**
    * Removes attached function
    */
    void detach( void );
    
    /**
    * Calls user function when I2C command is finished
    *
    * @param function - the function to call.
    * @param operation - when to call IRQ: IRQ_I2C_BOTH (default) - IRQ_I2C_READ - IRQ_I2C_WRITE
    */
    void attach(void (*function)(void), int operation = IRQ_I2C_BOTH);
    
    /**
    * Calls user function when I2C command is finished
    *
    * @param object - the object to call the function on.
    * @param member - the function to call
    * @param operation - when to call IRQ: IRQ_I2C_BOTH (default) - IRQ_I2C_READ - IRQ_I2C_WRITE
    */
    template<typename T>
        void attach(T *object, void (T::*member)(void), int operation = IRQ_I2C_BOTH);
        
    /**
    * Returns the current number of commands in the queue (including one currently being processed)
    *
    * Note that this is the number of commands, not the number of bytes
    *
    * @param return - number of commands in queue
    */
    int getQueue( void );


private:

    struct I2CData {
        MODI2C *caller;
        char address;
        char *data;
        int  length;
        bool repeated;
        int *status;
    };
    
    struct I2CBuffer {
        int queue;
        int count;
        I2CData Data[I2C_BUFFER];
        };
        
    //Settings:
    int duty;
    
    FunctionPointer callback;
    int IRQOp;   
    void runUserIRQ( I2CData Data );

    //Remove later:
    LPC_I2C_TypeDef *I2CMODULE;

    

    //Whole bunch of static stuff, pretty much everything that is ever called from ISR
    static I2CBuffer Buffer1;
    static I2CBuffer Buffer2;
    
    static void IRQHandler(I2CBuffer *Buffer, LPC_I2C_TypeDef *I2CMODULE);
    static void IRQ1Handler(void);
    static void IRQ2Handler(void);
    
    static void bufferHandler(LPC_I2C_TypeDef *I2CMODULE);
    static bool addBuffer(I2CData Data, LPC_I2C_TypeDef *I2CMODULE);
    static bool removeBuffer(LPC_I2C_TypeDef *I2CMODULE);
    static void startBuffer(LPC_I2C_TypeDef *I2CMODULE);

    static int defaultStatus;
    
    static void _start(LPC_I2C_TypeDef *I2CMODULE);
    static void _stop(LPC_I2C_TypeDef *I2CMODULE);
    static void _clearISR( LPC_I2C_TypeDef *I2CMODULE );
    static void _setISR( LPC_I2C_TypeDef *I2CMODULE );


    void writeSettings( void );
    void writePinState( void );
    void setISR( void );
    void clearISR( void );


    DigitalOut led;

};


#endif