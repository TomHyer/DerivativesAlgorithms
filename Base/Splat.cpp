
#include "Platform.h"
#include "Splat.h"
#include <map>
#include <iterator>
#include "Strict.h"

#include "Algorithms.h"
#include "Storable.h"
#include "Cell.h"
#include "Dictionary.h"
#include "Exceptions.h"
#include "Functionals.h"
#include "File.h"
#include "Archive.h"
#include "Numerics.h"
#include "StringUtils.h"
#include "DateUtils.h"
#include "DateTimeUtils.h"

using std::map;
using std::shared_ptr;

namespace
{
	static const String_ OBJECT_PREFACE("~");
   static const String_ TAG_PREFACE("$");
	struct XSplat_ : Archive::Store_
	{
      String_ tag_;
		String_ type_; // for storable reader/builder
      // String_ dataType_;
		map<String_, shared_ptr<XSplat_> > children_;
		map<const Storable_*, String_>& sharedTags_;
		Matrix_<Cell_> val_;

      XSplat_(map<const Storable_*, String_>& shared_tags) : sharedTags_(shared_tags) {}

		// output
		int Rows() const
		{
			assert(type_.empty() == children_.empty());
			if (!val_.Empty())
			{
				assert(type_.empty());
				return val_.Rows();
			}
			int retval = 0;
			for (auto c : children_)
				retval += c.second->Rows();
			return retval;
		}
		int Cols() const
		{
			if (!val_.Empty())
				return val_.Cols();
			int retval = 0;
			for (auto c : children_)
				retval = Max(retval, c.second->Cols());
			return 2 + retval;
		}

		void Write
			(Matrix_<Cell_>& dst,
			 int row_offset,
			 int col_offset)
		const
		{
			if (!val_.Empty())
			{
				for (int ir = 0; ir < val_.Rows(); ++ir)

					copy(val_.Row(ir).begin(), val_.Row(ir).end(), dst.Row(ir + row_offset).begin() + col_offset);
				return;
			}
			// OK, we are a composite object
			// first cell is type
			dst(row_offset, col_offset) = tag_ + OBJECT_PREFACE + type_;
			for (auto c : children_)
			{
				dst(row_offset, 1 + col_offset) = c.first;
				c.second->Write(dst, row_offset, 2 + col_offset);
				row_offset += c.second->Rows();
			}
		}

		// input
      bool StoreRef(const Storable_* object) override
      {
         if (auto ot = sharedTags_.find(object); ot != sharedTags_.end())
         {
            SetScalar(ot->second);  // just store the string in lieu of tag
            return true;
         }
         auto tag = TAG_PREFACE + ToString(1 + sharedTags_.size());
         sharedTags_.insert(make_pair(object, tag));
         SetTag(tag);
         return false;
      }

		void SetType(const String_& type) override { type_ = type; }
      void SetTag(const String_& tag) { tag_ = tag; }
		XSplat_& Child(const String_& name) override
		{
			shared_ptr<XSplat_>& retval = children_[name];
			if (!retval.get())
				retval.reset(new XSplat_(sharedTags_));
			return *retval;
		}

		template<class E_> void SetScalar(const E_& e)
		{
			val_.Resize(1, 1);
			val_(0, 0) = e;
		}
		template<class E_> void SetVector(const Vector_<E_>& v)
		{
			val_.Resize(1, v.size());	// vectors are splatted horizontally
			auto dst = val_.Row(0);
			Copy(v, &dst);
		}
		template<class E_> void SetMatrix(const Matrix_<E_>& m)
		{
			val_.Resize(m.Rows(), m.Cols());
			for (int ir = 0; ir < m.Rows(); ++ir)
			{
				auto dst = val_.Row(ir);
				Copy(m.Row(ir), &dst);
			}
		}

