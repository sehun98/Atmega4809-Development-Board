#define F_CPU 5000000UL
#include <avr/io.h>
#include <avr/interrupt.h>
#include <stdio.h>
#include <stdbool.h>
#include "i2c.h"

i2c_info_t i2c_info;

i2c_handler_t I2C_HANDLER_IDLE(void)
{
	i2c_info.error = I2C_NOERROR;
	i2c_info.busy = false;
	return I2C_IDLE;
}

i2c_handler_t I2C_HANDLER_STOP(void)
{
	TWI0.MCTRLB |= TWI_MCMD_STOP_gc;
	return I2C_HANDLER_IDLE();
}

i2c_handler_t I2C_HANDLER_NACK_STOP(void)
{
	TWI0.MCTRLB |= TWI_ACKACT_bm;
	TWI0.MCTRLB |= TWI_MCMD_STOP_gc;
	return I2C_HANDLER_IDLE();
}

i2c_handler_t I2C_HANDLER_RESET( void )
{
	i2c_info.busy = false;
	return I2C_RESET;
}

i2c_handler_t I2C_HANDLER_NACK_RESTART_WRITE(void)
{
	i2c_info.addressNACKCheck = true;
	TWI0.MCTRLB |= TWI_ACKACT_bm;
	TWI0.MCTRLB |= TWI_MCMD_REPSTART_gc;
	TWI0.MADDR = i2c_info.SlaveAddress << 1;
	return I2C_TX;
}

i2c_handler_t I2C_HANDLER_NACK_RESTART_READ(void)
{
	i2c_info.addressNACKCheck = true;
	TWI0.MCTRLB |= TWI_ACKACT_bm;
	TWI0.MCTRLB |= TWI_MCMD_REPSTART_gc;
	TWI0.MADDR = i2c_info.SlaveAddress << 1 | 1;
	return I2C_RX;
}


i2c_handler_t I2C_HANDLER_SEND_ADDR_WRITE(void)
{
	i2c_info.addressNACKCheck = true;
	TWI0.MADDR = i2c_info.SlaveAddress << 1;
	return I2C_TX;
}

i2c_handler_t I2C_HANDLER_SEND_ADDR_READ(void)
{
	i2c_info.addressNACKCheck = true;
	TWI0.MADDR = i2c_info.SlaveAddress << 1 | 1;
	return I2C_RX;
}

i2c_handler_t I2C_HANDLER_TX(void)
{
	if ( TWI0.MSTATUS & TWI_RXACK_bm )
	return I2C_HANDLER_STOP();
	else {
		i2c_info.addressNACKCheck = false;
		TWI0.MDATA = *i2c_info.data_ptr++;
		return ( --i2c_info.data_length )? I2C_TX : I2C_TX_COMPLETE;
	}
}

i2c_handler_t I2C_HANDLER_RX(void)
{
	i2c_info.addressNACKCheck = false;
	
	*i2c_info.data_ptr++ = TWI0.MDATA;
	if ( --i2c_info.data_length )
	{
		TWI0.MCTRLB &= ~TWI_ACKACT_bm;
		TWI0.MCTRLB |= TWI_MCMD_RECVTRANS_gc;
		return I2C_RX;
	}
	else
	{
		i2c_info.bufferFree = true;
		switch ( i2c_info.callbackDataXferComplete(i2c_info.callbackDataParameter) )
		{
			case I2C_RESTART_READ :
			return I2C_HANDLER_NACK_RESTART_READ();
			case I2C_RESTART_WRITE :
			return I2C_HANDLER_NACK_RESTART_WRITE();
			case I2C_CONTINUE :
			case I2C_OPERATION_STOP :
			default:
			return I2C_HANDLER_NACK_STOP();
		}
	}
}

