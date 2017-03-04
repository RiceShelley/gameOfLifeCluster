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

// Board dimentions
int gridW = 5;
int gridH = 5;

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

void printGrid() 
{
	//system("clear");
	for (int row = 0; row < gridH; row++)
	{
		for (int col = 0; col < gridW; col++)
		{
			if (row == 0 || row == (gridH - 1) || col == 0 || col == (gridW -1)) 
				grid[row][col] = '*';
			printf("%c", grid[row][col]);
		}
		printf("\n");
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
    printf("row = %i col = %i\n", row, col);
    printf("gridW = %i gridH = %i\n", gridW, gridH);
    printf("seg\n");
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
    printf("fault\n");
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
    printf("fault\n");
	return neighbours;
}

void step()
{
	// flags for identifing if certain parts of other matrixes are required for this matrixs computation
	// edges
	bool hasRight = true;
	bool hasLeft = true;
	bool hasTop = true;
	bool hasBot = true;
	// corners
	bool hasTopLeft = true;
	bool hasTopRight = true;
	bool hasBotLeft = true;
	bool hasBotRight = true;

	// init temp grid to equal current grid
	char rimN1_Z[(gridW * 2) + (gridH * 2)];
	char rim1_Z[(gridW * 2) + (gridH * 2)];
	char rimZ_N1[(gridW * 2) + (gridH * 2)];
	char rimZ_1[(gridW * 2) + (gridH * 2)];
	char rimN1_N1[(gridW * 2) + (gridH * 2)];
	char rimN1_1[(gridW * 2) + (gridH * 2)];
	char rim1_1[(gridW * 2) + (gridH * 2)];
	char rim1_N1[(gridW * 2) + (gridH * 2)];

    printf("before seg\n");

	// copy curent board into temp array
	for (int row = 0; row < gridH; row++)
	{
		for (int col = 0; col < gridW; col++)
		{
			tempGrid[row][col] = grid[row][col];
			/* check if grid is going to need data from other rims toDo make this work
			if (tempGrid[row][col] == '#') {
				if (row == 1 && col == 1) {
					hasTopLeft = true;
				} else if (row == 1 && col == gridW - 2) {
					hasTopRight = true;
				} else if (row == gridH - 2 && col == 1) {
					hasBotLeft = true;
				} else if (row == gridH - 2 && col == gridW - 2){
					hasBotRight = true;
				} else if (row == 1) {
					hasTop = true;
				} else if (row == gridH - 2) {
					hasBot = true;	
				} else if (col == 1) {
					hasLeft = true;
				} else if (col == gridW -1) {
					hasRight = true;
				}
			}*/
		}
	}
	if (hasTop) {
		// get rim of matrix above this one
		send(sockfd, "GETRIMOF:-1/0\0", (sizeof("GETRIMOF:-1/0\0") / sizeof(char)), 0);
		memset(rimN1_Z, 0, (sizeof(rimN1_Z) / sizeof(char)));
		recv(sockfd, rimN1_Z, (sizeof(rimN1_Z) / sizeof(char)), 0);
		char bottomRow[gridW];
		memset(bottomRow, 0, gridW);
		strcpy(rimN1_Z, (const char*) &rimN1_Z[gridW]);
		strncpy(bottomRow, rimN1_Z, gridW);
		for (int i = 0; i < gridW; i++)
		{
			grid[0][i] = bottomRow[i];
		}
	}
	if (hasBot) {
		// get rim of matrix bellow this one
		send(sockfd, "GETRIMOF:1/0\0", (sizeof("GETRIMOF:1/0\0") / sizeof(char)), 0);
		memset(rim1_Z, 0, (sizeof(rim1_Z) / sizeof(char)));
		recv(sockfd, rim1_Z, (sizeof(rim1_Z) / sizeof(char)), 0);
		char topRow[gridW];
		memset(topRow, 0, gridW);
		strncpy(topRow, rim1_Z, gridW);
		for (int i = 0; i < gridW; i++)
		{
			grid[gridH - 1][i] = topRow[i];
		}
	}
	if (hasLeft) {
		// get rim of matrix to the left of this one
		send(sockfd, "GETRIMOF:0/-1\0", (sizeof("GETRIMOF:0/-1\0") / sizeof(char)), 0);
		memset(rimZ_N1, 0, (sizeof(rimZ_N1) / sizeof(char)));
		recv(sockfd, rimZ_N1, (sizeof(rimZ_N1) / sizeof(char)), 0);
		char rightRow[gridH];
		memset(rightRow, 0, gridH);
		strcpy(rimZ_N1, (const char*) &rimZ_N1[((gridW * 2) + gridH)]);
		strcpy(rightRow, rimZ_N1);
		for (int i = 0; i < gridH; i++)
		{
			grid[i][0] = rightRow[i];
		}
	}
	if (hasRight) {
		// get rim of matrix to the right of this one
		send(sockfd, "GETRIMOF:0/1\0", (sizeof("GETRIMOF:0/1\0") / sizeof(char)), 0);
		memset(rimZ_1, 0, (sizeof(rimZ_1) / sizeof(char)));
		recv(sockfd, rimZ_1, (sizeof(rimZ_1) / sizeof(char)), 0);
		char leftRow[gridH];
		memset(leftRow, 0, gridH);
		strncpy(leftRow, (const char*) &rimZ_1[(gridW * 2)], gridH);
		for (int i = 0; i < gridH; i++) 
		{
			grid[i][gridW - 1] = leftRow[i];
		} 
	}
	// Add edges of surrounding matrices to this one if need be
	if (hasTopLeft) {
		// get rim of matrix above and the the left
		send(sockfd, "GETRIMOF:-1/-1\0", (sizeof("GETRIMOF:-1/-1\0") / sizeof(char)), 0);
		memset(rimN1_N1, 0, (sizeof(rimN1_N1) / sizeof(char)));
		recv(sockfd, rimN1_N1, (sizeof(rimN1_N1) / sizeof(char)), 0);
		// top left corner
		grid[0][0] = rimN1_N1[28];
	}
	if (hasTopRight) {
		// get rim of matrix above and to the right
		send(sockfd, "GETRIMOF:-1/1\0", (sizeof("GETRIMOF:-1/1\0") / sizeof(char)), 0);
		memset(rimN1_1, 0, (sizeof(rimN1_1) / sizeof(char)));
		recv(sockfd, rimN1_1, (sizeof(rimN1_1) / sizeof(char)), 0);
		// top right corner
		grid[0][gridW - 1] = rimN1_1[16];
	}
	if (hasBotRight) {
		// get rim of matrix bellow and to the right
		send(sockfd, "GETRIMOF:1/1\0", (sizeof("GETRIMOF:1/1\0") / sizeof(char)), 0);
		memset(rim1_1, 0, (sizeof(rim1_1) / sizeof(char)));
		recv(sockfd, rim1_1, (sizeof(rim1_1) / sizeof(char)), 0);
		// bot right corner
		grid[gridH - 1][gridW - 1] = rim1_1[1];
	}
	if (hasBotLeft) {
		// get rim of matrix bellow and to the left
		send(sockfd, "GETRIMOF:1/-1\0", (sizeof("GETRIMOF:1/-1\0") / sizeof(char)), 1);
		memset(rim1_N1, 0, (sizeof(rim1_N1) / sizeof(char)));
		recv(sockfd, rim1_N1, (sizeof(rim1_N1) / sizeof(char)), 0);
		// bot left corner
		grid[gridH - 1][0] = rim1_N1[13];
	}	

    printf("before seg0\n");

	print2dArray(grid, gridH, gridW);

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

    printf("before seg1\n");    

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
		send(sockfd, "RESIZE_REQ\0", 11, 0);
        resizeReq = false;
	}
	send(sockfd, "STEP_DONE\0", 10, 0);	
}

