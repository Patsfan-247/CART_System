////////////////////////////////////////////////////////////////////////////////
//
//  File           : cart_driver.c
//  Description    : This is the implementation of the standardized IO functions
//                   for used to access the CART storage system.
//
//  Author         : Jacob Hohenstein
//  Last Modified  : 12/9/16
//

// Includes
#include <stdlib.h>
#include <string.h>

// Project Includes
#include <cart_driver.h>
#include <cart_controller.h>
#include <cart_cache.h>
#include <cmpsc311_log.h>

//structure for the 5 elements in the opcode register
typedef struct {
	CartXferRegister KY1;
	CartXferRegister KY2;
	CartXferRegister RT1;
	CartXferRegister CT1;
	CartXferRegister FM1;
}Opcode;

//File System
//
struct Frame {
	CartridgeIndex cart_index;
	CartFrameIndex frame_index;
};

struct File {
	char file_path[CART_MAX_TOTAL_FILES];
	int current_pos;
	int end_pos;
	int file_open;
	struct Frame FrameList[CART_CARTRIDGE_SIZE];
	int frame_list_index;
};

struct File FileArray[CART_MAX_TOTAL_FILES]; //an array of struct file will contain all the file data
int file_counter;

CartridgeIndex loaded_cartridge= -1;           //keeps track of what cartirdge is currently loaded
CartridgeIndex avail_cart;
CartFrameIndex avail_frame;
//
//
// Implementation
////////////////////////////////////////////////////////
//
// Function     : create_cart_opcode
// Description  : Creates a register structure
//
// Inputs       : The opcode registers
// Outputs      : regstate
CartXferRegister create_cart_regstate(CartXferRegister KY1, CartXferRegister KY2,CartXferRegister RT1,CartXferRegister CT1, CartXferRegister FM1) {
	CartXferRegister regstate = 0x0, tempKY1, tempKY2, tempRT1, tempCT1, tempFM1, unused;

	tempKY1 = (KY1&0xff) <<56;
	tempKY2 = (KY2&0xff) <<48;
	tempRT1 = (RT1&0x1) <<47;
	tempCT1 = (CT1&0xffff) <<31;
	tempFM1 = (FM1&0xffff) << 15;
	unused = 0x0;
	regstate = tempKY1|tempKY2|tempRT1|tempCT1|tempFM1|unused; //combines the temporary registers to create a uint64_t opcode
	return( regstate );
}

////////////////////////////////////////////////////////////////////////////////
//
// Function     : extract_cart_opcode
// Description  : extracts a register structure
//
// Inputs       : opcode, struct for the 5 elements in register
// Outputs      : return 0 if success, 
int extract_cart_opcode(CartXferRegister reg, struct OpCode Oregstate)
{
	const struct Opcode oregstate={0};
	oregstate.KY1 = (reg&0xff00000000000000) >>56;
	oregstate.KY2 = (reg&0x00ff000000000000) >>48;
	oregstate.RT1 = (reg&0x0000800000000000) >>47;
	oregstate.CT1 = (reg&0x00007FFF80000000) >>31;
	oregstate.FM1 = (reg&0x000000007FFF8000) >>15;
	return (0);
}  

////////////////////////////////////////////////////////////////////////////////
//
// Function     : LDCART_opcode
// Description  : uses the cart_io_bus to load a cartrige
//
// Inputs       : cart_index : the index of cart to load
// Outputs      : return 0 if success, -1 if failed

int LDCART_opcode(CartridgeIndex cart_index)
{
	if(loaded_cartridge != cart_index) {
		CartXferRegister regstate= 0x0;
		const Opcode oregstate= {0};
		regstate = create_cart_regstate(CART_OP_LDCART,0,0,cart_index,0);
		oregstate= cart_client_bus_request(regstate,NULL);
		extract_cart_opcode(regstate,oregstate);
		if(oregstate.RT1 == -1){
			logMessage(LOG_ERROR_LEVEL, "CART driver failed: failed to load cartridge");
		return(-1);
		}
	loaded_cartridge = cart_index;
	}
	return(0);
}

