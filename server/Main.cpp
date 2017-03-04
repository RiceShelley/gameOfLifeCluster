#include <iostream>
#include <pthread.h>
#include <sys/socket.h>
#include <resolv.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include "Matrix.hpp"

// Master matrix dimentions (master matrix is 2d array of matrix objects representing nodes)
#define MASTER_MATRIX_W 4
#define MASTER_MATRIX_H 3
#define AMT_OF_NODES (MASTER_MATRIX_W * MASTER_MATRIX_H)

// Dimentions of 2d char array matrix held on nodes
int matrixW = 15;
int matrixH = 8;
int matrixArea = (matrixW * matrixH);

// Flag to signal that the a nodes matrix need to be resized 
bool resizeMatrix = false;

// Mutex for acessing shared data in memory 
pthread_mutex_t rMutex;

/* 
*   Returns 2d character array givin a width and height 
*   array is loaded onto the heap MUST BE MANUALLY DELETED
*/
char** allocateMatrix(const int width, const int height)
{ 
	char** grid = new char*[width]; 
	for (int i = 0; i < width; i++) {
		grid[i] = new char[height];
	}
	return grid;
}

/*
*   Returns a matrix object givin a 2d chararacter array and its dimentions
*   Matrix object is loaded onto the heap MUST BE MANUALLY DELETED!
*/
Matrix* constructMatrix(char** matrix, const int width, const int height)
{
	// Format 2d char array in such a way that the client code can process (aka create a '*' border)
	for (int col = 0; col < width; col++)
	{
		for (int row = 0; row < height; row++)
		{
            // If border index set eq to '*'
			if (row == 0 || row == (height - 1) || col == 0 || col == (width - 1))
			{
				matrix[row][col] = '*';
			}
			else 
			{
				matrix[row][col] = ' ';
			}
		}
	}
	usleep(1000 * 250);
	// Create matrix object
    Matrix* matrixObj = new Matrix(width, height);	
	// Store 2d char array into new matrix object
	matrixObj->setMatrix(matrix);
	return matrixObj;
}

// Structure that is passed into client thread
struct ClientData 
{
	// Socket descriptor
	int clientID;
	// Ref to 2d array of matrix objects
	Matrix** mMatrix;
	// Positon of matrix object in 2d array of matrix objects
	int posCol_in_matrix;
	int posRow_in_matrix;
	// Dimentions of 2d array of matrix objects
	int mMatrix_width;
	int mMatrix_height;
};

// print a 2D char array
void printMatrix(Matrix** mMatrix, const int rows, const int cols)
{
	int subMatrixRow = 0;
	for (int row = 0; row < rows; row++)
	{
		for (int i = 0; i < mMatrix[0][0].getRows(); i++)
		{
			for (int col = 0; col < cols; col++)
			{
				Matrix* matrix = &mMatrix[row][col];
				for (int col2 = 0; col2 < matrix->getColums(); col2++)
				{
					printf("%c", matrix->getMatrix()[subMatrixRow][col2]);
				}
			}
			subMatrixRow++;
			printf("\n");	
		}
		subMatrixRow = 0;
	}
}

void syncNode(struct ClientData* clientData) {
	// Buffer for handling socket output from node
	const int nodeIn_ln = 1000 * 10;
	char nodeIn[nodeIn_ln];
	Matrix* matrix = &clientData->mMatrix[clientData->posRow_in_matrix][clientData->posCol_in_matrix];
	// Send 2d char array matrix to node
	std::string str_matrix = "DATASET:" + matrix->toString() + "\0";
	send(clientData->clientID, str_matrix.c_str(), strlen(str_matrix.c_str()), 0); 
	// Wait for node to respond with confimation that it recved its matrix
	recv(clientData->clientID, nodeIn, (sizeof(nodeIn) / sizeof(char)), 0);
	if (strcmp(nodeIn, "SYNC_DONE") != 0)
	{
		printf("A catastrophic error has occured, all hope is lost.\nProc exiting...\n");
		exit(-1);
	}
}

