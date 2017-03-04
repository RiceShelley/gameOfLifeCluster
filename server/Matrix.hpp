#include <iostream>
#include <stdio.h>
#include <string.h>
#include <vector>
class Matrix {

	public:
		Matrix(int width, int height);
		Matrix();
		void printMatrix();
		std::string toString();
		char** getMatrix();
		char* getRim();
		int getRows();
		int getColums();
		bool get_updatedMatrix();
		void set_updatedMatrix(bool b);
		void setMatrix(char** newGrid);
		void setMatrix(char* str);
		void setRim(char* rim);
	private:
		int rows;
		int colums;
		char** grid;
		char* matrixRim;
		// Boolean for keeping matrix rims in serv mem updated
		bool updatedMatrix;
};