		void operator=(double d) override { SetScalar(d); }
		void operator=(const Date_& d) override { SetScalar(d); }
		void operator=(const String_& s) override { SetScalar(s); }
		void operator=(const Vector_<>& v) override { SetVector(v); }
		void operator=(const Vector_<int>& v) override { SetVector(v); }
		void operator=(const Vector_<bool>& v) override { SetVector(v); }
		void operator=(const Vector_<String_>& v) override { SetVector(v); }
		void operator=(const Vector_<Date_>& v) override { SetVector(v); }
		void operator=(const Vector_<DateTime_>& v) override { SetVector(v); }
		void operator=(const Matrix_<>& m) override { SetMatrix(m); }
		void operator=(const Matrix_<Cell_>& m) override { SetMatrix(m); }
		void operator=(const Dictionary_& d) override
		{
			val_.Resize(d.Size(), 2);
			int ir = 0;
			for (auto [k, v] : d)
			{
				val_(ir, 0) = k;
				val_(ir, 1) = v;
				++ir;
			}
		}
	};

	//------------------------------------------------------------------------
	// helper functions for UnSplat
	struct ExtractDouble_
	{
		double operator()(double d) const { return d; }
		double operator()(const String_& s) const { return String::ToDouble(s); }
		template<class T_> double operator()(T_) const { THROW("Can't create a number from a non-numeric type"); }
	};
	double ExtractDouble(const Cell_& src)
	{
		return src.Visit(ExtractDouble_());
	}

	int ExtractInt(const Cell_& src)
	{
		const double d = ExtractDouble(src);
		int retval = static_cast<int>(d);
		REQUIRE(retval == d, "Non-integer value not expected");
		return retval;
	}
	
	struct ExtractBool_
	{
		bool operator()(bool b) const { return b; }
		bool operator()(const String_& s) const { return String::ToBool(s); }
		template<class T_> bool operator()(T_) const { THROW("Can't create a boolean from cell"); }
	};
	bool ExtractBool(const Cell_& src)
	{
		return src.Visit(ExtractBool_());
	}

	struct ExtractString_
	{
		const String_& operator()(const String_& s) const { return s; }
		const String_& operator()(std::monostate) const { static const String_ EMPTY; return EMPTY; }	// should only occur in the interior of a table
		template<class T_> const String_& operator()(T_) const { THROW("Can't create a string from cell"); }
	};
	String_ ExtractString(const Cell_& src)
	{
		return src.Visit(ExtractString_());
	}

	struct ExtractDate_
	{
		Date_ operator()(const String_& s) const { return Date::FromString(s); }
		Date_ operator()(const Date_& dt) const { return dt; }
		Date_ operator()(std::monostate) const { return Date_(); }	// should only occur in the interior of a table
		template<class T_> Date_ operator()(T_) const { THROW("Can't create a date from cell"); }
	};
	Date_ ExtractDate(const Cell_& src)
	{
		return src.Visit(ExtractDate_());
	}

	struct ExtractDateTime_
	{
		DateTime_ operator()(const String_& s) const { return DateTime::FromString(s); }
		DateTime_ operator()(const Date_& dt) const { return DateTime_(dt, 0.0); }	// allow promotion
		DateTime_ operator()(const DateTime_& dt) const { return dt; }
		DateTime_ operator()(std::monostate) const { return DateTime_(); }	// should only occur in the interior of a table
		template<class T_> DateTime_ operator()(T_) const { THROW("Can't create a datetime from cell"); }
	};
	DateTime_ ExtractDateTime(const Cell_& src)
	{
		return src.Visit(ExtractDateTime_());
	}

   template<class R_, class T_> auto TranslateRange(const R_& range, const T_& translate)
      ->Vector_<VALUE_TYPE_OF(translate(Cell_()))>
   {
      Vector_<VALUE_TYPE_OF(translate(Cell_()))> retval;
      transform(range.first, range.second, back_inserter(retval), translate);
      return retval;
   }

	struct XUnSplat_ : Archive::View_
	{
		const Matrix_<Cell_>& data_;	// we share a reference
		int rowStart_;
		int rowStop_;	// the portion of the data we might look at
		int colStart_;
		mutable Vector_<shared_ptr<XUnSplat_> > children_;
      bool quiet_;   

		XUnSplat_(const Matrix_<Cell_>& data, int row_start, int row_stop, int col_start, bool quiet) : data_(data), rowStart_(row_start), rowStop_(row_stop), colStart_(col_start), quiet_(quiet) {}