////////////////////////////////////////////////////////////////////////////////
//
// Function     : BZERO_opcode
// Description  : uses the cart_io_bus to zero the currently loaded cartridge
//
// Inputs       : none
// Outputs      : return 0 if success, -1 if failed
int BZERO_opcode(void){
	CartXferRegister regstate= 0x0;
	const struct Opcode oregstate= {0};
	regstate = create_cart_regstate(CART_OP_BZERO,0,0,0,NULL);
	oregstate= cart_client_bus_request(regstate,NULL);
	extract_cart_opcode(regstate,oregstate);
	if(oregstate.RT1 == -1){
		logMessage(LOG_ERROR_LEVEL, "CART driver failed: failed to zero current cartridge");
		return(-1);
	}
	return(0);
}

////////////////////////////////////////////////////////////////////////////////
//
// Function     : RDFRME_opcode
// Description  : reads the frame to the current cartridge using cart_io_bus
//
// Inputs:	  frame_index- index of the frame being read
//		  *buf- char pointer where data from frame is written
// Outputs      : return 0 if success, -1 if failed
int RDFRME_opcode(CartFrameIndex frame_index, char *buf) {
	//cache is checked for frame being read first
	char *tempbuf;
	tempbuf = get_cart_cache(loaded_cartridge,frame_index);
	if(tempbuf==NULL) {
		CartXferRegister regstate= 0x0;
		const struct Opcode oregstate = {0};
		regstate = create_cart_regstate(CART_OP_RDFRME,0,0,0,frame_index);
		oregstate= cart_client_bus_request(regstate,buf);
		extract_cart_opcode(regstate,oregstate);
		if(oregstate.RT1 == -1){
			logMessage(LOG_ERROR_LEVEL, "CART driver failed: failed to read frame");
			return(-1);
		}
	}
	else {
		memcpy(buf,tempbuf,CART_FRAME_SIZE);
	}
	return(0);
}

////////////////////////////////////////////////////////////////////////////////
//
// Function     : WRFRME_opcode
// Description  : writes a frame to current cartridge using cart_io_bus
//
// Inputs:	  frame_index- index of the frame being written
//		  *buf- char pointer that contains the characters to be written
// Outputs      : return 0 if success, -1 if failed
int WRFRME_opcode(CartFrameIndex frame_index, char *buf)
{
	CartXferRegister regstate= 0x0;
	struct Opcode oregstate = {0};
	regstate = create_cart_regstate(CART_OP_WRFRME,0,0,0,frame_index);
	oregstate= cart_client_bus_request(regstate,buf);
	extract_cart_opcode(regstate,oregstate);
	if(oregstate.RT1 == -1){
		logMessage(LOG_ERROR_LEVEL, "CART driver failed: failed to write to frame");
		return(-1);
	}
	return(0);
}
////////////////////////////////////////////////////////////////////////////////
//
// Function     : cart_poweron
// Description  : Startup up the CART interface, initialize filesystem
//
// Inputs       : none
// Outputs      : 0 if successful, -1 if failure

int32_t cart_poweron(void) {
	loaded_cartidge= NULL;
	CartXferRegister regstate= 0x0, i;
	const struct Opcode oregstate = {0};
	if((regstate = create_cart_regstate(CART_OP_INITMS,0,0,0,NULL)) ==-1) {
		logMessage(LOG_ERROR_LEVEL, "CART driver failed: fail on init");
		return(-1);
	}
	oregstate = cart_client_bus_request(regstate, NULL);
	extract_cart_opcode(regstate, oregstate);
	if(oregstate.RT1 !=0) {
		logMessage(LOG_ERROR_LEVEL, "CART driver failed: fail to power on");
		return(-1);
	}

	//load and zero all cartridges
	for(i = 0x0; i <CART_MAX_CARTRIDGES;i++)
	{
		//load cartridge i
		if(LDCART_opcode(i) == -1)
			return(-1);
		//zeroes current cartridge
		BZERO_opcode();
	}
	// Initilize and set up file system
	file_counter= 0;
	for(i=0;i <CART_MAX_TOTAL_FILES;i++)
	{
		FileArray[i].file_path[0]= '\0';
		FileArray[i].current_pos= 0;
		FileArray[i].end_pos= 0;
		FileArray[i].file_open= 0;
	}
	avail_cart= 0;
	avail_frame= 0;

	//Initialize Cache
	init_cart_cache();
	// Return successfully
	return(0);
}

////////////////////////////////////////////////////////////////////////////////
//
// Function     : cart_poweroff
// Description  : Shut down the CART interface, close all files
//
// Inputs       : none
// Outputs      : 0 if successful, -1 if failure

