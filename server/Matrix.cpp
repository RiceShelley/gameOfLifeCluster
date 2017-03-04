#include "Matrix.hpp"

Matrix::Matrix(int width, int hieght) : rows(hieght), colums(width)
{
	std::cout << "Matrix created" << std::endl;
	matrixRim = new char[(rows * 2) + (colums * 2)];
	updatedMatrix = false;
}

Matrix::Matrix() 
{
	std::cout << "Matrix created" << std::endl;
}

void Matrix::setMatrix(char** newGrid)
{
	grid = newGrid;		
}

void Matrix::setMatrix(char* str)
{
	int i = 0;
	for (int row = 0; row < rows; row++)
	{
		for (int col = 0; col < colums; col++)
		{
			grid[row][col] = str[i];
			i++;
		}
	}
}

void Matrix::setRim(char* rim)
{
	strcpy(matrixRim, rim);
}

char* Matrix::getRim()
{
	return matrixRim;
}

char** Matrix::getMatrix()
{
	return grid;	
}

void Matrix::printMatrix()
{
	for (int row = 0; row < rows; row++)
	{
		for (int col = 0; col < colums; col++)
		{
			printf("%c", grid[row][col]);
		}
		printf("\n");
	}
}

std::string Matrix::toString()
{
	std::string str_matrix;
	for (int row = 0; row < rows; row++)
	{
		for (int col = 0; col < colums; col++)
		{
			str_matrix += grid[row][col];
		}
	}
	return str_matrix;
}

int Matrix::getRows()
{
	return rows;
}

int Matrix::getColums()
{
	return colums;
}

bool Matrix::get_updatedMatrix()
{
	return updatedMatrix;
}

void Matrix::set_updatedMatrix(bool b)
{
	updatedMatrix = b;
}
