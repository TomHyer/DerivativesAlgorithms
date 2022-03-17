
#include "Platform.h"
#include "MatrixUtils.h"
#include "Strict.h"

#include "Cell.h"
#include "Matrix.h"
#include "Exceptions.h"

namespace
{
	typedef Matrix_<Cell_> Table_;

	struct WriterView_
	{
		mutable Table_* dst_;
		int rowOffset_;
		int colOffset_;
		// linear mapping from input coordinate to coordinate in dst_
		int r2r_, r2c_, c2r_, c2c_;

		WriterView_(Table_* dst) : dst_(dst), rowOffset_(0), colOffset_(0), r2r_(1), r2c_(0), c2r_(0), c2c_(1) {}

		void Write(int i_row, int i_col, const Cell_& val) const
		{
			const int dstRow = rowOffset_ + r2r_ * i_row + c2r_ * i_col;
			const int dstCol = colOffset_ + r2c_ * i_row + c2c_ * i_col;
			(*dst_)(dstRow, dstCol) = val;
		}

		WriterView_ Transpose() const
		{
			WriterView_ retval(*this);
			std::swap(retval.r2r_, retval.c2r_);
			std::swap(retval.r2c_, retval.c2c_);
			return retval;
		}
		WriterView_ Invert(int n_rows, int n_cols) const
		{
			WriterView_ retval(*this);
			retval.rowOffset_ += r2r_ * (n_rows - 1) + c2r_ * (n_cols - 1);
			retval.colOffset_ += r2c_ * (n_rows - 1) + c2c_ * (n_cols - 1);
			retval.r2r_ *= -1;
			retval.r2c_ *= -1;
			retval.c2r_ *= -1;
			retval.c2c_ *= -1;
			return retval;
		}
		WriterView_ Shift(int row_offset, int col_offset) const
		{
			WriterView_ retval(*this);
			retval.rowOffset_ += r2r_ * row_offset + c2r_ * col_offset;
			retval.colOffset_ += r2c_ * row_offset + c2c_ * col_offset;
			return retval;
		}
		WriterView_ Flatten(int n_cols) const
		{
			WriterView_ retval(*this);
			retval.r2r_ = n_cols * retval.c2r_;
			retval.r2c_ = n_cols * retval.c2c_;
			return retval;
		}
	};


	struct Writer_ : noncopyable
	{
		virtual ~Writer_() {}
		virtual int Rows(const Vector_<const Table_*>& args) const = 0;
		virtual int Cols(const Vector_<const Table_*>& args) const = 0;
		virtual void Write
			(const WriterView_& dst,
			 const Vector_<const Table_*>& args)
		const = 0;
	};

	struct TransposedWriter_ : Writer_
	{
		scoped_ptr<Writer_> base_;
		TransposedWriter_(Writer_* base) : base_(base) {}
		int Rows(const Vector_<const Table_*>& args) const { return base_->Cols(args); }
		int Cols(const Vector_<const Table_*>& args) const { return base_->Rows(args); }
		void Write
			(const WriterView_& dst,
			 const Vector_<const Table_*>& args)
		const
		{
			base_->Write(dst.Transpose(), args);
		}
	};

	struct InvertedWriter_ : Writer_
	{
		scoped_ptr<Writer_> base_;
		InvertedWriter_(Writer_* base) : base_(base) {}
		int Rows(const Vector_<const Table_*>& args) const { return base_->Rows(args); }
		int Cols(const Vector_<const Table_*>& args) const { return base_->Cols(args); }
		void Write
			(const WriterView_& dst,
			 const Vector_<const Table_*>& args)
		const
		{
			base_->Write(dst.Invert(Rows(args), Cols(args)), args);
		}
	};

	struct LinearWriter_ : Writer_	// puts everything in a single row
	{
		scoped_ptr<Writer_> base_;
		LinearWriter_(Writer_* base) : base_(base) {}
		int Rows(const Vector_<const Table_*>&) const { return 1; }
		int Cols(const Vector_<const Table_*>& args) const { return base_->Rows(args) * base_->Cols(args); }
		void Write
			(const WriterView_& dst,
			 const Vector_<const Table_*>& args)
		const
		{
			base_->Write(dst.Flatten(base_->Cols(args)), args);
		}
	};

	struct HorizontalWriter_ : Writer_	// writes args left-to-right, justifies to top; transpose to make a vertical writer
	{
		Vector_<Handle_<Writer_> > elements_;
		int Rows(const Vector_<const Table_*>& args) const
		{
			int retval = 0;
			for (auto e : elements_)
				retval = Max(retval, e->Rows(args));
			return retval;
		}
		int Cols(const Vector_<const Table_*>& args) const
		{
			int retval = 0;
			for (auto e : elements_)
				retval += e->Cols(args);
			return retval;
		}
		void Write
			(const WriterView_& dst,
			 const Vector_<const Table_*>& args)
		const
		{
			WriterView_ temp(dst);
			for (auto e : elements_)
			{
				e->Write(temp, args);
				temp = temp.Shift(0, e->Cols(args));
			}
		}
	};

