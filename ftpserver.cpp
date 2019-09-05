//////////////////////////////////////
//Peter Fredatovich
//98141269
/////////////////////////////////////

#define USE_IPV6 false
#define _WIN32_WINNT 0x501 //ADDED to make Maefile work

#include <winsock2.h>
#include <ws2tcpip.h> //required by getaddrinfo() and special constants
#include <stdlib.h>
#include <stdio.h>
#include <iostream>
#include <direct.h> //used for chdir / getcwd
#define WSVERS MAKEWORD(2,2) /* Use the MAKEWORD(lowbyte, highbyte) macro declared in Windef.h */
                    //The high-order byte specifies the minor version number; 
                    //the low-order byte specifies the major version number.

WSADATA wsadata; //Create a WSADATA object called wsadata. 

#define DEFAULT_PORT "1234"
#define BUFFER_SIZE 200

using namespace std;


//********************************************************************
//MAIN
//********************************************************************
int main(int argc, char *argv[]) {

	
	char username[80];
	char passwd[80];
	char Root[BUFFER_SIZE];
	bool authorisedUser = false;


	//********************************************************************
	// INITIALIZATION of the SOCKET library
	//********************************************************************
	struct sockaddr_storage localaddr,remoteaddr; //IPV6-compatible
	char clientHost[NI_MAXHOST]; 
	char clientService[NI_MAXSERV];

	SOCKET ListenSocket =INVALID_SOCKET, ClientSocket=INVALID_SOCKET;
	SOCKET ListenSocket_data_act = INVALID_SOCKET, ClientSocket_data = INVALID_SOCKET;

	char send_buffer[BUFFER_SIZE],receive_buffer[BUFFER_SIZE];
	int active=0;
	int n,bytes,addrlen;
	char portNum[NI_MAXSERV];

	//********************************************************************
	// Clean up the structures
	//********************************************************************
	ZeroMemory(&username, sizeof(username));
	ZeroMemory(&passwd, sizeof(passwd));
	ZeroMemory(&localaddr, sizeof(localaddr));
	ZeroMemory(&remoteaddr, sizeof(remoteaddr));

	//********************************************************************
	// WSSTARTUP
	//********************************************************************
	int err = WSAStartup(WSVERS, &wsadata);
	if (err != 0) {
		WSACleanup();
		// Tell the user that we could not find a usable WinsockDLL
		cout<<"WSAStartup failed with error: "<<err<<endl;
		exit(1);
	}
  
	//********************************************************************
	/* Confirm that the WinSock DLL supports 2.2.        */
	/* Note that if the DLL supports versions greater    */
	/* than 2.2 in addition to 2.2, it will still return */
	/* 2.2 in wVersion since that is the version we      */
	/* requested.                                        */
	//********************************************************************
    if (LOBYTE(wsadata.wVersion) != 2 || HIBYTE(wsadata.wVersion) != 2) {
        /* Tell the user that we could not find a usable */
        /* WinSock DLL.                                  */
        cout<<"Could not find a usable version of Winsock.dll"<<endl;
        WSACleanup();
        exit(1);
    }else{              
    	cout<<"The Winsock 2.2 dll was initialised."<<endl;
    }

	cout<<endl<<"==============================="<<endl;
	cout<<"     159.334 FTP Server"<<endl;
	cout<<"==============================="<<endl<<endl;

	//********************************************************************
	//SOCKET
	//********************************************************************

	//********************************************************************
	// set the socket address structure.
	//********************************************************************
	struct addrinfo *result = NULL;
	struct addrinfo hints;
	int iResult;


	//********************************************************************
	// STEP#0 - Specify server address information and socket properties
	//********************************************************************
		 
	ZeroMemory(&hints, sizeof (hints)); 

	if(USE_IPV6){
		hints.ai_family = AF_INET6;  
	}else{ //IPV4
		hints.ai_family = AF_INET;
	}

	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = IPPROTO_TCP;
	hints.ai_flags = AI_PASSIVE; 	// For wildcard IP address 
                             		//setting the AI_PASSIVE flag indicates the caller intends to use 
									//the returned socket address structure in a call to the bind function. 

	// Resolve the local address and port to be used by the server
	if(argc==2){	 
		iResult = getaddrinfo(NULL, argv[1], &hints, &result); 
		sprintf(portNum,"%s", argv[1]);
		cout<<"Using Specified Port "<<argv[1]<<endl;

	} else {
   		iResult = getaddrinfo(NULL, DEFAULT_PORT, &hints, &result); 
   		sprintf(portNum,"%s", DEFAULT_PORT);
	 	cout<<"Using DEFAULT_PORT = "<<DEFAULT_PORT<<endl;
	}

	if (iResult != 0) {
    	cout<<"getaddrinfo failed: "<< iResult<<endl;
    	WSACleanup();
    	return 1;
	}	 

	//********************************************************************
	// STEP#1 - Create Listen SOCKET
	//********************************************************************

	ListenSocket = socket(result->ai_family, result->ai_socktype, result->ai_protocol);			
 	if (ListenSocket == INVALID_SOCKET) {
    	cout<<"Error at socket(): "<< WSAGetLastError()<<endl;
    	freeaddrinfo(result);
    	WSACleanup();
     	exit(1);//return 1;
  	}

	//********************************************************************
	//STEP#2 - BIND the Listen socket
	//********************************************************************
   	iResult = bind( ListenSocket, result->ai_addr, (int)result->ai_addrlen);

    if (iResult == SOCKET_ERROR) {
    	cout<<"bind failed with error: "<< WSAGetLastError()<<endl;
      	freeaddrinfo(result);
      	closesocket(ListenSocket);
      	WSACleanup();
      	return 1;
    }
    freeaddrinfo(result);
	 
	//********************************************************************
	//STEP#3 - LISTEN on welcome socket for any incoming connection
	//********************************************************************

    if (listen( ListenSocket, SOMAXCONN ) == SOCKET_ERROR ) {
    	/* A value for the backlog of SOMAXCONN is a special constant that instructs
    	 * the underlying service provider responsible for socket s to set the length
    	 * of the queue of pending connections to a maximum reasonable value */
    	cout<<"Listen failed with error: "<< WSAGetLastError()<<endl;
      	closesocket(ListenSocket);
      	WSACleanup(); 
      	exit(1);
   	} 

   	//If we get this far Listen Socket should be open


	// ********************************************************************
	// Change directory to ftp_root
	// ********************************************************************
	chdir("ftp_root");
	getcwd(Root, BUFFER_SIZE);
			
	//********************************************************************
	//INFINITE LOOP
	//********************************************************************
	//====================================================================================
	while (1) {//Start of MAIN LOOP
	//====================================================================================
		addrlen = sizeof(remoteaddr);

		cout<<"\n------------------------------------------------------------------------"<<endl;
		cout<<"SERVER is waiting for an incoming connection request..."<<endl;
		cout<<"------------------------------------------------------------------------"<<endl;
		
		//ClientSocket = INVALID_SOCKET;
		
		//********************************************************************
		//NEW SOCKET ClientSocket = accept  //CONTROL CONNECTION
		//********************************************************************	
		// STEP#4 - Accept a client connection.  
		//	accept() blocks the iteration, and causes the program to wait.  
		//	Once an incoming client is detected, it returns a new socket ns
		// exclusively for the client.  
		// It also extracts the client's IP address and Port number and stores
		// it in a structure.
		//********************************************************************
		
		ClientSocket = accept(ListenSocket,(struct sockaddr *)(&remoteaddr),&addrlen); //IPV4 & IPV6-compliant
      
  		if (ClientSocket == INVALID_SOCKET) {
     		cout<<"FLAG Accept failed: "<< WSAGetLastError();
     		closesocket(ListenSocket);
     		WSACleanup();
     		return 1;
  		} else {
			DWORD returnValue;
			memset(clientHost, 0, sizeof(clientHost));
			memset(clientService, 0, sizeof(clientService));

			returnValue = getnameinfo((struct sockaddr *)&remoteaddr, addrlen,
            				clientHost, sizeof(clientHost),
            				clientService, sizeof(clientService),
            				NI_NUMERICHOST);
			if(returnValue != 0){
   				cout<<"\nError detected: getnameinfo() failed with error#%d\n"<<WSAGetLastError();
   				exit(1);
			} else{
				cout<<"\n============================================================================"<<endl;
   				cout<<"connected to [CLIENT's IP "<< clientHost <<", port "<< clientService <<" through SERVER's port "<< portNum<<endl;
   				cout<<"============================================================================\n"<<endl;
			}
		}
		//********************************************************************
		//Respond with welcome message
		//*******************************************************************
		sprintf(send_buffer,"220 FTP Server ready. \r\n"); //int sprintf ( char * str, const char * format, ... );  content is stored as a C string in the buffer pointed by str
		bytes = send(ClientSocket, send_buffer, strlen(send_buffer), 0);

		//=================================================================================
		//COMMUNICATION LOOP per CLIENT
		//=================================================================================
		while (1) {
			ZeroMemory(&receive_buffer, BUFFER_SIZE);			 
		 	n = 0;
		 	//=======================================================
		 	//PROCESS message received from CLIENT
		 	//=======================================================
		 	while (1) {
				//********************************************************************
				//RECEIVE
				//********************************************************************
				bytes = recv(ClientSocket, &receive_buffer[n], 1, 0);//receive byte by byte...
				//********************************************************************
				//PROCESS REQUEST
				//********************************************************************

				if ((bytes == SOCKET_ERROR) || (bytes == 0)) break;
				if (receive_buffer[n] == '\n') { /*end on a LF*/
					receive_buffer[n] = '\0';
					break;
				}
				if (receive_buffer[n] != '\r') n++; /*Trim CRs*/
			} 
			//=======================================================
			//End of PROCESS message received from CLIENT
			//=======================================================

			if ((bytes == SOCKET_ERROR) || (bytes == 0)) break;
			cout<<"<< DEBUG INFO. >>: the message from the CLIENT reads: "<< receive_buffer << endl;

			if (strncmp(receive_buffer,"USER",4)==0)  {
				ZeroMemory(&send_buffer, BUFFER_SIZE);    
				sscanf(receive_buffer,"%*s%s",username);

				if((strcmp(username,"nhreyes")  == 0) && (strcmp(username, "") !=0)){
					cout<<"Logging in Authorised User.\n";
					sprintf(send_buffer,"331 User name okay, need password. \r\n");
					bytes= send(ClientSocket,send_buffer,strlen(send_buffer),0);
					authorisedUser = true;
					if(authorisedUser)
						if(bytes < 0) break;
					}else{
						sprintf(send_buffer,"Logging in as anonymous. \r\n");
						bytes = send(ClientSocket, send_buffer, strlen(send_buffer), 0);
						if (bytes < 0) break;
					}
				ZeroMemory(&username, BUFFER_SIZE);  
			} 

			if (strncmp(receive_buffer,"PASS",4)==0)  {
				if(authorisedUser){
					sscanf(receive_buffer,"%*s%s",passwd);
					if(strcmp(passwd,"334")==0)	{
						sprintf(send_buffer,"230 Authorized User login successful \r\n");
						cout<<"<< DEBUG INFO. >>: REPLY sent to CLIENT: "<< send_buffer << endl;
						bytes = send(ClientSocket, send_buffer, strlen(send_buffer), 0);
						if (bytes < 0) break;				 			
					}else{
						sprintf(send_buffer,"230 Incorrect Password. Public login successful. \r\n");
						cout<<"<< DEBUG INFO. >>: REPLY sent to CLIENT: "<< send_buffer << endl;
						bytes = send(ClientSocket, send_buffer, strlen(send_buffer), 0);
						authorisedUser = false;					        
						if (bytes < 0) break;
					}
				}else{
					sprintf(send_buffer,"230 Public login successful.\r\n");
					cout<<"<< DEBUG INFO. >>: REPLY sent to CLIENT: "<< send_buffer << endl;
					bytes = send(ClientSocket, send_buffer, strlen(send_buffer), 0);
					if (bytes < 0) break;
				}
			}


			if (strncmp(receive_buffer,"SYST",4)==0)  {
				cout<<"Information about the system"<<endl;
				sprintf(send_buffer,"215 Windows Type: WIN32\r\n");
				cout<<"<< DEBUG INFO. >>: REPLY sent to CLIENT: "<< send_buffer << endl;
				bytes = send(ClientSocket, send_buffer, strlen(send_buffer), 0);
				if (bytes < 0) break;
			}


			if (strncmp(receive_buffer,"OPTS",4)==0)  {
				cout << endl << "Unrecognised command" << endl;
				sprintf(send_buffer,"502 command not implemented\r\n");
				cout<<"<< DEBUG INFO. >>: REPLY sent to CLIENT: "<< send_buffer << endl;
				bytes = send(ClientSocket, send_buffer, strlen(send_buffer), 0);
				if (bytes < 0) break;
			}


			if (strncmp(receive_buffer,"QUIT",4)==0)  {
				cout << "Quit. Logging out user. Connection closed by Client" << endl;
				authorisedUser = false;
				sprintf(send_buffer,"221 Connection close by client\r\n");
				cout<<"<< DEBUG INFO. >>: REPLY sent to CLIENT: "<< send_buffer << endl;
				bytes = send(ClientSocket, send_buffer, strlen(send_buffer), 0);
				if (bytes < 0) break;
				closesocket(ClientSocket);
			}

			if (strncmp(receive_buffer,"EPRT",4)==0)  {
				cout<< "EPRT Not supported" << endl;
				sprintf(send_buffer,"550 Not supported\r\n");
				cout<<"<< DEBUG INFO. >>: REPLY sent to CLIENT: "<< send_buffer << endl;
				bytes = send(ClientSocket, send_buffer, strlen(send_buffer), 0);
				if (bytes < 0) break;
			}

			//Specifies an address and port to which the server should connect.
			if(strncmp(receive_buffer,"PORT",4)==0) {
				int act_port[2];
				int act_ip[4], port_dec;
				char ip_decimal[40];
				char act_port_char[10];
				cout<< "\n==================================================="<<endl;
				cout<< "\tActive FTP mode, the client is listening... "<<endl;
				active=1;//flag for active connection

				int scannedItems = sscanf(receive_buffer, "PORT %d,%d,%d,%d,%d,%d",
				&act_ip[0],&act_ip[1],&act_ip[2],&act_ip[3],
				&act_port[0],&act_port[1]);

				if(scannedItems < 6) {
					sprintf(send_buffer,"501 Syntax error in arguments \r\n");
					cout<<"<< DEBUG INFO. >>: REPLY sent to CLIENT: "<< send_buffer << endl;
					bytes = send(ClientSocket, send_buffer, strlen(send_buffer), 0);
					break;
				}

				sprintf(ip_decimal, "%d.%d.%d.%d", act_ip[0], act_ip[1], act_ip[2],act_ip[3]);
				cout<<"\tCLIENT's IP is "<< ip_decimal << endl; 
				
				port_dec=act_port[0];
				port_dec=port_dec << 8;
				port_dec=port_dec+act_port[1];

				sprintf(act_port_char,"%d",port_dec);
				cout << "\tCLIENT's Port is "<< port_dec << endl;
				cout << "===================================================" << endl;
				iResult = getaddrinfo(ip_decimal, act_port_char, &hints, &result);////the returned socket address structure used for connection
				ListenSocket_data_act = socket(result->ai_family, result->ai_socktype, result->ai_protocol);	 

				sprintf(send_buffer, "200 PORT Command successful\r\n");
				bytes = send(ClientSocket, send_buffer, strlen(send_buffer), 0);
				cout<<"<< DEBUG INFO. >>: REPLY sent to CLIENT: "<< send_buffer << endl;
				cout << "Connected to client" << endl;

			}

			//  technically, LIST is different than NLST,but we make them the same here
			// 	Returns a list of file names in a specified directory.
			if ( (strncmp(receive_buffer,"LIST",4)==0) || (strncmp(receive_buffer,"NLST",4)==0))   {
			
				system("dir > tmp.txt");
				FILE *fin=fopen("tmp.txt","r");

				sprintf(send_buffer,"150 Opening ASCII mode data connection... \r\n");
				cout<<"<< DEBUG INFO. >>: REPLY sent to CLIENT: "<< send_buffer << endl;
				bytes = send(ClientSocket, send_buffer, strlen(send_buffer), 0);
				if (connect(ListenSocket_data_act, result->ai_addr, result->ai_addrlen) != 0){
					sprintf(send_buffer, "425 Can't open data connection \r\n");
					bytes = send(ClientSocket, send_buffer, strlen(send_buffer), 0);
					cout<<"<< DEBUG INFO. >>: REPLY sent to CLIENT: "<< send_buffer << endl;
					closesocket(ListenSocket_data_act);
					freeaddrinfo(result);
					WSACleanup();
				}else{					 
					char temp_buffer[80];
					while (!feof(fin)){
						fgets(temp_buffer,78,fin);
						sprintf(send_buffer,"%s",temp_buffer);
						if (active==0) send(ClientSocket_data, send_buffer, strlen(send_buffer), 0);
						else send(ListenSocket_data_act, send_buffer, strlen(send_buffer), 0);
					}
					fclose(fin);
					sprintf(send_buffer,"226 226 File transfer complete. \r\n");
					cout<<"<< DEBUG INFO. >>: REPLY sent to CLIENT: "<< send_buffer << endl;
					bytes = send(ClientSocket, send_buffer, strlen(send_buffer), 0);
					if (active==0 ){
						closesocket(ClientSocket_data);
					}else {
						closesocket(ListenSocket_data_act);
				}
				closesocket(ListenSocket_data_act);
				freeaddrinfo(result);
				}
				system("del tmp.txt");
			}


			if(strncmp(receive_buffer,"RETR",4)==0){
				char fileName[BUFFER_SIZE], temp_buffer[BUFFER_SIZE];
				ZeroMemory(fileName, BUFFER_SIZE);
				sscanf(receive_buffer,"%*s%s",fileName);
				FILE *stream;
				
				if(fopen(fileName,"r")==NULL){
					sprintf(send_buffer,"450 Requested file [%s] not found. \r\n", fileName);
					bytes = send(ClientSocket,send_buffer,strlen(send_buffer),0);
				}else{
					stream = fopen(fileName,"rb");
					sprintf(send_buffer,"150 File status okay;about to open data connection.\r\n");
					bytes = send(ClientSocket, send_buffer, strlen(send_buffer), 0);

					if (connect(ListenSocket_data_act, result->ai_addr, result->ai_addrlen) != 0){
						sprintf(send_buffer, "425 Can't open data connection \r\n");
						bytes = send(ClientSocket, send_buffer, strlen(send_buffer), 0);
						cout<<"<< DEBUG INFO. >>: REPLY sent to CLIENT: "<< send_buffer << endl;

						closesocket(ListenSocket_data_act);
						WSACleanup();
						freeaddrinfo(result);
					}else{	
						while (fgets(temp_buffer,BUFFER_SIZE,stream)){
							sprintf(send_buffer,"%s",temp_buffer);
							send(ListenSocket_data_act,send_buffer,strlen(send_buffer),0);
						}						
						if(fclose(stream)) cout<<"fclose error"<<endl;
						sprintf(send_buffer,"226 Closing data connection. Requested file action successful.\r\n");
						bytes = send(ClientSocket,send_buffer,strlen(send_buffer),0);
						closesocket(ListenSocket_data_act);
					}
				}
			}


			if(strncmp(receive_buffer,"STOR",4)==0){
				char fileName[BUFFER_SIZE];
				ZeroMemory(fileName, BUFFER_SIZE);
				sscanf(receive_buffer,"%*s%s",fileName);
				FILE *stream;

				if(fopen(fileName,"w")== NULL){
					sprintf(send_buffer,"450 Requested file action not taken.\r\n");
					bytes = send(ClientSocket,send_buffer,strlen(send_buffer),0);
				}else{
					stream = fopen(fileName,"wb");
					sprintf(send_buffer,"150 File status okay; about to open data connection. \r\n");
					bytes = send(ClientSocket,send_buffer,strlen(send_buffer),0);
					if (connect(ListenSocket_data_act, result->ai_addr, result->ai_addrlen) != 0){
						sprintf(send_buffer, "425 Can't open data connection \r\n");
						bytes = send(ClientSocket, send_buffer, strlen(send_buffer), 0);
						cout<<"<< DEBUG INFO. >>: REPLY sent to CLIENT: "<< send_buffer << endl;
						closesocket(ListenSocket_data_act);
						freeaddrinfo(result);
					}else{
						while(bytes>0){
							ZeroMemory(&receive_buffer, BUFFER_SIZE);
							bytes = recv(ListenSocket_data_act,receive_buffer,1,0);
							fwrite(receive_buffer,1,1,stream);
						}
						sprintf(send_buffer,"226 File transfer completed.\r\n");
						bytes = send(ClientSocket,send_buffer,strlen(send_buffer),0);
						if(fclose(stream)) cout<<"fclose error"<<endl;
						closesocket(ListenSocket_data_act);
					}	
				}
			}

			if(strncmp(receive_buffer,"CWD",3)==0){
				char fileName[BUFFER_SIZE];
				char path[BUFFER_SIZE];
				ZeroMemory(&path, BUFFER_SIZE);
				sscanf(receive_buffer,"%*s%s",fileName);
				if(strncmp(fileName,"vip_folder",10)==0){
					if(authorisedUser){
						if(chdir(fileName)==-1){
								sprintf(send_buffer, "550 CWD Failed. \"%s\": directory not found\r\n",fileName);
								bytes = send(ClientSocket, send_buffer, strlen(send_buffer), 0);
								cout<<"<< DEBUG INFO. >>: REPLY sent to CLIENT: "<< send_buffer << endl;			     				
						}else{
								sprintf(send_buffer, "250 Directory Changed\r\n");
								bytes = send(ClientSocket, send_buffer, strlen(send_buffer), 0);
								cout<<"<< DEBUG INFO. >>: REPLY sent to CLIENT: "<< send_buffer << endl;		     				
						}
					}else{
						sprintf(send_buffer, "331 Password required for anonymous\r\n");
						bytes = send(ClientSocket, send_buffer, strlen(send_buffer), 0);
						cout<<"<< DEBUG INFO. >>: REPLY sent to CLIENT: "<< send_buffer << endl;						    
					}
				}else if (strncmp(fileName,"public_folder",13)==0){
					if(chdir(fileName)==-1){
						sprintf(send_buffer, "550 CWD Failed. \"%s\": directory not found\r\n",fileName);
						bytes = send(ClientSocket, send_buffer, strlen(send_buffer), 0);
						cout<<"<< DEBUG INFO. >>: REPLY sent to CLIENT: "<< send_buffer << endl;
					}else{
						sprintf(send_buffer, "250 Directory Changed\r\n");
						bytes = send(ClientSocket, send_buffer, strlen(send_buffer), 0);
						cout<<"<< DEBUG INFO. >>: REPLY sent to CLIENT: "<< send_buffer << endl;
					}
				}else if (strncmp(fileName,"public_folder",13)==0){
					if(chdir(fileName)==-1){
						sprintf(send_buffer, "550 CWD Failed. \"%s\": directory not found\r\n",fileName);
						bytes = send(ClientSocket, send_buffer, strlen(send_buffer), 0);
						cout<<"<< DEBUG INFO. >>: REPLY sent to CLIENT: "<< send_buffer << endl;
					}else{
						sprintf(send_buffer, "250 Directory Changed\r\n");
						bytes = send(ClientSocket, send_buffer, strlen(send_buffer), 0);
						cout<<"<< DEBUG INFO. >>: REPLY sent to CLIENT: "<< send_buffer << endl;
					}
				}else if(strncmp(fileName,"..",2)==0){
					getcwd(path,BUFFER_SIZE);
					if(strncmp(Root,path,strlen(path))==0){
						sprintf(send_buffer, "550 Cannot change root directory\r\n");
						bytes = send(ClientSocket, send_buffer, strlen(send_buffer), 0);
						cout<<"<< DEBUG INFO. >>: REPLY sent to CLIENT: "<< send_buffer << endl;		     		    
					}else{
						if(chdir(fileName)==-1){
								sprintf(send_buffer, "550 CWD Failed. \"%s\": directory not found\r\n",fileName);
								bytes = send(ClientSocket, send_buffer, strlen(send_buffer), 0);
								cout<<"<< DEBUG INFO. >>: REPLY sent to CLIENT: "<< send_buffer << endl;		     				
						}else{
								sprintf(send_buffer, "250 Directory Changed\r\n");
								bytes = send(ClientSocket, send_buffer, strlen(send_buffer), 0);
								cout<<"<< DEBUG INFO. >>: REPLY sent to CLIENT: "<< send_buffer << endl;			     				
						}
					}
				}else {
					sprintf(send_buffer, "550 CWD Failed. \"%s\": directory not found\r\n",fileName);
					bytes = send(ClientSocket, send_buffer, strlen(send_buffer), 0);
					cout<<"<< DEBUG INFO. >>: REPLY sent to CLIENT: "<< send_buffer << endl;
				}
			}
		}
		//=================================================================================	 
		//End of COMMUNICATION LOOP per CLIENT
		//=================================================================================

		//********************************************************************
		//CLOSE SOCKET
		//********************************************************************
		closesocket(ClientSocket);

		cout<<"DISCONNECTED from IP address: "<< clientHost << " Port: "<< clientService<<endl;
		sprintf(send_buffer, "221 Server closing control connection.\r\n");
		printf("<< DEBUG INFO. >>: REPLY sent to CLIENT: %s\n", send_buffer);
		bytes = send(ClientSocket, send_buffer, strlen(send_buffer), 0);

	//====================================================================================
	} //End of MAIN LOOP
	//====================================================================================

	closesocket(ListenSocket);//close listening socket
	WSACleanup(); /* call WSACleanup when done using the Winsock dll */ 

	printf("\nSERVER SHUTTING DOWN...\n");
	return 0;
}


