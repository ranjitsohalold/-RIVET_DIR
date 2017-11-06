/**********************************************************************
Copyright 2014-2016 The RIVET Developers. See the COPYRIGHT file at
the top-level directory of this distribution.

This file is part of RIVET.

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
**********************************************************************/
/**
 * \class	MapMatrix and related classes
 * \brief	Stores a matrix representing a simplicial map and provides operations for persistence calculations.
 * \author	Matthew L. Wright
 * \date	February 2014
 * 
 * The MapMatrix class stores a matrix representing a simplicial map, such as a boundary map.
 * Such a matrix is a sparse matrix with entries in the two-element field.
 * This implementation is based on that described in the persistent homology survey paper by Edelsbrunner and Harer.
 * Operations are those necessary for persistence computations.
 *
   //TODO: Change the below
 * Implementation details: A vector contains pointers to the first element (possibly null) in each column; each column is represented by a linked list.
 * Linked lists connecting entries in each row are not implemented.
 * Each entry in the matrix is an instance of the MapMatrixNode class.
 *
 * The MapMatrix_Base class provides the basic structures and functionality; it is the parent class and is not meant to be instantiated directly.
 * The class MapMatrix inherits MapMatrix_Base and stores matrices in a column-sparse format, designed for basic persistence calcuations.
 * The class MapMatrix_Perm inherits MapMatrix, adding functionality for row and column permutations; it is designed for the reduced matrices of vineyard updates.
 * Lastly, the class MapMatrix_RowPriority_Perm inherits MapMatrix_Base and stores matrices in a row-sparse format with row and column permutations; it is designed for the upper-triangular matrices of vineyard updates.
 */

#ifndef __MapMatrix_H__
#define __MapMatrix_H__

#include "phat_mod/include/phat/boundary_matrix_mod.h"
#include <ostream> //for testing
#include <vector>

class IndexMatrix;
class FIRep;

//base class simply implements features common to all MapMatrices, whether column-priority or row-priority
//written here using column-priority terminology, but this class is meant to be inherited, not instantiated directly

class MapMatrix_Base {
protected:

    MapMatrix_Base(unsigned rows, unsigned cols); //constructor to create matrix of specified size (all entries zero)
    MapMatrix_Base(unsigned size); //constructor to create a (square) identity matrix
    virtual ~MapMatrix_Base(); //destructor
    
    //Uses the vector_heap_mod representation by default
    phat::boundary_matrix_mod<> matrix; //modified PHAT boundary matrix
    unsigned num_rows; //number of rows in the matrix

    virtual unsigned width() const; //returns the number of columns in the matrix
    virtual unsigned height() const; //returns the number of rows in the matrix
    
    //WARNING: Current Implementation assumes the entry has not already been added.
    virtual void set(unsigned i, unsigned j); //sets (to 1) the entry in row i, column j
    
    //TODO: Remove this
    //virtual void clear(unsigned i, unsigned j); //clears (sets to 0) the entry in row i, column j

    virtual bool entry(unsigned i, unsigned j) const; //returns true if entry (i,j) is 1, false otherwise

    virtual void add_to(unsigned j, unsigned k); //adds column j to column k; RESULT: column j is not changed, column k contains sum of columns j and k (with mod-2 arithmetic)
    
    
    //Set the matrix to the trivial one.
    //TODO: Should num_rows also change?
    //TODO: I think this (and it's related functions) should be removed; it will probably no longer be necessary.
    virtual void clear();
};

//MapMatrix is a column-priority matrix designed for standard persistence calculations
class MapMatrix_Perm;
class MapMatrix : public MapMatrix_Base {
    friend class FIRep;

public:

    MapMatrix(unsigned rows, unsigned cols); //constructor to create matrix of specified size (all entries zero)
    MapMatrix(unsigned size); //constructor to create a (square) identity matrix

    MapMatrix(); // creates an empty MapMatrix
    
    //NOTE: Commenting this out for now.  Constructor probably should be rewritten to be more column-sparse friendly.
    //MapMatrix(std::initializer_list<std::initializer_list<int>>);
    
    virtual unsigned width() const; //returns the number of columns in the matrix
    virtual unsigned height() const; //returns the number of rows in the matrix
    
    //friend std::ostream& operator<<(std::ostream&, const MapMatrix&);

    //Not needed; default will behave correctly.  In any case, do we ever need to compare two map matrices.?
    //virtual bool operator==(MapMatrix& other);

    //requests that the columns vector have enough capacity for num_cols columns
    void reserve_cols(unsigned num_cols); //requests that the columns vector have enough capacity for num_cols columns
    
    //WARNING: Current Implementation assumes the entry has not already been added.
    virtual void set(unsigned i, unsigned j); //sets (to 1) the entry in row i, column j
    
    virtual bool entry(unsigned i, unsigned j) const; //returns true if entry (i,j) is 1, false otherwise
    
    virtual int low(unsigned j) const; //returns the "low" index in the specified column, or -1 if the column is empty
    
    bool col_is_empty(unsigned j) const; //returns true iff column j is empty

    void add_column(unsigned j, unsigned k); //adds column j to column k; RESULT: column j is not changed, column k contains sum of columns j and k (with mod-2 arithmetic)
    
    void add_column(const MapMatrix* other, unsigned j, unsigned k); //adds column j from MapMatrix* other to column k of this matrix
    
    void prepare_col(unsigned i);
    
    //copies column with index src_col from other to column dest_col in this matrix
    void copy_cols_from(const MapMatrix* other, int src_col, int dest_col);
    
