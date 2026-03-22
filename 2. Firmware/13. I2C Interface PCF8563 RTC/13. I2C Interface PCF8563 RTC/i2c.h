#ifndef I2C_H_
#define I2C_H_

typedef uint16_t i2c_address_t;

typedef enum
{
	I2C_IDLE,
	I2C_SEND_ADDR_WRITE,
	I2C_SEND_ADDR_READ,
	I2C_TX,
	I2C_RX,
	I2C_TX_COMPLETE,
	I2C_STOP,
	
	I2C_NACK_STOP,
	I2C_NACK_RESTART_WRITE,
	I2C_NACK_RESTART_READ,
	I2C_ADDRESS_NACK,
	I2C_BUS_COLLISION,
	I2C_BUS_ERROR,
	I2C_RESET
}i2c_handler_t;

typedef enum
{
	I2C_OPERATION_STOP,
	I2C_RESTART_WRITE,
	I2C_RESTART_READ,
	I2C_CONTINUE
}i2c_operation_t;

typedef enum
{
	I2C_ERROR,
	I2C_NOERROR,
	I2C_BUSY
}i2c_error_t;

typedef enum
{
	M_WRITE,
	M_READ
}i2c_m_read_write_t;

typedef i2c_handler_t (*stateHandler)(void);
typedef i2c_operation_t (*i2c_callback_t)(void* p);

typedef struct
{
	uint8_t busy : 1;
	uint8_t inUse : 1;
	uint8_t bufferFree : 1;
	uint8_t addressNACKCheck : 1;
	
	i2c_address_t SlaveAddress;
	uint8_t* data_ptr;
	uint16_t data_length;
	i2c_handler_t handler;
	i2c_error_t error;
	
	i2c_callback_t callbackDataXferComplete;
	void* callbackDataParameter;
}i2c_info_t;

typedef struct
{
	uint8_t* buff;
	uint16_t len;
}i2c_block_t;


void I2C_Init(void);

void I2C_Write_Command(i2c_address_t slaveAddress, uint8_t cmd);

void I2C_Write_Cmd_Uint8( i2c_address_t slaveAddress, uint8_t cmd, uint8_t data );
void I2C_Write_Cmd_Uint16( i2c_address_t slaveAddress, uint8_t cmd, uint16_t data );
void I2C_Write_Block( i2c_address_t slaveAddress, uint8_t *buffer, uint8_t length );

uint8_t I2C_Read_Cmd_Uint8( i2c_address_t slaveAddress, uint8_t cmd );
uint16_t I2C_Read_Cmd_Uint16(i2c_address_t slaveAddress, uint8_t cmd);
void I2C_Read_Cmd_Block( i2c_address_t slaveAddress, uint8_t cmd, uint8_t *buffer, uint8_t length );

void I2C_Write_Address_Uint8( i2c_address_t slaveAddress, uint16_t address, uint8_t data );
void I2C_Write_Address_Uint16( i2c_address_t slaveAddress, uint16_t address, uint16_t data );
void I2C_Write_Address_Block( i2c_address_t slaveAddress, uint16_t address, uint8_t *buffer, uint16_t length );

uint8_t I2C_Read_Address_Uint8( i2c_address_t slaveAddress, uint16_t address );
uint16_t I2C_Read_Address_Uint16( i2c_address_t slaveAddress, uint16_t address );
void I2C_Read_Address_Block( i2c_address_t slaveAddress, uint16_t address, uint8_t *buffer, uint16_t length );

#endif /* I2C_H_ */