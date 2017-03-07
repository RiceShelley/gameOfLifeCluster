/*
 * =====================================================================================
 *
 *       Filename:  Main.c
 *
 *    Description:  game of life 
 *
 *        Version:  1.0
 *        Created:  08/07/2016 03:10:27 AM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  Rice Shelley
 *   Organization:  
 *
 * =====================================================================================
 */
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <stdbool.h>
#include <sys/socket.h>
#include <string.h>
#include <resolv.h>
#include <errno.h>

// Board dimentions <- set by server 
int gridW = 5;
int gridH = 5;
int gridA = 25;

// 2d array of chars for holding game of life board
char** grid;
char** tempGrid;

// flag requesting server matrix resize
bool resizeReq = false;

// Socket descripter
int sockfd = 0;

void writeMatrix(char* newMatrix)
{
	int i = 0;
	for (int row = 0; row < gridH; row++)
	{
		for (int col = 0; col < gridW; col++)
		{
			grid[row][col] = newMatrix[i];
			i++;
		}
	}	
}

/* 
*   Returns 2d character array givin a width and height 
*   array is loaded onto the heap MUST BE MANUALLY DELETED
*/
char** allocateMatrix(int width, int height)
{ 
	char** grid = calloc((height + 1), sizeof(char*)); 
	for (int i = 0; i < (height + 1); i++) {
		grid[i] = calloc((width + 1), sizeof(char));
	}
	return grid;
}

void deallocateMatrix(char** matrix, int height) {
    for (int i = 0; i < (height + 1); i++) {
        free(matrix[i]);
    }
    free(matrix);
}

void send_server(int cID, char *buff, int len) 
{
    len += 2;
    char s_buff[len];
    memset(s_buff, '\0', len);
    strcpy(s_buff, buff);
    strcat(s_buff, "!");
    send(cID, s_buff, len, 0);
}

// read until '!' or until "len" bytes are read
void recv_server(int cID, char *buff, int len)
{
    memset(buff, '\0', len);
    while (true) {
        char tempBuff[len];
        int b = recv(cID, tempBuff, len, 0);
        // check for '!'
        for (int i = 0; i < b; i++) {
            if (tempBuff[i] == '!') {
                strncat(buff, tempBuff, i);
                return;
            }
        }
        strncat(buff, tempBuff, b);
    }
}

char* getMatrixRim()
{
	char* rim = (char* ) malloc(((gridW * 2) + (gridH * 2) + 1));
	int i = 0;
	int col = 0;
	int row = 1;
	// assign top then bottom rows followed by left then right
	for (int n = 0; n < 2; n++)
	{
		for (col = 0; col < gridW; col++)
		{
			rim[i] = grid[row][col];
			i++;
		}
		row = (gridH - 2);
	}
	col = 1;
	for (int n = 0; n < 2; n++)
	{
		for (row = 0; row < gridH; row++)
		{
			rim[i] = grid[row][col];
			i++;
		}
		col = (gridW - 2);
	}
	rim[i] = '\0';
	return rim;
}

void clearGrid()
{
	for (int row = 0; row < gridH; row++)
	{
		for (int col = 0; col < gridW; col++)
		{
			if (row == 0 || row == (gridH - 1) || col == 0 || col == (gridW -1)) 
				grid[row][col] = '*';
			else
				grid[row][col] = ' ';
		}
	}
}

void print2dArray(char **grid, int rows, int cols) 
{
	for (int row = 0; row < rows; row++)
	{
		for (int col = 0; col < cols; col++)
		{
			printf("%c", grid[row][col]);
		}
		printf("\n");
	}
}