i2c_handler_t I2C_HANDLER_TX_COMPLETE(void)
{
	if ( TWI0.MSTATUS & TWI_RXACK_bm )
	return I2C_HANDLER_STOP();
	else
	{
		i2c_info.bufferFree = true;
		switch ( i2c_info.callbackDataXferComplete(i2c_info.callbackDataParameter) )
		{
			case I2C_RESTART_READ :
			return I2C_HANDLER_NACK_RESTART_READ();
			case I2C_RESTART_WRITE :
			return I2C_HANDLER_NACK_RESTART_WRITE();
			case I2C_CONTINUE :
			return I2C_HANDLER_TX();
			case I2C_OPERATION_STOP :
			default:
			return I2C_HANDLER_STOP();
		}
	}
}

i2c_handler_t I2C_HANDLER_ADDRESS_NACK(void)
{
	TWI0.MSTATUS |= TWI_ARBLOST_bm;
	i2c_info.error = I2C_ERROR;
	return I2C_HANDLER_RESET();
}

i2c_handler_t I2C_HANDLER_BUS_COLLISION( void )
{
	TWI0.MSTATUS |= TWI_ARBLOST_bm;
	i2c_info.error = I2C_ERROR;
	return I2C_HANDLER_RESET();
}

i2c_handler_t I2C_HANDLER_BUS_ERROR( void )
{
	TWI0.MSTATUS |= TWI_BUSERR_bm | TWI_BUSSTATE_IDLE_gc;
	TWI0.MCTRLB |= TWI_FLUSH_bm;
	
	i2c_info.error = I2C_ERROR;
	return I2C_HANDLER_RESET();
}

stateHandler stateHandlerTable[] = {
	I2C_HANDLER_IDLE,
	I2C_HANDLER_SEND_ADDR_WRITE,
	I2C_HANDLER_SEND_ADDR_READ,
	I2C_HANDLER_TX,
	I2C_HANDLER_RX,
	I2C_HANDLER_TX_COMPLETE,
	I2C_HANDLER_STOP,
	
	I2C_HANDLER_NACK_STOP,
	I2C_HANDLER_NACK_RESTART_WRITE,
	I2C_HANDLER_NACK_RESTART_READ,
	I2C_HANDLER_ADDRESS_NACK,
	I2C_HANDLER_BUS_COLLISION,
	I2C_HANDLER_BUS_ERROR,
	I2C_HANDLER_RESET
};

ISR( TWI0_TWIM_vect )
{
	TWI0.MSTATUS |= TWI_RIF_bm | TWI_WIF_bm;
	
	if ( i2c_info.addressNACKCheck && (TWI0.MSTATUS & TWI_RXACK_bm) )
	i2c_info.handler = I2C_ADDRESS_NACK;
	if ( TWI0.MSTATUS & TWI_ARBLOST_bm )
	i2c_info.handler = I2C_BUS_COLLISION;
	if ( TWI0.MSTATUS &  TWI_BUSERR_bm )
	i2c_info.handler = I2C_BUS_ERROR;
	
	i2c_info.handler = stateHandlerTable[i2c_info.handler]();
}

i2c_operation_t I2C_Stop_CB(void* p)
{
	return I2C_OPERATION_STOP;
}

i2c_error_t I2C_OPEN(i2c_address_t address)
{
	i2c_error_t ret = I2C_BUSY;
	
	if(!i2c_info.inUse)
	{
		i2c_info.SlaveAddress = address;
		
		i2c_info.busy = false;
		i2c_info.inUse = true;
		i2c_info.bufferFree = true;
		i2c_info.addressNACKCheck = false;
		
		i2c_info.handler = I2C_IDLE;
		
		i2c_info.callbackDataXferComplete = I2C_Stop_CB;
		i2c_info.callbackDataParameter = NULL;
		
		TWI0.MCTRLB		|= TWI_FLUSH_bm;
		TWI0.MSTATUS	|= TWI_BUSSTATE_IDLE_gc;
		TWI0.MSTATUS    |= TWI_RIF_bm | TWI_WIF_bm;
		TWI0.MCTRLA		|= TWI_WIEN_bm | TWI_RIEN_bm;
		
		ret = I2C_NOERROR;
	}
	
	return ret;
}

i2c_error_t I2C_CLOSE(void)
{
	i2c_error_t ret = I2C_BUSY;
	
	if(!i2c_info.busy)
	{
		i2c_info.inUse = false;
		i2c_info.SlaveAddress = 0xff;
		
		TWI0.MCTRLA &= ~(TWI_WIEN_bm | TWI_RIEN_bm);
		ret = i2c_info.error;
	}
	return ret;
}