char** reconnstructMasterMatrix(struct ClientData* clusterNodeData) 
{
    // matrixH / matrixW - 2 beacuse node matrix borders will not be added into the completed matrix
    int cMatrixH = (MASTER_MATRIX_H * (matrixH - 2));
    int cMatrixW = (MASTER_MATRIX_W * (matrixW - 2));
    char** cMatrix = allocateMatrix(cMatrixH,cMatrixW); 
    for (int i = 0; i < AMT_OF_NODES; i++) {
        char matrix_str[((matrixW * matrixH) + strlen("DATASET:\0"))];
        send(clusterNodeData[i].clientID, "REQDATASET\0", strlen("REQDATASET\0"), 0);
        recv(clusterNodeData[i].clientID, matrix_str, (sizeof(matrix_str) / sizeof(char)), 0); 
        if (strncmp(matrix_str, "DATASET:", 8) == 0) {
            char* matrix_strP = &matrix_str[8];
            // add one to skip the '*' border
            int rowStart = (clusterNodeData[i].posRow_in_matrix * (matrixH - 2));
            int colStart = (clusterNodeData[i].posCol_in_matrix * (matrixW - 2));
            int i = matrixW;
            for (int row = rowStart; row < (rowStart + matrixH) - 2; row++) {
                for (int col = colStart; col < (colStart + matrixW) - 2; col++) {
                    // omit border characters
                    while (matrix_strP[i] == '*') 
                        i++;
                    cMatrix[row][col] = matrix_strP[i];
                    i++;
                }
            }
        } 
        else 
        {
            printf("corrupt data? -> on reconnstruction of master matrix\n");
        }
    }
    for (int row = 0; row < cMatrixH; row++) {
        for (int col = 0; col < cMatrixW; col++) {
            printf("%c", cMatrix[row][col]);
        }
        printf("\n");
    }
    return cMatrix;
}

static void* syncNodeRim(void* p) {
	struct ClientData* clientData = ((ClientData*) p);
	// Buffer for handling socket output from node
	const int nodeIn_ln = 1000 * 10;
	char nodeIn[nodeIn_ln];
	Matrix* matrix = &clientData->mMatrix[clientData->posRow_in_matrix][clientData->posCol_in_matrix];
	// Update matrix rim in serv mem
	memset(nodeIn, '\0', nodeIn_ln);
	send(clientData->clientID, "GETRIM\0", 7, 0);
	recv(clientData->clientID, nodeIn, nodeIn_ln, 0);
	if (strncmp(nodeIn, "MATRIXRIM", 9) != 0)
	{
		printf("this is bad very bad\n");
		printf("data read '%s'\n", nodeIn);
		syncNodeRim(clientData);
		pthread_exit(NULL);
	}
	matrix->setRim(&nodeIn[9]);
	pthread_exit(NULL);
}