int testCell(int row, int col, char **tempGrid)
{
	// do neighbour count
	if (tempGrid[row][col] == '#') {
		if ((col == 2 && (row > 1 && row < gridH - 2)) || 
		(col == gridW - 3 && (row > 1 && row < gridH)) ||
		(row == 2 && (col > 1 && col < gridW - 2)) || 
		(row == gridH - 3 && (col > 1 && col < gridW - 2))) {
			/* 
			* do check for undefined data character
			* undefined data could effect the sim if a life cell nears it 
			* in the case of a live cell near a undefined data character
			* the board needs to be resized to allow the sim to acuratly continue
			*/
			if (tempGrid[row + 2][col - 2] == '~') {
				printf("1\n");
				resizeReq = true;
			} else if (tempGrid[row][col - 2] == '~') {
				resizeReq = true;
				printf("2\n");
			} else if (tempGrid[row - 2][col - 2] == '~') {
				resizeReq = true;
				printf("3\n");
			} else if (tempGrid[row - 2][col] == '~') {
				resizeReq = true;
				printf("4\n");
			} else if (tempGrid[row - 2][col + 2] == '~') {
				resizeReq = true;
				printf("5\n");
			} else if (tempGrid[row][col + 2] == '~') { 
				resizeReq = true;
				printf("6\n");
			} else if (tempGrid[row + 2][col + 2] == '~') {
				resizeReq = true;
				printf("7\n");
			} else if (tempGrid[row + 2][col] == '~') {
				resizeReq = true;	
				printf("8\n");
			}
		}
	}
	int neighbours = 0;
	if (tempGrid[row + 1][col - 1] == '#') 
		neighbours++;
	if (tempGrid[row][col - 1] == '#')
		neighbours++;
	if (tempGrid[row - 1][col - 1] == '#')
		neighbours++;
	if (tempGrid[row - 1][col] == '#')
		neighbours++;
	if (tempGrid[row - 1][col + 1] == '#')
		neighbours++;
	if (tempGrid[row][col + 1] == '#')
		neighbours++;
	if (tempGrid[row + 1][col + 1] == '#')
		neighbours++;
	if (tempGrid[row + 1][col] == '#')
		neighbours++;
	return neighbours;
}

