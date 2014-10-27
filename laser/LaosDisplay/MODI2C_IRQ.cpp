#include "MODI2C.h"

//**********************************************************
//*********************IRQ FUNCTIONS************************
//**********************************************************


void MODI2C::runUserIRQ(I2CData Data) {
     
    if (IRQOp==IRQ_I2C_BOTH)        //Always call if both
        callback.call();
    if ((IRQOp==IRQ_I2C_READ)&&(Data.address&0x01)) //Call if read and byte was read
        callback.call();
    if ((IRQOp==IRQ_I2C_WRITE)&&(!(Data.address&0x01))) //Call if write and byte was written
        callback.call();
}

void MODI2C::IRQ1Handler( void ) {
    IRQHandler(&Buffer1, LPC_I2C1);
}

void MODI2C::IRQ2Handler( void ) {
    IRQHandler(&Buffer2, LPC_I2C2);
}

void MODI2C::IRQHandler( I2CBuffer *Buffer, LPC_I2C_TypeDef *I2CMODULE) {
    I2CData *Data = &Buffer->Data[0];

    //Depending on the status register it determines next action, see datasheet
    //This is also pretty much copy pasting the datasheet
    //General options
    switch (I2CMODULE->I2STAT) {
        case(0x08):
        case(0x10):
            //Send Address
            I2CMODULE->I2DAT = Data->address;
            I2CMODULE->I2CONCLR = 1<<I2C_FLAG | 1<<I2C_START;
            break;

            //All master TX options

            //Address + W has been sent, ACK received
            //Data has been sent, ACK received
        case(0x18):
        case(0x28):
            if (Buffer->count==Data->length) {
                *Data->status=I2CMODULE->I2STAT;
                if (!Data->repeated)
                    _stop(I2CMODULE);
                else {
                    I2CMODULE->I2CONSET = 1<<I2C_START;
                    I2CMODULE->I2CONCLR = 1<<I2C_FLAG;
                }
                bufferHandler(I2CMODULE);
            } else {
                I2CMODULE->I2DAT = Data->data[Buffer->count];
                I2CMODULE->I2CONSET = 1<<I2C_ASSERT_ACK;        //I dont see why I have to enable that bit, but datasheet says so
                I2CMODULE->I2CONCLR = 1<<I2C_FLAG;
                Buffer->count++;
            }
            break;

            //Address + W has been sent, NACK received
            //Data has been sent, NACK received
        case(0x20):
        case(0x30):
            *Data->status=I2CMODULE->I2STAT;
            _stop(I2CMODULE);
            bufferHandler(I2CMODULE);
            break;

            //Arbitration lost (situation looks pretty hopeless to me if you arrive here)
        case(0x38):
            _start(I2CMODULE);
            break;


            //All master RX options

            //Address + R has been sent, ACK received
        case(0x40):
            //If next byte is last one, NACK, otherwise ACK
            if (Data->length <= Buffer->count + 1)
                I2CMODULE->I2CONCLR = 1<<I2C_ASSERT_ACK;
            else
                I2CMODULE->I2CONSET = 1<<I2C_ASSERT_ACK;
            I2CMODULE->I2CONCLR = 1<<I2C_FLAG;
            break;

            //Address + R has been sent, NACK received
        case(0x48):
            *Data->status=I2CMODULE->I2STAT;
            _stop(I2CMODULE);
            bufferHandler(I2CMODULE);
            break;

            //Data was received, ACK returned
        case(0x50):
            //Read data
            Data->data[Buffer->count]=I2CMODULE->I2DAT;
            Buffer->count++;

            //If next byte is last one, NACK, otherwise ACK
            if (Data->length == Buffer->count + 1)
                I2CMODULE->I2CONCLR = 1<<I2C_ASSERT_ACK;
            else
                I2CMODULE->I2CONSET = 1<<I2C_ASSERT_ACK;

            I2CMODULE->I2CONCLR = 1<<I2C_FLAG;
            break;

            //Data was received, NACK returned (last byte)
        case(0x58):
            //Read data
            *Data->status=I2CMODULE->I2STAT;
            Data->data[Buffer->count]=I2CMODULE->I2DAT;
            if (!Data->repeated)
                _stop(I2CMODULE);
            else {
                I2CMODULE->I2CONSET = 1<<I2C_START;
                I2CMODULE->I2CONCLR = 1<<I2C_FLAG;
            }
            bufferHandler(I2CMODULE);
            break;

        default:
            *Data->status=I2CMODULE->I2STAT;
            bufferHandler(I2CMODULE);
            break;
    }
}


//**********************************************************
//*********************COMMAND BUFFER***********************
//**********************************************************

void MODI2C::bufferHandler(LPC_I2C_TypeDef *I2CMODULE) {
    I2CBuffer *Buffer;
    if (I2CMODULE == LPC_I2C1) {
        Buffer = &Buffer1;
    } else {
        Buffer = &Buffer2;
    }

    //Start user interrupt
    Buffer->Data[0].caller->runUserIRQ(Buffer->Data[0]);

    removeBuffer(I2CMODULE);



    if (Buffer->queue!=0)
        startBuffer(I2CMODULE);
    else
        _clearISR(I2CMODULE);
}

//Returns true if succeeded, false if buffer is full
bool MODI2C::addBuffer(I2CData Data, LPC_I2C_TypeDef *I2CMODULE) {
    I2CBuffer *Buffer;
    if (I2CMODULE == LPC_I2C1) {
        Buffer = &Buffer1;
    } else {
        Buffer = &Buffer2;
    }
    if (Buffer->queue<I2C_BUFFER) {
        
        if(Data.status == NULL) {
            Data.status = &defaultStatus;
            wait_us(1);     //I blame the compiler that this is needed
            }
        *Data.status = 0;
            
        Buffer->Data[Buffer->queue]=Data;
        Buffer->queue++;

        //If queue was empty, set I2C settings, start conversion
        if (Buffer->queue==1) {
            startBuffer(I2CMODULE);
        }

        return true;
    } else
        return false;

}

//Returns true if buffer still has data, false if empty
bool MODI2C::removeBuffer(LPC_I2C_TypeDef *I2CMODULE) {
    I2CBuffer *Buffer;
    if (I2CMODULE == LPC_I2C1) {
        Buffer = &Buffer1;
    } else {
        Buffer= &Buffer2;
    }

    if (Buffer->queue>0) {
        for (int i =0; i<Buffer->queue-1; i++)
            Buffer->Data[i]=Buffer->Data[i+1];
        Buffer->queue--;
    }
    if (Buffer->queue>0)
        return true;
    else
        return false;
}

//Starts new conversion
void MODI2C::startBuffer(LPC_I2C_TypeDef *I2CMODULE) {
    I2CBuffer *Buffer;
    if (I2CMODULE == LPC_I2C1) {
        Buffer = &Buffer1;
    } else {
        Buffer = &Buffer2;
    }

    //Write settings
    Buffer->Data[0].caller->writeSettings();
    Buffer->count=0;

    //Send Start
    _start(I2CMODULE);

    //Start ISR (when buffer wasnt empty this wont do anything)
    _setISR(I2CMODULE);

}