int32_t cart_poweroff(void) {
	CartXferRegister regstate=0x0;
	const Opcode oregstate={0};
	regstate = create_cart_regstate(CART_OP_POWOFF,0,0,0,NULL);
	oregstate = cart_client_bus_request(regstate,NULL);
	extract_cart_opcode(regstate,oregstate);
	if(oregstate.RT1 !=0) {
		logMessage(LOG_ERROR_LEVEL, "CART driver failed: fail to power off");
		return(-1);
	}
	//close cache
	close_cart_cache();
	// Return successfully
	return(0);
}

////////////////////////////////////////////////////////////////////////////////
//
// Function     : cart_open
// Description  : This function opens the file and returns a file handle
//
// Inputs       : path - filename of the file to open
// Outputs      : file handle if successful, -1 if failure

int16_t cart_open(char *path) {
	int length = strlen(path) +1; //includes '/0'
	//search for a file with that path
	int i;
	for(i=0;i<file_counter;i++)
	{
		if(strncmp(FileArray[i].file_open, path, length)==0) {
			//if file open
			if(FileArray[i].file_open == 1) {
				return(-1);
			} 
			else {
				FileArray[i].file_open = 1;
				FileArray[i].current_pos= 0;
				return(i); 
			}
		}
	}
	file_counter++;
	strncpy(FileArray[file_counter].file_path,path,length);
	FileArray[file_counter].file_open =1;
	FileArray[file_counter].current_pos = 0;
	FileArray[file_counter].end_pos = 0;
	FileArray[file_counter].frame_list_index = -1;

	// THIS SHOULD RETURN A FILE HANDLE
	return (file_counter);
}

////////////////////////////////////////////////////////////////////////////////
//
// Function     : cart_close
// Description  : This function closes the file
//
// Inputs       : fd - the file descriptor
// Outputs      : 0 if successful, -1 if failure

int16_t cart_close(int16_t fd) {
	if (fd >= CART_MAX_TOTAL_FILES || fd <0) {
		logMessage(LOG_ERROR_LEVEL, "CART driver failed: bad file handle.");
		return (-1);
	}

	if (FileArray[fd].file_open == 0) {
		//file has already been closed
		logMessage(LOG_ERROR_LEVEL, "CART driver failed: file is closed.");
		return (-1);
	}
	FileArray[fd].file_open = 0;
	// Return successfully
	return (0);
}

////////////////////////////////////////////////////////////////////////////////
//
// Function     : cart_read
// Description  : Reads "count" bytes from the file handle "fh" into the 
//                buffer "buf"
//
// Inputs       : fd - filename of the file to read from
//                buf - pointer to buffer to read into
//                count - number of bytes to read
// Outputs      : bytes read if successful, -1 if failure

int32_t cart_read(int16_t fd, void *buf, int32_t count) {
	int32_t read_length = count, frame_pos, buf_loc= 0, remaining_bytes= count;
	char tempbuf[CART_FRAME_SIZE];
	if (fd >= CART_MAX_TOTAL_FILES || fd <0) {
		logMessage(LOG_ERROR_LEVEL, "CART driver failed: bad file handle.");
		return (-1);
	}

	if (FileArray[fd].file_open == 0) {
		//file has already been closed
		logMessage(LOG_ERROR_LEVEL, "CART driver failed: file is closed.");
		return (-1);
	}
	if(count > (FileArray[fd].end_pos - FileArray[fd].current_pos)) {
		read_length = FileArray[fd].end_pos - FileArray[fd].current_pos;
		remaining_bytes = read_length;		
	}
	while (remaining_bytes > 0) {
		if(RDCART_opcode(FileArray[fd].FrameList[FileArray[fd].current_pos/CART_FRAME_SIZE].cart_index, FileArray[fd].FrameList[FileArray[fd].current_pos%CART_FRAME_SIZE].frame_index, tempbuf) == -1)
			return(-1);
		
		int frame_bytes;
		if(remaining_bytes == read_length){
			if(read_length < CART_FRAME_SIZE)
				frame_bytes= read_length;
			else
				frame_bytes= CART_FRAME_SIZE-frame_pos;

			memcpy(buf+buf_loc,&tempbuf[frame_pos], frame_bytes);
		}
		else if (remaining_bytes < CART_FRAME_SIZE) {
			frame_bytes= remaining_bytes;
			memcpy(buf+buf_loc,tempbuf, frame_bytes); // all of the frame is copied
		}
		else {
			frame_bytes = CART_FRAME_SIZE;
			memcpy(buf+buf_loc,tempbuf,frame_bytes);
		}
		FileArray[fd].current_pos += frame_bytes;
		buf_loc += frame_bytes;
		remaining_bytes -= frame_bytes;
	}
	// Return successfully
	return (read_length);
}