i2c_error_t I2C_M_READ_WRITE(i2c_m_read_write_t m_rw)
{
	i2c_error_t ret = I2C_BUSY;
	
	if(!i2c_info.busy)
	{
		i2c_info.busy = true;
		ret = I2C_NOERROR;
		
		i2c_info.handler = (m_rw == M_READ) ? I2C_SEND_ADDR_READ : I2C_SEND_ADDR_WRITE;
		i2c_info.handler = stateHandlerTable[i2c_info.handler]();
	}
	return ret;
}

void I2C_SetDataBuffer(uint8_t* buffer, uint16_t length)
{
	if(i2c_info.bufferFree)
	{
		i2c_info.data_ptr = buffer;
		i2c_info.data_length = length;
		i2c_info.bufferFree = false;
	}
}

void I2C_SetDataXferCompleteCallback(i2c_callback_t cb, void* p)
{
	if(cb)
	{
		i2c_info.callbackDataXferComplete = cb;
		i2c_info.callbackDataParameter = p;
	}
	else
	{
		i2c_info.callbackDataXferComplete = I2C_Stop_CB;
		i2c_info.callbackDataParameter = NULL;
	}
}

void I2C_Init(void)
{
	PORTA.DIRSET = PIN2_bm | PIN3_bm;
	PORTA.OUTSET = PIN2_bm | PIN3_bm;
	
	TWI0.MBAUD = 20;
	TWI0.MCTRLA |= TWI_ENABLE_bm;
}

void I2C_Write_Command(i2c_address_t slaveAddress, uint8_t cmd)
{
	while(I2C_OPEN(slaveAddress) == I2C_BUSY);
	I2C_SetDataBuffer(&cmd, (uint16_t)sizeof(cmd));
	I2C_M_READ_WRITE(M_WRITE);
	while(I2C_CLOSE() == I2C_BUSY);
}

i2c_operation_t I2C_Write_Cmd_Uint8_CB( void *p )
{
	I2C_SetDataBuffer( p, 1 );
	I2C_SetDataXferCompleteCallback( I2C_Stop_CB, NULL );
	return I2C_CONTINUE;
}

void I2C_Write_Cmd_Uint8( i2c_address_t slaveAddress, uint8_t cmd, uint8_t data )
{
	while ( I2C_OPEN(slaveAddress) == I2C_BUSY ) ;
	I2C_SetDataXferCompleteCallback( I2C_Write_Cmd_Uint8_CB, &data );
	I2C_SetDataBuffer( &cmd, (uint16_t)sizeof(cmd) );
	I2C_M_READ_WRITE(M_WRITE);
	while ( I2C_CLOSE() == I2C_BUSY ) ;
}


i2c_operation_t I2C_Write_Cmd_Uint16_CB( void *p )
{
	I2C_SetDataBuffer( p, 2 );
	I2C_SetDataXferCompleteCallback( I2C_Stop_CB, NULL );
	return I2C_CONTINUE;
}

void I2C_Write_Cmd_Uint16( i2c_address_t slaveAddress, uint8_t cmd, uint16_t data )
{
	while ( I2C_OPEN( slaveAddress ) == I2C_BUSY ) ;
	data =  data << 8 | data >> 8;
	I2C_SetDataXferCompleteCallback( I2C_Write_Cmd_Uint16, &data );
	I2C_SetDataBuffer( &cmd, (uint16_t)sizeof(cmd) );
	I2C_M_READ_WRITE( M_WRITE );
	while ( I2C_CLOSE() == I2C_BUSY ) ;
}

void I2C_Write_Block( i2c_address_t slaveAddress, uint8_t *buffer, uint8_t length )
{
	while ( I2C_OPEN( slaveAddress ) == I2C_BUSY ) ;
	I2C_SetDataBuffer( buffer, (uint16_t)length );
	I2C_M_READ_WRITE( M_WRITE );
	while ( I2C_CLOSE() == I2C_BUSY ) ;
}