		String_ Type() const override
		{
			const Cell_& c = data_(rowStart_, colStart_);
			if (auto p = std::get_if<String_>(&c.val_))
				if (auto pt = p->find(OBJECT_PREFACE); pt != String_::npos)
					return p->substr(pt + OBJECT_PREFACE.size());
			return String_();
		}

		String_ Tag() const
		{
			const Cell_& c = data_(rowStart_, colStart_);
			if (auto p = std::get_if<String_>(&c.val_); p && p->substr(0, TAG_PREFACE.size()) == TAG_PREFACE)
				return p->substr(0, p->find(OBJECT_PREFACE));
			return String_();
		}

		const View_& Child(const String_& name) const override
		{
			assert(!Type().empty());   // only composite types have children
			const int nameCol = colStart_ + 1;
			assert(nameCol < data_.Cols());
			for (int ir = rowStart_; ir < rowStop_; ++ir)
			{
				if (data_(ir, nameCol) == name)
				{
					// found it
					int jr = ir + 1;
					while (jr < rowStop_ && Cell::IsEmpty(data_(jr, nameCol)))
						++jr;
					children_.push_back(shared_ptr<XUnSplat_>(new XUnSplat_(data_, ir, jr, nameCol + 1, quiet_)));
					return *children_.back();
				}
			}
			THROW("Child '" + name + "' not found");
		}
		bool HasChild(const String_& name) const override
		{
			assert(!Type().empty());
			const int nameCol = colStart_ + 1;
			assert(nameCol < data_.Cols());
			for (int ir = rowStart_; ir < rowStop_; ++ir)
			{
				if (data_(ir, nameCol) == name)
					return true;
			}
			return false;
		}
      void Unexpected(const String_& child_name) const override
      {
         REQUIRE(quiet_, "Unexpected child '" + child_name + "'; aborting");
      }

		// output atomic values
		const Cell_& GetScalar() const
		{
			REQUIRE(rowStop_ == rowStart_ + 1, "Can't get a scalar value from a multi-line entry");
			REQUIRE(colStart_ == data_.Cols() - 1 || Cell::IsEmpty(data_(rowStart_, colStart_ + 1)), "Can't get a scalar value from a multi-row entry");
			return data_(rowStart_, colStart_);
		}
      int AsInt() const override
      {
         double temp = AsDouble();
         int retval = ::AsInt(temp);
         REQUIRE(retval == temp, "Can't get an integer from a non-integer entry");
         return retval;
      }
		double AsDouble() const override
		{
         return ExtractDouble(GetScalar());
		}
		bool AsBool() const override
		{
			return ExtractBool(GetScalar());
		}
      Date_ AsDate() const override
      {
         return ExtractDate(GetScalar());
      }
      String_ AsString() const override
      {
         return ExtractString(GetScalar());
      }

		pair<Matrix_<Cell_>::Row_::const_iterator, Matrix_<Cell_>::Row_::const_iterator> VectorRange() const
		{
			REQUIRE(rowStop_ == rowStart_ + 1, "Can't get a vector value from a multi-line entry");
			int colStop = colStart_ + 1;
			while (colStop < data_.Cols() && !Cell::IsEmpty(data_(rowStart_, colStop)))
				++colStop;
			return make_pair(data_.Row(rowStart_).begin() + colStart_, data_.Row(rowStart_).begin() + colStop);
		}

		Vector_<> AsDoubleVector() const override
		{
         return TranslateRange(VectorRange(), ExtractDouble);
		}
		Vector_<int> AsIntVector() const override
		{
         return TranslateRange(VectorRange(), ExtractInt);
		}
		Vector_<bool> AsBoolVector() const override
		{
         return TranslateRange(VectorRange(), ExtractBool);
		}
		Vector_<String_> AsStringVector() const override
		{
         return TranslateRange(VectorRange(), ExtractString);
		}
		Vector_<Date_> AsDateVector() const override
		{
         return TranslateRange(VectorRange(), ExtractDate);
		}
		Vector_<DateTime_> AsDateTimeVector() const override
		{
         return TranslateRange(VectorRange(), ExtractDateTime);
		}

