
#pragma once

#include "Vectors.h"
#include "Algorithms.h"

#include "Strings.h"	// because String_ is not (yet) a proper class

template<class E_> class Matrix_	// default is supplied in Platform.h
{
	Vector_<E_> vals_;
	int cols_;
	using I_ = typename Vector_<E_>::iterator;
	Vector_<I_> hooks_;

	void SetHooks(int from = 0)
	{
		for (int ii = from; ii < hooks_.size(); ++ii)
			hooks_[ii] = vals_.begin() + ii * cols_;
	}
public:
	Matrix_() : cols_(0) {}
	Matrix_(int rows, int cols) : vals_(rows* cols, E_{}), cols_(cols), hooks_(rows) { SetHooks(); }
	Matrix_(const Matrix_<E_>& src) : vals_(src.vals_), cols_(src.cols_), hooks_(src.hooks_.size()) { SetHooks(); }
	void operator=(const Matrix_<E_>& rhs) { vals_ = rhs.vals_; cols_ = rhs.cols_; hooks_.Resize(rhs.hooks_.size()); SetHooks(); }
	Matrix_(Matrix_<E_>&& src) : vals_(std::move(src.vals_)), cols_(src.cols_), hooks_(std::move(src.hooks_)) { }

	int Rows() const { return hooks_.size(); }
	int Cols() const { return cols_; }
	bool Empty() const { return vals_.empty(); }
	void Clear() { vals_.clear(); cols_ = 0; hooks_.clear(); }
	typename Vector_<E_>::const_iterator Last() const { return vals_.end(); }    // used to detect aliasing

	// Fortran-style addressing for maximum speed
	typename Vector_<E_>::const_reference operator()(int row, int col) const
	{
		return hooks_[row][col];
	}
	typename Vector_<E_>::reference operator()(int row, int col)
	{
		return hooks_[row][col];
	}

	// Slices -- ephemeral containers of rows or columns
	class ConstRow_
	{
	protected:
		I_ begin_, end_;    // non-const to support Row_, below
	public:
		using value_type = E_;
		using const_iterator = typename Vector_<E_>::const_iterator;
		// construct from begin/end or from begin/size
		ConstRow_(I_ begin, I_ end) : begin_(begin), end_(end) {}
		ConstRow_(I_ begin, int size) : begin_(begin), end_(begin + size) {}

		const_iterator begin() const { return begin_; }
		const_iterator end() const { return end_; }
		size_t size() const { return static_cast<int>(end_ - begin_); }
		const E_& operator[](int col) const { return *(begin_ + col); }
	};
	ConstRow_ Row(int i_row) const { return ConstRow_(hooks_[i_row], cols_); }
	ConstRow_ operator[](int i_row) const { return Row(i_row); }    // C-style access

	struct Row_ : ConstRow_
	{
		// inherit data members and const_iterator
		using iterator = I_;
		Row_(I_ begin, I_ end) : ConstRow_(begin, end) {}	
		Row_(I_ begin, int size) : ConstRow_(begin, size) {}

		// have to double-implement begin/end, otherwise non-const implementations hide the inherited const
		iterator begin() { return ConstRow_::begin_; }
		typename ConstRow_::const_iterator begin() const { return ConstRow_::begin_; }
		iterator end() { return ConstRow_::end_; }
		typename ConstRow_::const_iterator end() const { return ConstRow_::end_; }
		E_& operator[](size_t col) { return *(ConstRow_::begin_ + col); }
		const E_& operator[](size_t col) const { return *(ConstRow_::begin_ + col); }
	};
	Row_ Row(int i_row) { return Row_(hooks_[i_row], cols_); }
	Row_ operator[](int i_row) { return Row(i_row); }    // C-style access

	// Iteration through columns is less efficient
	template<class RI_> class ColBase_
	{
	public:
		struct Iterator_    // column iterator in terms of row iterator
		{
			RI_ val_;
			size_t stride_;
			Iterator_(RI_ val, size_t stride) : val_(val), stride_(stride) {}
			Iterator_(const Iterator_&) = default;

			Iterator_& operator++() { val_ += stride_; return *this; }
			Iterator_ operator++(int) { Iterator_ ret(*this); val_ += stride_; return ret; }
			Iterator_& operator--() { val_ -= stride_; return *this; }
			Iterator_ operator--(int) { Iterator_ ret(*this); val_ -= stride_; return ret; }
			Iterator_ operator+(size_t inc) { Iterator_ ret(*this); ret.val_ += inc * stride_; return ret; }
			bool operator==(const Iterator_& rhs) const { assert(stride_ == rhs.stride_); return val_ == rhs.val_; }
			auto operator<=>(const Iterator_& rhs) const { assert(stride_ == rhs.stride_); return val_.operator<=>(rhs.val_); }
			typename RI_::difference_type operator-(const Iterator_& rhs) const { assert(stride_ == rhs.stride_); assert((val_ - rhs.val_) % stride_ == 0); return (val_ - rhs.val_) / stride_; }

			typename RI_::reference operator*() { return *val_; }
			using value_type = E_;
		};
	protected:
		Iterator_ begin_;
		size_t size_;
	public:
		using value_type = E_;
		ColBase_(I_ begin, size_t size, size_t stride) : begin_(begin, stride), size_(size) {}

		Iterator_ begin() const { return begin_; }
		Iterator_ end() const { return Iterator_(begin_.val_ + size_ * begin_.stride_, begin_.stride_); }
		size_t size() const { return size_; }
		const E_& operator[](int row) const { return *(begin_.val_ + row * begin_.stride_); }
	};

	using ConstCol_ = ColBase_<typename Vector_<E_>::const_iterator>;
	ConstCol_ Col(int i_col) const { return ConstCol_(hooks_[0] + i_col, hooks_.size(), cols_); }

	using Col_ = ColBase_<typename Vector_<E_>::iterator>;
	Col_ Col(int i_col) { return Col_(hooks_[0] + i_col, hooks_.size(), cols_); }

	// POSTPONED -- submatrix

	void Swap(Matrix_<E_>* other)
	{
		assert(other != nullptr);
		vals_.Swap(&other->vals_);
		hooks_.Swap(&other->hooks_);    // works because pointers inside vals_ are swapped
		std::swap(cols_, other->cols_);
		assert(hooks_.empty() || hooks_.front() == vals_.begin());
	}
	void Fill(const E_& val) { vals_.Fill(val); }
	template<class T_> void operator*=(const T_& scale) { vals_ *= scale; }
	void operator+=(const E_& shift) { vals_ += shift; }
	void Resize(int rows, int cols)
	{
		const int oldRows = hooks_.size();
		if (cols == cols_)    // easy case
		{
			vals_.Resize(rows * cols);
			hooks_.Resize(rows);
			SetHooks(oldRows);
		}
		else    // brute force
		{
			const int nCopy = Min(cols, cols_);
			cols_ = cols;
			Vector_<E_> newVals(rows * cols);
			for (int ir = 0; ir < rows && ir < oldRows; ++ir)
			{
				std::copy(hooks_[ir], hooks_[ir] + nCopy, newVals.begin() + ir * cols);
			}
			vals_.Swap(&newVals);
			hooks_.Resize(rows);
			SetHooks();
		}
	}
};

