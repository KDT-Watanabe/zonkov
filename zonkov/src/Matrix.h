#pragma once
#include <vector>
#include <iostream>
using namespace std;

class ID {
	int kind;
	int id;
};

class Matrix {
	

	vector<vector<ID>> matrix;

	size_t rows = matrix.size();
	size_t cols = matrix.size();

	
	ID GetValue(size_t row,size_t col) {
		ID value = matrix[row][col];
		return value;
	}
};