void step()
{
    printf("inside step\n");
	// init temp grid to equal current grid
	char rimN1_Z[(gridW * 2) + (gridH * 2)];
	char rim1_Z[(gridW * 2) + (gridH * 2)];
	char rimZ_N1[(gridW * 2) + (gridH * 2)];
	char rimZ_1[(gridW * 2) + (gridH * 2)];
	char rimN1_N1[(gridW * 2) + (gridH * 2)];
	char rimN1_1[(gridW * 2) + (gridH * 2)];
	char rim1_1[(gridW * 2) + (gridH * 2)];
	char rim1_N1[(gridW * 2) + (gridH * 2)];

	// copy curent board into temp array
	for (int row = 0; row < gridH; row++)
	{
		for (int col = 0; col < gridW; col++)
		{
			tempGrid[row][col] = grid[row][col];
		}
    }
    // get rim of matrix above this one
    send_server(sockfd, "GETRIMOF:-1/0", (sizeof("GETRIMOF:-1/0") / sizeof(char)));
    memset(rimN1_Z, 0, (sizeof(rimN1_Z) / sizeof(char)));
    recv_server(sockfd, rimN1_Z, (sizeof(rimN1_Z) / sizeof(char)));
    char bottomRow[gridW];
    memset(bottomRow, 0, gridW);
    strcpy(rimN1_Z, (const char*) &rimN1_Z[gridW]);
    strncpy(bottomRow, rimN1_Z, gridW);
    for (int i = 0; i < gridW; i++)
    {
        grid[0][i] = bottomRow[i];
    }
    // get rim of matrix bellow this one
    send_server(sockfd, "GETRIMOF:1/0", (sizeof("GETRIMOF:1/0") / sizeof(char)));
    memset(rim1_Z, 0, (sizeof(rim1_Z) / sizeof(char)));
    recv_server(sockfd, rim1_Z, (sizeof(rim1_Z) / sizeof(char)));
    char topRow[gridW];
    memset(topRow, 0, gridW);
    strncpy(topRow, rim1_Z, gridW);
    for (int i = 0; i < gridW; i++)
    {
        grid[gridH - 1][i] = topRow[i];
    }
    // get rim of matrix to the left of this one
    send_server(sockfd, "GETRIMOF:0/-1", (sizeof("GETRIMOF:0/-1") / sizeof(char)));
    memset(rimZ_N1, 0, (sizeof(rimZ_N1) / sizeof(char)));
    recv_server(sockfd, rimZ_N1, (sizeof(rimZ_N1) / sizeof(char)));
    char rightRow[gridH];
    memset(rightRow, 0, gridH);
    strcpy(rimZ_N1, (const char*) &rimZ_N1[((gridW * 2) + gridH)]);
    strcpy(rightRow, rimZ_N1);
    for (int i = 0; i < gridH; i++)
    {
        grid[i][0] = rightRow[i];
    }
	// get rim of matrix to the right of this one
    send_server(sockfd, "GETRIMOF:0/1", (sizeof("GETRIMOF:0/1") / sizeof(char)));
    memset(rimZ_1, 0, (sizeof(rimZ_1) / sizeof(char)));
    recv_server(sockfd, rimZ_1, (sizeof(rimZ_1) / sizeof(char)));
    char leftRow[gridH];
    memset(leftRow, 0, gridH);
    strncpy(leftRow, (const char*) &rimZ_1[(gridW * 2)], gridH);
    for (int i = 0; i < gridH; i++) 
    {
        grid[i][gridW - 1] = leftRow[i];
    } 
	// Add edges of surrounding matrices to this one if need be

    // get rim of matrix above and the the left
    send_server(sockfd, "GETRIMOF:-1/-1", (sizeof("GETRIMOF:-1/-1") / sizeof(char)));
    memset(rimN1_N1, 0, (sizeof(rimN1_N1) / sizeof(char)));
    recv_server(sockfd, rimN1_N1, (sizeof(rimN1_N1) / sizeof(char)));
    // top left corner
    grid[0][0] = rimN1_N1[(gridW * 2) - 2];

    // get rim of matrix above and to the right
    send_server(sockfd, "GETRIMOF:-1/1", (sizeof("GETRIMOF:-1/1") / sizeof(char)));
    memset(rimN1_1, 0, (sizeof(rimN1_1) / sizeof(char)));
    recv_server(sockfd, rimN1_1, (sizeof(rimN1_1) / sizeof(char)));
    // top right corner
    grid[0][gridW - 1] = rimN1_1[gridW + 1];

    // get rim of matrix bellow and to the right
    send_server(sockfd, "GETRIMOF:1/1", (sizeof("GETRIMOF:1/1") / sizeof(char)));
    memset(rim1_1, 0, (sizeof(rim1_1) / sizeof(char)));
    recv_server(sockfd, rim1_1, (sizeof(rim1_1) / sizeof(char)));
    // bot right corner
    grid[gridH - 1][gridW - 1] = rim1_1[1];

	// get rim of matrix bellow and to the left
    send_server(sockfd, "GETRIMOF:1/-1", (sizeof("GETRIMOF:1/-1") / sizeof(char)));
    memset(rim1_N1, 0, (sizeof(rim1_N1) / sizeof(char)));
    recv_server(sockfd, rim1_N1, (sizeof(rim1_N1) / sizeof(char)));
    // bot left corner
    grid[gridH - 1][0] = rim1_N1[gridW - 2];
    
    printf("welp\n");

	//print2dArray(grid, gridH, gridW);

	// nt3dA logic to grid
	for (int row = 1; row < gridH - 1; row++)
	{
		for (int col = 1; col < gridW - 1; col++)
		{
			int neighbours = testCell(row, col, grid);
			if (neighbours == 2 || neighbours == 3)
			{
				if (grid[row][col] == ' ' && neighbours == 3)
				{
					tempGrid[row][col] = '#';	
				}
			}
			else 
			{
				tempGrid[row][col] = ' ';
			}
		}
	}

	// set grid to contents of tempGrid
	for (int row = 0; row < gridH; row++)
	{
		for (int col = 0; col < gridW; col++)
		{
			grid[row][col] = tempGrid[row][col];
		}
	}
	if (resizeReq) {
		printf("RESIZE DEAR GOD!\n");
		send_server(sockfd, "RESIZE_REQ\0", 11);
        resizeReq = false;
	}
	send_server(sockfd, "STEP_DONE\0", 10);	
}