// Function for mataining node on seperate thread. param -> ClientData struct declared near the top of this file
static void* stepNode(void *p)
{
	// Buffer for handling socket output from node
	const int nodeIn_ln = 1000 * 10;
	char nodeIn[nodeIn_ln];
	// Convert void pointer back into struct
	struct ClientData* clientData = ((ClientData*) p);
	Matrix* matrix = &clientData->mMatrix[clientData->posRow_in_matrix][clientData->posCol_in_matrix];
	// Begin step
	send(clientData->clientID, "STEP\0", 5, 0);
	do 
	{
		memset(nodeIn, '\0', nodeIn_ln);
		recv(clientData->clientID, nodeIn, nodeIn_ln, 0);
		// Node will request rims of surounding matrixs so it can acuratly complet simulation 
		if (strncmp(nodeIn, "GETRIMOF:", 9) == 0)
		{
			// Parse pos of matrix requested 
			char* cMatCords = &nodeIn[9];
			char cY[5];
			memset(cY, 0, 5);
			strncpy(cY, (const char*)cMatCords, (strlen(cMatCords) - strlen(strchr(cMatCords, '/'))));
			char* cX = &strchr(cMatCords, '/')[1];
			int row = atoi(cY);
			int col = atoi(cX);
			// Get matrix requested via offset from current node pos
			int posR = clientData->posRow_in_matrix + row;
			int posC = clientData->posCol_in_matrix + col;
			// If matrix is not a valid pos send back empty rim and prepare to resize matrix
			if (posR < 0 || posC < 0 || posR >= clientData->mMatrix_height || posC >= clientData->mMatrix_width)
			{
				// generate blank rim	
				int len = ((matrix->getRows() * 2) + (matrix->getColums() * 2));
				char rim[len];
				for (int i = 0; i < len; i++)
				{
					// ~ is used in this implementation of conways game of life to desinate undifined data 
					rim[i] = '~';
				}
				send(clientData->clientID, rim, len, 0);
			}
			else 
			{
				// Send back requested rim
				int len = ((matrix->getRows() * 2) + (matrix->getColums() * 2));
				char rim[len];
				strcpy(rim, clientData->mMatrix[posR][posC].getRim());
				send(clientData->clientID, rim, len, 0);
			}
		} else if (strcmp(nodeIn, "RESIZE_REQ") == 0) {
            printf("ran\n");
            // Set resize matrix flag to true
            pthread_mutex_lock(&rMutex);
	        resizeMatrix = true;	
            pthread_mutex_unlock(&rMutex);
        }
	} while(strncmp(nodeIn, "STEP_DONE\0", 10) != 0);
	pthread_exit(NULL);
}