i2c_operation_t I2C_Read_Cmd_Uint8_CB( void *p )
{
	I2C_SetDataBuffer( p, 1 );
	I2C_SetDataXferCompleteCallback( I2C_Stop_CB, NULL );
	return I2C_RESTART_READ;
}

uint8_t I2C_Read_Cmd_Uint8( i2c_address_t slaveAddress, uint8_t cmd )
{
	uint8_t	data;
	
	while ( I2C_OPEN( slaveAddress ) == I2C_BUSY ) ;
	I2C_SetDataXferCompleteCallback( I2C_Read_Cmd_Uint8_CB, &data );
	I2C_SetDataBuffer( &cmd, (uint16_t)sizeof(cmd) );
	I2C_M_READ_WRITE( M_WRITE );
	while ( I2C_CLOSE() == I2C_BUSY ) ;
	
	return data;
}

i2c_operation_t I2C_Read_Cmd_Uint16_CB( void *p )
{
	I2C_SetDataBuffer( p, 2 );
	I2C_SetDataXferCompleteCallback( I2C_Stop_CB, NULL );
	return I2C_RESTART_READ;
}

uint16_t I2C_Read_Cmd_Uint16(i2c_address_t slaveAddress, uint8_t cmd)
{
	uint16_t data;
	while(I2C_OPEN(slaveAddress) == I2C_BUSY);
	I2C_SetDataXferCompleteCallback( I2C_Read_Cmd_Uint16_CB, &data );
	I2C_SetDataBuffer( &cmd, (uint16_t)sizeof(cmd) );
	I2C_M_READ_WRITE( M_WRITE );
	while ( I2C_CLOSE() == I2C_BUSY ) ;
	
	return ( data << 8 | data >> 8 );
}

i2c_operation_t I2C_Read_Cmd_Block_CB( void *p )
{
	i2c_block_t *pp = (i2c_block_t *)p;
	
	I2C_SetDataBuffer( pp->buff, pp->len );
	I2C_SetDataXferCompleteCallback( I2C_Stop_CB, NULL );
	
	return I2C_RESTART_READ;
}

void I2C_Read_Cmd_Block( i2c_address_t slaveAddress, uint8_t cmd, uint8_t *buffer, uint8_t length )
{
	i2c_block_t block;
	
	block.buff = buffer;
	block.len = (uint16_t)length;
	
	while ( I2C_OPEN( slaveAddress ) == I2C_BUSY ) ;
	I2C_SetDataXferCompleteCallback( I2C_Read_Cmd_Block_CB, &block );
	I2C_SetDataBuffer( &cmd, (uint16_t)sizeof(cmd) );
	I2C_M_READ_WRITE( M_WRITE );
	while ( I2C_CLOSE() == I2C_BUSY ) ;
}

//////////////////////////////////////////////////////////////////////////
i2c_operation_t I2C_Write_Address_Uint8_CB( void *p )
{
	I2C_SetDataBuffer( p, 1 );
	I2C_SetDataXferCompleteCallback( I2C_Stop_CB, NULL );
	return I2C_CONTINUE;
}

void I2C_Write_Address_Uint8( i2c_address_t slaveAddress, uint16_t address, uint8_t data )
{
	while ( I2C_OPEN( slaveAddress ) == I2C_BUSY ) ;
	I2C_SetDataXferCompleteCallback( I2C_Write_Address_Uint8_CB, &data );
	address = address << 8 | address >> 8;
	I2C_SetDataBuffer( &address, sizeof(address) );
	I2C_M_READ_WRITE( M_WRITE );
	while ( I2C_CLOSE() == I2C_BUSY ) ;
}

i2c_operation_t I2C_Write_Address_Uint16_CB( void *p ) {
	I2C_SetDataBuffer( p, 2 );
	I2C_SetDataXferCompleteCallback( I2C_Stop_CB, NULL );
	return I2C_CONTINUE;
}