		int MatrixStop() const
		{
			for (int retval = colStart_ + 1;;)
			{
				for (int ir = rowStart_;; ++ir)
				{
					if (ir == rowStop_)
						return retval;	// found an empty column
					else if (!Cell::IsEmpty(data_(ir, retval)))
						break;	// column is not empty
				}
				if (++retval == data_.Cols())
					return retval;
			}
		}
		template<class F_> auto TranslateMatrix
			(int col_stop,
			 F_ translate)
		const
      ->Matrix_<VALUE_TYPE_OF(translate(data_(0, 0)))>
      {
         Matrix_<VALUE_TYPE_OF(translate(data_(0, 0)))> retval;
			retval.Resize(rowStop_ - rowStart_, col_stop - colStart_);
			for (int ir = rowStart_; ir < rowStop_; ++ir)
				transform(data_.Row(ir).begin() + colStart_, data_.Row(ir).begin() + col_stop, retval.Row(ir - rowStart_).begin(), translate);
         return retval;
		}

		Matrix_<> AsDoubleMatrix() const override
		{
			return TranslateMatrix(MatrixStop(), ExtractDouble);
		}

		Matrix_<Cell_> AsCellMatrix() const override
		{
			return TranslateMatrix(MatrixStop(), Identity_<Cell_>());
		}

		Dictionary_ AsDictionary() const override
		{
			REQUIRE(MatrixStop() == colStart_ + 2, "Can't extract dictionary because entry does not have two columns");
         Dictionary_ retval;
			for (int ir = rowStart_; ir < rowStop_; ++ir)
				retval.Insert(ExtractString(data_(ir, colStart_)), data_(ir, colStart_ + 1));
         return retval;
		}

		Handle_<Storable_>& Known(Archive::Built_& built) const override
		{
         return built.known_[Tag()];
		}
	};

	Cell_ AsCell(const String_& content)
	{
		if (content.empty())
			return Cell_();
		else if (content.front() == '"' && content.back() == '"')
			return Cell_(content.substr(1, content.size() - 2));
		else if (content == "TRUE")
			return Cell_(true);
		else if (content == "FALSE")
			return Cell_(false);
		else if (content.find_first_not_of("0123456789.eE+-") == String_::npos)
		{
         char* e;
         double retval = std::strtod(content.c_str(), &e);
         if (!*e) // strtod consumed the whole string, success
            return Cell_(retval);
		}
		// just a string
		return Cell_(content);
	}

	Vector_<Cell_> SplitLine(const String_& line)
	{
		Vector_<Cell_> retval;
		auto start = line.begin();
		while (start != line.end())
		{
			// find the next comma not in quotes
			auto stop = start;
			bool quoted = false;
			for (; stop != line.end(); ++stop)
			{
				if (stop == line.end())
					break;
				else if (!quoted && *stop == ',')
					break;
				else if (*stop == '"')
					quoted = !quoted;	// POSTPONED -- deal with nested quotes
			}
			retval.push_back(AsCell(String_(start, stop)));
			start = stop == line.end() ? line.end() : Next(stop);	// skip the comma if we found one
		}
		return retval;
	}

	Matrix_<Cell_> FileTable(const String_& filename)
	{
		Vector_<String_> lines;
		File::Read(filename, &lines);
		Vector_<Vector_<Cell_> > data = Apply(AsFunctor(SplitLine), lines);
		int cols = 0;
		for (auto d : data)
			cols = Max(cols, d.size());
		Matrix_<Cell_> retval(data.size(), cols);
		for (int ii = 0; ii < data.size(); ++ii)
			copy(data[ii].begin(), data[ii].end(), retval.Row(ii).begin());
		return retval;
	}
}	// leave local

Matrix_<Cell_> Splat(const Storable_& src)
{
   map<const Storable_*, String_> tags;
	XSplat_ task(tags);
	src.Write(task);
	Matrix_<Cell_> retval(task.Rows(), task.Cols());
	task.Write(retval, 0, 0);
	return retval;
}

Handle_<Storable_> UnSplat(const Matrix_<Cell_>& src, bool quiet)
{
	XUnSplat_ task(src, 0, src.Rows(), 0, quiet);
	NOTE("Extracting object from splatted data");
   Archive::Built_ built;
	return Archive::Extract(task, built);
}

Handle_<Storable_> UnSplatFile(const String_& filename, bool quiet)
{
	return UnSplat(FileTable(filename), quiet);
}