void procControl()
{
	// Connect to server
	struct sockaddr_in serv_def;
	int portNum = 9002;
	sockfd = socket(AF_INET, SOCK_STREAM, 0);

	serv_def.sin_family = AF_INET;
	serv_def.sin_port = htons(portNum);
	inet_aton((const char*) "192.168.1.174", &serv_def.sin_addr.s_addr);
	// connect to server
	connect(sockfd, (struct sockaddr*)&serv_def, sizeof(serv_def));
    int buff_pad = 100;
	while(true) 
	{
        
        int fromS_ln = gridA + buff_pad;
	    char fromS[fromS_ln];
		recv_server(sockfd, fromS, fromS_ln);
        printf("from server -> %s\n", fromS);
		// Server requested node to run next step of the simulation
		if (strncmp(fromS, "STEP", 4) == 0)
		{
			step();
		}
		// Server has sent node a new matrix
		else if (strncmp(fromS, "DATASET:", 8) == 0)
		{
			char* dataSet = &fromS[8];
			// Parse data set
			writeMatrix(dataSet);
			send_server(sockfd, "SYNC_DONE\0", 10);
		}
        // Server is resizing matrix
        else if (strncmp(fromS, "SET_DIMEN:", 10) == 0) {
            // Free mem from old grids 
            deallocateMatrix(grid, gridH);
            deallocateMatrix(tempGrid, gridH);
			// Parse dimentions of grid
            char* dimen = &fromS[10]; 
			char cY[5];
			memset(cY, 0, 5);
			strncpy(cY, (const char*)dimen, (strlen(dimen) - strlen(strchr(dimen, '/'))));
			char* cX = &strchr(dimen, '/')[1];
			gridH = atoi(cY);
			gridW = atoi(cX);
            gridA = (gridH * gridW);
            printf("new dimen h = %d, w = %d", gridH, gridW);
            // Create new grids with new dimensions
            grid = allocateMatrix(gridW, gridH);
            tempGrid = allocateMatrix(gridW, gridH);
            clearGrid();
        }
		/*
		 * server has reqested the matrix this node has
		 * been processing!!!
		 * possible resons: 
		 * - server is building a master matrix of the current point in the simulation
		 */
		else if (strncmp(fromS, "REQDATASET", 10) == 0)
		{
			char matrix_str[((gridW * gridH) + 13)];
			strcpy(matrix_str, "DATASET:");
			int i = 8;
			for (int row = 0; row < gridH; row++)
			{
				for (int col = 0; col < gridW; col++)
				{
					matrix_str[i] = grid[row][col];
					i++;
				}
			}
			matrix_str[i] = '\0';
            printf("%s\n", matrix_str);
            send_server(sockfd, matrix_str, strlen(matrix_str));
			//send(sockfd, matrix_str, strlen(matrix_str), 0);
		}
		/* 
		 * server has requested the rim of this matrix
		 * reason: another node processing an adjacent 
		 * matrix needs to know the postion of cells 
		 * that could possibly affect its simulation
		 */
		else if (strncmp(fromS, "GETRIM", 6) == 0)
		{
			char* rim = getMatrixRim();
			char* rimFormat = (char*) malloc(((int) strlen(rim)) + ((int) strlen("MATRIXRIM\0")) + 1);
			strcpy(rimFormat, ((const char*) "MATRIXRIM"));
			strcat(rimFormat, ((const char*) rim));
            printf("%s\n", rimFormat);
			send_server(sockfd, rimFormat, strlen(rimFormat));
			free(rimFormat);
			free(rim);
		}
		else if (strcmp(fromS, "") == 0)
		{
			printf("the world just ended\n");
			exit(1);
		}
	}
}

int main() 
{
    grid = allocateMatrix(gridW, gridH);
    tempGrid = allocateMatrix(gridW, gridH);
	// Init grid by clearing
	clearGrid();	
	// Start process loop
	procControl();
}