int main() 
{
	// Keeps track of how many steps the nodes have taken
	int globalStep = 0;
	// Array of node data structs
	struct ClientData clusterClientData[AMT_OF_NODES];
	// Create 2d array of matrix object pointers
	Matrix** masterMatrix = new Matrix*[MASTER_MATRIX_H];	
	for (int row = 0; row < MASTER_MATRIX_H; row++)
	{
		masterMatrix[row] = new Matrix[MASTER_MATRIX_W];
	}
	// Assign and init matrix object
	for (int row = 0; row < MASTER_MATRIX_H; row++)
	{
		for (int col = 0; col < MASTER_MATRIX_W; col++)
		{
            // Create the matrix objects matrix in mem 
			char** matrix = allocateMatrix(matrixW, matrixH);
            // Assine values to the matrix array
			masterMatrix[row][col] = *constructMatrix(matrix, matrixW, matrixH);
            // Print it so we can see it worked
			masterMatrix[row][col].printMatrix();
		}
	}
    printf("made it to here\n");
	// init mutex for acessing shared data withen the threads
	if (pthread_mutex_init(&rMutex, NULL) != 0) {
		printf("failed to init rMutex\n");
		return -1;
	}

	// Start tcp sock serv <- program will act as a beowolf cluster meaning this process will be the master
    // and 'slave' computers will connect to this server and be used for processing 
	struct sockaddr_in socket_def;

	int sock = socket(AF_INET, SOCK_STREAM, 0);

	socket_def.sin_family = AF_INET;
	socket_def.sin_port = htons(9002);
	socket_def.sin_addr.s_addr = INADDR_ANY;

    // bind tcp socket
	if (bind(sock, (struct sockaddr*) &socket_def, sizeof(socket_def)) < 0)
	{
		printf("failed to bind socket.\n>.<\n");
		exit(-1);
	}
    // set socket to listen for incoming connections
	listen(sock, 1);

	// Pos of matrix object in matrix object 2d array the node will be assigned 
	int givinMatrixRow = 0;
	int givinMatrixCol = 0;

    char** grid = masterMatrix[0][1].getMatrix();
    // TODO read in start board from some output file 
	grid[3][6] = '#';
	grid[4][6] = '#';
	grid[5][6] = '#';
	grid[5][5] = '#';
	grid[4][4] = '#';
	
	/*grid[3][6] = '#';
	grid[3][5] = '#';
	grid[4][5] = '#';
	grid[5][5] = '#';
	grid[4][4] = '#';*/

	/*grid[6][5] = '#';
	grid[6][6] = '#';
	grid[6][7] = '#';*/
	
	// Loop untill all 12 nodes have connected
	while (true)
	{
		struct sockaddr_in client_addr;	
		socklen_t sock_len = sizeof(client_addr);
		// Listen for client
		int clientID = accept(sock, (struct sockaddr*) &client_addr, &sock_len);
		printf("got connection\n");
		// Create struct of data that the node needs
		struct ClientData clientData;
		// Socket ID
		clientData.clientID = clientID;
		// Pointer to matrix segment
		clientData.mMatrix = masterMatrix;
		// Pos of matrix segment in master matrix
		clientData.posCol_in_matrix = givinMatrixCol;
		clientData.posRow_in_matrix = givinMatrixRow;
		// Dimentions of master matrix
		clientData.mMatrix_width = MASTER_MATRIX_W;
		clientData.mMatrix_height = MASTER_MATRIX_H;
		// Node step
		if (givinMatrixRow < 1) {
			clusterClientData[((givinMatrixRow * (MASTER_MATRIX_W - 1)) + givinMatrixCol)] = clientData;
		} else {
			clusterClientData[((givinMatrixRow * MASTER_MATRIX_W) + givinMatrixCol)] = clientData;
		}

		if (givinMatrixRow == 2 && givinMatrixCol != 3) {
			givinMatrixRow = 0;
			givinMatrixCol++;
			continue;
		}
		if (givinMatrixRow == 2 && givinMatrixCol == 3)
		{
			break;
		}
		givinMatrixRow++;
	}

	for (int i = 0; i < sizeof(clusterClientData) / sizeof(struct ClientData); i++)
	{
		syncNode(&clusterClientData[i]);
	}	

	pthread_t threads[(sizeof(clusterClientData) / sizeof(struct ClientData))];
	// Maintain timing of node steps 
	while (true)
	{
		// sync node rims
		for (int i = 0; i < AMT_OF_NODES; i++)
		{
			int rtn = -1;
			do {
				if ((rtn = pthread_create(&threads[i], NULL, syncNodeRim, &clusterClientData[i])) != 0) {
					printf("failed to create pthread on node (%i, %i)\n", clusterClientData[i].posRow_in_matrix, clusterClientData[i].posCol_in_matrix);
				}
			} while (rtn != 0);

		}
		// wait for all threads to finish
		for (int i = 0; i < AMT_OF_NODES; i++)
		{
			pthread_join(threads[i], NULL);	
		}
		// step nodes
		for (int i = 0; i < AMT_OF_NODES; i++)
		{
			int rtn = -1;
			do {
				if((rtn = pthread_create(&threads[i], NULL, stepNode, &clusterClientData[i])) != 0) {
					printf("failed to create pthread on node (%i, %i)\n", clusterClientData[i].posRow_in_matrix, clusterClientData[i].posCol_in_matrix);
				}
			} while (rtn != 0);
		}
		// wait for all threads to finish
		for (int i = 0; i < AMT_OF_NODES; i++)
		{
			pthread_join(threads[i], NULL);	
		}
        globalStep++;
        if (resizeMatrix) {
            printf("risize detected\n");
            exit(0);
        }
		usleep(1000 * 100);
		
        if ((globalStep % 100) == 0) {
			if ((globalStep % 100000) == 0)
			{
				printf("globalStep = %i\n", globalStep);
			}
			char** grid = clusterClientData[0].mMatrix[clusterClientData[0].posRow_in_matrix][clusterClientData[0].posCol_in_matrix].getMatrix();
			grid[2][1] = '#';
			grid[3][6] = '#';
			grid[4][6] = '#';
			grid[5][6] = '#';
			grid[5][5] = '#';
			grid[4][4] = '#';
			syncNode(&clusterClientData[0]);
		}
        // gather all noe data and compile into one big array
        if ((globalStep % 1) == 0) {
            reconnstructMasterMatrix(clusterClientData);
        }

	}
    pthread_mutex_destroy(&rMutex);
    return 0;
}