    //copies NONZERO columns with indexes in [first, last] from other, appending them to this matrix to the right of all existing columns
    //  all row indexes in copied columns are increased by offset
    //Note: Only needed for the Koszul homology algorithm???
    void copy_cols_from(const MapMatrix* other, int first, int last, unsigned offset);

    //copies columns with indexes in [first, last] from other, inserting them in this matrix with the same column indexes
    void copy_cols_same_indexes(const MapMatrix* other, int first, int last);

    //removes zero columns from this matrix
    //  ind_old gives grades of columns before zero columns are removed; new grade info stored in ind_new
    void remove_zero_cols(IndexMatrix* ind_old, IndexMatrix* ind_new);
    
    //FOR TESTING ONLY
    //virtual void print(); //prints the matrix to standard output (for testing)

    //check for inconsistencies in matrix column, for testing purposes
    //void assert_cols_correct();
    
    
    //returns a boundary matrix for hom_dim-simplices with columns in a specified order -- for vineyard-update algorithm
    MapMatrix_Perm* get_permuted_and_trimmed_mx(const std::vector<int>& coface_order, unsigned num_simplices);
    
    //returns a boundary matrix for (hom_dim+1)-simplices with columns and rows a specified orders -- for vineyard-update algorithm
    MapMatrix_Perm* get_permuted_and_trimmed_mx(const std::vector<int>& face_order, unsigned num_faces, const std::vector<int>& coface_order, const unsigned num_cofaces);
    
    void finalize(unsigned i);
    
    void print();
    
    //TODO: Remove me, no longer necessary
    virtual void clear();
};

//MapMatrix with row/column permutations and low array, designed for "vineyard updates"
class MapMatrix_RowPriority_Perm; //forward declaration
class MapMatrix_Perm : public MapMatrix {
public:
    MapMatrix_Perm(unsigned rows, unsigned cols);
    MapMatrix_Perm(unsigned size);
    
    //Defaults shoudl work correctly
    //MapMatrix_Perm(const MapMatrix_Perm& other); //copy constructor
    //~MapMatrix_Perm();
    
    //TODO: Replace this?  I can't remember if it is still necessary. -Mike.
    //void set(unsigned i, unsigned j); //sets (to 1) the entry in row i, column j
    
    bool entry(unsigned i, unsigned j) const; //returns true if entry (i,j) is 1, false otherwise

    //reduces this matrix, fills the low array, and returns the corresponding upper-triangular matrix for the RU-decomposition
    //  NOTE: only to be called before any rows are swapped!
    MapMatrix_RowPriority_Perm* decompose_RU();
    
    int low(unsigned j) const; //returns the "low" index in the specified column, or -1 if the column is empty
    int find_low(unsigned l) const; //returns the index of the column with low l, or -1 if there is no such column

    void swap_rows(unsigned i, bool update_lows); //transposes rows i and i+1, optionally updates low array
    void swap_columns(unsigned j, bool update_lows); //transposes columns j and j+1, optionally updates low array
    
    //TODO: Add lazy versions of these functions which don't redo the whole computation
    //clears the matrix, then rebuilds it from reference with columns permuted according to col_order
    //NOTE: This funciton and the next do not remove columns or rows.
    void rebuild(MapMatrix_Perm* reference, const std::vector<unsigned>& col_order);

    //clears the matrix, then rebuilds it from reference with columns permuted according to col_order and rows permuted according to row_order
    void rebuild(MapMatrix_Perm* reference, const std::vector<unsigned>& col_order, const std::vector<unsigned>& row_order);
    
    ///FOR TESTING ONLY
    //virtual void print(); //prints the matrix to standard output (for testing)
    //void check_lows(); //checks for inconsistencies in low arrays

protected:
    std::vector<unsigned> perm; //permutation vector
    std::vector<unsigned> mrep; //inverse permutation vector
    std::vector<int> low_by_row; //stores index of column with each low number, or -1 if no such column exists -- NOTE: only accurate after decompose_RU() is called
    std::vector<int> low_by_col; //stores the low number for each column, or -1 if the column is empty -- NOTE: only accurate after decompose_RU() is called
};

//MapMatrix stored in row-priority format, with row/column permutations, designed for upper-triangular matrices in vineyard updates
class MapMatrix_RowPriority_Perm : public MapMatrix_Base {
public:
    MapMatrix_RowPriority_Perm(unsigned size); //constructs the identity matrix of specified size
    //MapMatrix_RowPriority_Perm(const MapMatrix_RowPriority_Perm& other); //copy constructor
    ~MapMatrix_RowPriority_Perm();

    unsigned width() const; //returns the number of columns in the matrix
    unsigned height() const; //returns the number of rows in the matrix

    //void set(unsigned i, unsigned j); //sets (to 1) the entry in row i, column j
    //void clear(unsigned i, unsigned j); //clears (sets to 0) the entry in row i, column j
    bool entry(unsigned i, unsigned j) const; //returns true if entry (i,j) is 1, false otherwise

    void add_row(unsigned j, unsigned k); //adds row j to row k; RESULT: row j is not changed, row k contains sum of rows j and k (with mod-2 arithmetic)

    void swap_rows(unsigned i); //transposes rows i and i+1
    void swap_columns(unsigned j); //transposes columns j and j+1

    ///FOR TESTING ONLY
    //void print(); //prints the matrix to qDebug() for testing
    //void print_perm(); //prints the permutation vectors to qDebug() for testing

protected:
    std::vector<unsigned> perm; //permutation vector
    std::vector<unsigned> mrep; //inverse permutation vector
};

#endif // __MapMatrix_H__