////////////////////////////////////////////////////////////////////////////////
//
// Function     : cart_write
// Description  : Writes "count" bytes to the file handle "fh" from the 
//                buffer  "buf"
//
// Inputs       : fd - filename of the file to write to
//                buf - pointer to buffer to write from
//                count - number of bytes to write
// Outputs      : bytes written if successful, -1 if failure

int32_t cart_write(int16_t fd, void *buf, int32_t count) {
	char tempbuf[CART_FRAME_SIZE];
	int remaining_bytes= count, write_bytes, frame_pos, list_loc, buf_loc= 0;
	if (fd >= CART_MAX_TOTAL_FILES || fd <0) {
		logMessage(LOG_ERROR_LEVEL, "CART driver failed: bad file handle.");
		return (-1);
	}

	if (FileArray[fd].file_open == 0) {
		//file has already been closed
		logMessage(LOG_ERROR_LEVEL, "CART driver failed: file is closed.");
		return (-1);
	}

	while(remaining_bytes >0){
		frame_pos= FileArray[fd].current_pos % CART_FRAME_SIZE;
		list_loc= FileArray[fd].current_pos /CART_FRAME_SIZE;
		
		if(CART_FRAME_SIZE <= remaining_bytes + frame_pos){
			write_bytes = CART_FRAME_SIZE - frame_pos;
		}
		else if((remaining_bytes + frame_pos)% CART_FRAME_SIZE != 0) {
			write_bytes = remaining_bytes % CART_FRAME_SIZE;
		}
		else {
			write_bytes = CART_FRAME_SIZE;
		}
		if(list_loc > FileArray[fd].frame_list_index){
			FileArray[fd].FrameList[list_loc].cart_index = avail_cart;
			FileArray[fd].FrameList[list_loc].frame_index = avail_frame;
			if (avail_frame >= CART_CARTRIDGE_SIZE - 1) {
				avail_cart += 1;
				avail_frame = 0;
			} else {
				avail_frame++;	
			}
		}
		else{
			RDCART_opcode(FileArray[fd].FrameList[list_loc].cart_index,FileArray[fd].FrameList[list_loc].frame_index, tempbuf);
		}
		//update
		memcpy(&tempbuf[frame_pos], buf+buf_loc, write_bytes);
		//write to cache
		put_cart_cache(FileArray[fd].FrameList[list_loc].cart_index, FileArray[fd].FrameList[list_loc].frame_index, tempbuf);
		//write to cart
		WRCART_opcode(FileArray[fd].FrameList[list_loc].cart_index, FileArray[fd].FrameList[list_loc].frame_index, tempbuf);
		buf_loc += write_bytes;
		remaining_bytes -= write_bytes;
		FileArray[fd].current_pos += write_bytes;
		if (FileArray[fd].end_pos < FileArray[fd].current_pos){
			FileArray[fd].end_pos = FileArray[fd].current_pos;
		}
	}
	// Return successfully
	return (count);
}

////////////////////////////////////////////////////////////////////////////////
//
// Function     : cart_read
// Description  : Seek to specific point in the file
//
// Inputs       : fd - filename of the file to write to
//                loc - offfset of file in relation to beginning of file
// Outputs      : 0 if successful, -1 if failure

int32_t cart_seek(int16_t fd, uint32_t loc) {
	if (fd >= CART_MAX_TOTAL_FILES || fd <0) {
		logMessage(LOG_ERROR_LEVEL, "CART driver failed: bad file handle.");
		return (-1);
	}

	if (FileArray[fd].file_open == 0) {
		//file has already been closed
		logMessage(LOG_ERROR_LEVEL, "CART driver failed: file is closed.");
		return (-1);
	}
	if(loc > FileArray[fd].end_pos){
		logMessage(LOG_ERROR_LEVEL, "CART driver failed: loc exceeds file length.");
		return (-1);
	}
	FileArray[fd].current_pos= loc;
	// Return successfully
	return (0);
}