	struct VerticalWriter_ : Writer_	// writes args left-to-right, justifies to top; transpose to make a horizontal writer
	{
		Vector_<Handle_<Writer_> > elements_;
		int Cols(const Vector_<const Table_*>& args) const
		{
			int retval = 0;
			for (auto e : elements_)
				retval = Max(retval, e->Cols(args));
			return retval;
		}
		int Rows(const Vector_<const Table_*>& args) const
		{
			int retval = 0;
			for (auto e : elements_)
				retval += e->Rows(args);
			return retval;
		}
		void Write
			(const WriterView_& dst,
			 const Vector_<const Table_*>& args)
		const
		{
			WriterView_ temp(dst);
			for (auto e : elements_)
			{
				e->Write(temp, args);
				temp = temp.Shift(e->Rows(args), 0);
			}
		}
	};

	struct EmptyCell_ : Writer_
	{
		int Rows(const Vector_<const Table_*>&) const { return 1; }
		int Cols(const Vector_<const Table_*>&) const { return 1; }
		void Write
			(const WriterView_& dst,
			 const Vector_<const Table_*>&)
		const
		{
			dst.Write(0, 0, Cell_());
		}
	};

	struct ArgWriter_ : Writer_
	{
		int whichArg_;
		ArgWriter_(int which) : whichArg_(which) {}
		int Rows(const Vector_<const Table_*>& args) const
		{
			return whichArg_ < args.size()
				? args[whichArg_]->Rows()
				: 0;
		}
		int Cols(const Vector_<const Table_*>& args) const
		{
			return whichArg_ < args.size()
				? args[whichArg_]->Cols()
				: 0;
		}
		void Write
			(const WriterView_& dst,
			 const Vector_<const Table_*>& args)
		const
		{
			if (whichArg_ >= args.size())
				return;
			const Table_& src = *args[whichArg_];
			for (int ir = 0; ir < src.Rows(); ++ir)
				for (int ic = 0; ic < src.Cols(); ++ic)
					dst.Write(ir, ic, src(ir, ic));
		}
	};


	Vector_<String_> Split
		(const String_& src,
		char sep)
	{
		Vector_<String_> retval(1, String_());
		int depth = 0;
		for (auto s : src)
		{
			if (s == sep && depth == 0)
				retval.push_back(String_());
			else
			{
				retval.back().push_back(s);
				if (s == '(')
					++depth;
				if (s == ')')
					--depth;
			}
		}
		return retval;
	}

	String_ Strip(const String_& src)	// remove parentheses around the whole thing
	{
		auto ps = src.begin();
		for (int depth = 0; ps != src.end(); ++ps)
		{
			if (*ps == '(')
				++depth;
			else if (*ps == ')')
				--depth;
			if (depth == 0)
				break;
		}
		return (ps == src.end() - 1 && *ps == ')')	// parentheses wrap the whole thing
			? Strip(src.substr(1, src.size() - 2))
			: src;
	}

	Writer_* NewWriter(const String_& format)
	{
		Vector_<String_> subs = Split(Strip(format), ';');
		if (subs.size() > 1)	// it's a semicolon-separated list
		{
			std::unique_ptr<VerticalWriter_> retval(new VerticalWriter_);
			for (auto s : subs)
				retval->elements_.push_back(Handle_<Writer_>(NewWriter(s)));
			return retval.release();
		}
		// ok, no unparenthesized semicolons
		subs = Split(Strip(format), ',');
		if (subs.size() > 1)	// it's a sequence
		{
			std::unique_ptr<HorizontalWriter_> retval(new HorizontalWriter_);
			for (auto s : subs)
				retval->elements_.push_back(Handle_<Writer_>(NewWriter(s)));
			return retval.release();
		}
		// no commas:  just one element
		const String_& e = subs[0];
		if (toupper(e.back()) == 'T')
			return new TransposedWriter_(NewWriter(e.substr(0, e.size() - 1)));
		else if (toupper(e.back()) == 'I')
			return new InvertedWriter_(NewWriter(e.substr(0, e.size() - 1)));
		else if (e.back() == '*')
			return new LinearWriter_(NewWriter(e.substr(0, e.size() - 1)));
		else
		{
			REQUIRE(e.size() == 1 && e.front() >= '0' && e.front() <= '9', "Can't recognize format element -- expected argument index (format = '" + format + "')");
			return e.front() == '0'
				? (Writer_*) new EmptyCell_
				: new ArgWriter_(e.front() - '1');	// implements 1-offset count of args
		}
	}
}	// leave local

Matrix_<Cell_> Matrix::Format
	(const Vector_<const Table_*>& args,
	 const String_& format)
{
	scoped_ptr<Writer_> writer(NewWriter(format));
	Matrix_<Cell_> retval(writer->Rows(args), writer->Cols(args));
	REQUIRE(retval.Rows() * retval.Cols() > 0, "Nothing to output");
	writer->Write(WriterView_(&retval), args);
	return retval;
}