void procControl()
{
	// Connect to server
	struct sockaddr_in serv_def;
	int portNum = 9002;
	sockfd = socket(AF_INET, SOCK_STREAM, 0);

	serv_def.sin_family = AF_INET;
	serv_def.sin_port = htons(portNum);
	inet_aton((const char*) "192.168.1.30", &serv_def.sin_addr.s_addr);
	// connect to server
	connect(sockfd, (struct sockaddr*)&serv_def, sizeof(serv_def));
	char fromS[1000 * 10];
	int fromSLn = (sizeof(fromS) / sizeof(char));
	memset(fromS, '\0', fromSLn);
	while(true) 
	{
		
		memset(fromS, '\0', fromSLn);
		recv(sockfd, fromS, fromSLn, 0);
		// Server requested node to run next step of the simulation
		if (strncmp(fromS, "STEP", 4) == 0)
		{
			step();
			//printGrid();
		}
		// Server has sent node a new matrix
		else if (strncmp(fromS, "DATASET:", 8) == 0)
		{
			char* dataSet = &fromS[8];
			// Parse data set
			writeMatrix(dataSet);
			send(sockfd, "SYNC_DONE\0", 10, 0);
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
			send(sockfd, matrix_str, strlen(matrix_str), 0);
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
			send(sockfd, rimFormat, strlen(rimFormat), 0);
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
