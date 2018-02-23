////////////////////////////////////////////////////////////////////////////////
//
//  File          : cart_client.c
//  Description   : This is the client side of the CART communication protocol.
//
//   Author       : Jacob Hohenstein
//  Last Modified : 12/11/16
//

// Include Files
#include <stdio.h>
#include <arpa/inet.h>

// Project Include Files
#include <cart_network.h>
#include <cart_driver.h>

//
//  Global data
int client_socket = -1;
int                cart_network_shutdown = 0;   // Flag indicating shutdown
unsigned char     *cart_network_address = NULL; // Address of CART server
unsigned short     cart_network_port = 0;       // Port of CART serve
unsigned long      CartControllerLLevel = 0; // Controller log level (global)
unsigned long      CartDriverLLevel = 0;     // Driver log level (global)
unsigned long      CartSimulatorLLevel = 0;  // Driver log level (global)

////////////////////////////////////////////////////////////////////////////////
//IPv4 Address Structure

struct             sockaddr_in caddr; 

//
// Functions

////////////////////////////////////////////////////////////////////////////////
//
// Function     : client_cart_bus_request
// Description  : This the client operation that sends a request to the CART
//                server process.   It will:
//
//                1) if INIT make a connection to the server
//                2) send any request to the server, returning results
//                3) if CLOSE, will close the connection
//
// Inputs       : reg - the request reqisters for the command
//                buf - the block to be read/written from (READ/WRITE)
// Outputs      : the response structure encoded as needed

int client_cart_bus_request(CartXferRegister reg, void *buf) {
	uint64_t value; //used to hold values when switching between network byte order and host byte order
	//if there is no existing connection, make a connection to the server
	if(client_socket==-1){
		//setting up address information
		caddr.sin_family = AF_INET; //protocol family
		caddr.sin_port = htons(CART_DEFAULT_PORT);
		if( inet_aton(CART_DEFAULT_IP, &caddr.sin_addr) == 0){
			return(-1);
		}
		client_socket = socket(AF_INET, SOCK_STREAM, 0); //creates file handle for network and saves it in client_socket
		if (client_socket == -1){
			printf("Error on socket creation \n"); //if socket was not created output this error message
			return(-1);
		}
		//connect socket file descriptor to address, returns 0 if successfule, -1 if failed
		if(connect(client_socket, (const struct sockaddr *)&caddr, sizeof(caddr)) ==-1) {
			printf("Error on socket connect\n"); 
			return(-1);
		}
	}
	//use already existing connection
	else{
		const struct Opcode oregstate= {0};
		extract_cart_opcode(reg,oregstate);
		
		switch(oregstate.KY1){
			case CART_OP_RDFRME:
				value = htonll64(reg);
				if(write(client_socket, &value, sizeof(value)) != sizeof(value)) {
					printf("Error writing network data\n");
					return(-1);
				} 
				printf("Sent a value of [%i]\n", ntohll64(value));
				if( read( client_socket, &value, sizeof(value)) != sizeof(value)) {
					printf("Error reading network data\n");
					return(-1);
				}
				value = ntohll64(value);
				printf("Recieved a value of [%lu]\n",value);
				//char buf = malloc(1024);
				if( read( client_socket, buf, sizeof(buf)) != sizeof(buf)) {
					printf("Error reading network data\n");
					return(-1);
				}
				printf("Recieved a buf of [%p]\n",buf);
				break;

			case CART_OP_WRFRME: 
				value = htonll64(reg);
				if(write(client_socket, &value, sizeof(value)) != sizeof(value)) {
					printf("Error writing network data \n");
					return(-1);
				} 
				printf("Sent a value of [%d]\n", ntohll64(value));
				////////////////////////////////////////////////////////////////////////////////////////
				//char buf = malloc(1024);
				if(write(client_socket,buf, sizeof(buf)) != sizeof(buf)) {
					printf("Error writing network data\n");
					return(-1);
				}
				/////////////////////////////////////////////////////////////////////////////////////////
				if( read( client_socket, &value, sizeof(value)) != sizeof(value)) {
					printf("Error reading network data\n");
					return(-1);
				}
				value = ntohll64(value);
				printf("Recieved a value of [%lu]\n",value);
				break;

			case CART_OP_POWOFF:
				value = htonll64(reg);
				if(write(client_socket, &value, sizeof(value)) != sizeof(value)) {
					printf("Error writing network data\n");
					return(-1);
				} 
				printf("Sent a value of [%d]\n", ntohll64(value));
				if( read( client_socket, &value, sizeof(value)) != sizeof(value)) {
					printf("Error reading network data \n");
					return(-1);
				}
				value = ntohll64(value);
				printf("Recieved a value of [%lu]\n",value);
				close(client_socket);
				client_socket = -1; 
				break;

			default:
				value = htonll64(reg);
				if(write(client_socket, &value, sizeof(value)) != sizeof(value)) {
					printf("Error writing network data \n");
					return(-1);
				} 
				printf("Sent a value of [%d]\n", ntohll64(value));
				if( read( client_socket, &value, sizeof(value)) != sizeof(value)) {
					printf("Error reading network data\n");
					return(-1);
				}
				value = ntohll64(value);
				printf("Recieved a value of [%lu]\n",value);
				break;
		}

	}
	return(0);
}