void I2C_Write_Address_Uint16( i2c_address_t slaveAddress, uint16_t address, uint16_t data )
{
	while ( I2C_OPEN( slaveAddress ) == I2C_BUSY ) ;
	I2C_SetDataXferCompleteCallback( I2C_Write_Address_Uint16_CB, &data );
	address = address << 8 | address >> 8;
	I2C_SetDataBuffer( &address, sizeof(address) );
	I2C_M_READ_WRITE( M_WRITE );
	while ( I2C_CLOSE() == I2C_BUSY ) ;
}

i2c_operation_t I2C_Write_Address_Block_CB( void *p )
{
	i2c_block_t *pp = (i2c_block_t *)p;
	
	I2C_SetDataBuffer( pp->buff, pp->len );
	I2C_SetDataXferCompleteCallback( I2C_Stop_CB, NULL );
	
	return I2C_CONTINUE;
}

void I2C_Write_Address_Block( i2c_address_t slaveAddress, uint16_t address, uint8_t *buffer, uint16_t length )
{
	i2c_block_t block;
	
	block.buff = buffer;
	block.len = length;
	
	while ( I2C_OPEN( slaveAddress ) == I2C_BUSY ) ;
	I2C_SetDataXferCompleteCallback( I2C_Write_Address_Block_CB, &block );
	address = address << 8 | address >> 8;
	I2C_SetDataBuffer( &address, sizeof(address) );
	I2C_M_READ_WRITE( M_WRITE );
	while ( I2C_CLOSE() == I2C_BUSY ) ;
}

i2c_operation_t I2C_Read_Address_Uint8_CB( void *p )
{
	I2C_SetDataBuffer( p, 1 );
	I2C_SetDataXferCompleteCallback( I2C_Stop_CB, NULL );
	return I2C_RESTART_READ;
}

uint8_t I2C_Read_Address_Uint8( i2c_address_t slaveAddress, uint16_t address )
{
	uint8_t		data;
	
	while ( I2C_OPEN( slaveAddress ) == I2C_BUSY ) ;
	I2C_SetDataXferCompleteCallback( I2C_Read_Address_Uint8_CB, &data );
	address = address << 8 | address >> 8;
	I2C_SetDataBuffer( &address, sizeof(address) );
	I2C_M_READ_WRITE( M_WRITE );
	while ( I2C_CLOSE() == I2C_BUSY ) ;
	
	return data;
}

i2c_operation_t I2C_Read_Address_Uint16_CB( void *p )
{
	I2C_SetDataBuffer( p, 2 );
	I2C_SetDataXferCompleteCallback( I2C_Stop_CB, NULL );
	return I2C_RESTART_READ;
}

uint16_t I2C_Read_Address_Uint16( i2c_address_t slaveAddress, uint16_t address )
{
	uint16_t		data;
	
	while ( I2C_OPEN( slaveAddress ) == I2C_BUSY ) ;
	I2C_SetDataXferCompleteCallback( I2C_Read_Address_Uint16_CB, &data );
	address = address << 8 | address >> 8;
	I2C_SetDataBuffer( &address, sizeof(address) );
	I2C_M_READ_WRITE( M_WRITE );
	while ( I2C_CLOSE() == I2C_BUSY ) ;
	
	return data;
}


i2c_operation_t I2C_Read_Address_Block_CB( void *p )
{
	i2c_block_t *pp = (i2c_block_t *)p;
	
	I2C_SetDataBuffer( pp->buff, pp->len );
	I2C_SetDataXferCompleteCallback( I2C_Stop_CB, NULL );
	
	return I2C_RESTART_READ;
}

void I2C_Read_Address_Block( i2c_address_t slaveAddress, uint16_t address, uint8_t *buffer, uint16_t length )
{
	i2c_block_t block;
	
	block.buff = buffer;
	block.len = length;
	
	while ( I2C_OPEN( slaveAddress ) == I2C_BUSY ) ;
	I2C_SetDataXferCompleteCallback( I2C_Read_Address_Block_CB, &block );
	address = address << 8 | address >> 8;
	I2C_SetDataBuffer( &address, sizeof(address) );
	I2C_M_READ_WRITE( M_WRITE );
	while ( I2C_CLOSE() == I2C_BUSY ) ;
}
