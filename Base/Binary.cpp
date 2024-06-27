
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
typedef unsigned char uint8;

namespace
{
	constexpr uint8 MonomorphicHeader(int which, int dim, int stride_extra_bytes)
	{
		// dimension 0 to 3: 2 bits
		// stride 1 to 4 bytes big: 2 bits
		// kind of data: 4 bits
		return static_cast<uint8>(dim | (stride_extra_bytes << 2) | (which << 4));
	}
	enum
	{
		M_NONE,
		M_DOUBLE,
		M_INT,
		M_BOOL,
		M_DATE,
		M_DATETIME,
		M_CSTR,
		M_CELL,
		M_TYPE,
		M_TAG
	};

	int StrideExtraBytes(int size)
	{
		int retval = 0;
		while (size >> 8)
		{
			++retval;
			size >>= 8;
		}
		return retval;
	}

	constexpr uint8 C_NULL = 0;

	// binary store must support Push(const char* start, size_t count, size_t size);
		// the pushed memory (count * size bytes) must be copied by the store
		// on big-endian architectures, size must be used to reverse bytes if size > 1
		// size > 1 may also trigger skipping some bytes to match alignment

	template<class B_, class V_> inline void Append(B_& dst, V_ val)
	{
		// POSTPONED -- support this only for true atomic types
		dst.Push(&val, 1, sizeof(V_));
	}

	template<class B_> void SetDouble(B_& dst, double d)
	{
		constexpr uint8 HEADER = MonomorphicHeader(M_DOUBLE, 0, 0);
		Append(dst, HEADER);
		Append(dst, d);
	}
	template<class B_> void SetInt(B_& dst, int i, int kind = M_INT)
	{
		Append(dst, MonomorphicHeader(kind, 0, 0));
		Append(dst, i);
	}
	template<class B_> void SetBool(B_& dst, bool b)
	{
		constexpr uint8 HEADER = MonomorphicHeader(M_BOOL, 0, 0);
		Append(dst, HEADER);
		Append(dst, b);
	}
	template<class B_> void SetDate(B_& dst, const Date_& d)
	{
		constexpr uint8 HEADER = MonomorphicHeader(M_DATE, 0, 0);
		Append(dst, HEADER);
		Append(dst, Date::ToExcel(d));
	}
	template<class B_> void SetDateTime(B_& dst, const DateTime_& d)
	{
		constexpr uint8 HEADER = MonomorphicHeader(M_DATETIME, 0, 0);
		Append(dst, HEADER);
		Append(dst, NumericValueOf(d));
	}
	template<class B_> void SetString(B_& dst, const String_& s, int kind = M_CSTR)
	{
		constexpr uint8 HEADER = MonomorphicHeader(kind, 0, 0);
		Append(dst, HEADER);
		dst.Push(&s[0], s.size(), 1);
		Append(dst, C_NULL);
	}

	template<class B_> struct SetCell_
	{
		B_& dst_;
		SetCell_(B_& dst) : dst_(dst) {}

		void operator()(double d) const { SetDouble(dst_, d); }
		double operator()(bool b) const { SetBool(dst_, b); }
		double operator()(const Date_& dt) const { SetDate(dst_, dt); }
		double operator()(const DateTime_& dt) const { SetDateTime(dst_, dt); }
		double operator()(const String_& s) const { SetString(dst_, s); }
		void operator()(std::monostate) const 
		{
			constexpr uint8 HEADER = MonomorphicHeader(M_NONE, 0, 0);
			Append(dst_, HEADER);
		}

		template<class E_> double operator()(E_) const { THROW("Unrecognized cell type"); }
	};

	template<class B_, bool verbose = false>
	struct XBinary_ : Archive::Store_
	{
		B_& dst_;
		std::map<void*, int> sharedTags_;

		XBinary_(B_& dst) : dst_(dst) 
		{	}

		bool StoreRef(const Storable_* object) override
		{
			if (auto ot = sharedTags_.find(object); ot != sharedTags_.end())
			{
				SetInt(dst_, ot->second, M_TAG);  // just store the tagging int, not the object
				return true;
			}
			auto tag = 1 + sharedTags_.size();
			sharedTags_.insert(make_pair(object, tag));
			SetInt(dst_, tag, M_TAG);
			return false;
		}

		void SetType(const String_& type) override { SetString(dst_, type, M_TYPE); }
		Store_& Child(const String_&) override { return *this; }

		void AddNullChild(const String_&) override { SetInt(dst_, 0, M_NONE); }

		void operator=(double d) override { SetDouble(dst_, d); }
		void operator=(const Date_& d) override { SetDate(dst_, d); }
		void operator=(const String_& s) override { SetString(dst_, s); }

		template<class E_> void SetVectorHeader(const Vector_<E_>& v, int which)
		{
			int n = v.size(), nExtra = StrideExtraBytes(n);
			Append(dst_, MonomorphicHeader(which, 1, nExtra));
			for (int ii = 0; ii <= nExtra; ++ii)
			{
				Append(dst_, static_cast<uint8>(n & 0xFF));
				n >>= 8;
			}
		}
		template<class E_> void SetVector(const Vector_<E_>& v, int which)
		{
			SetVectorHeader(v, which);
			for (auto x : v)
				Append(dst_, x);
		}
		void operator=(const Vector_<>& val) override { SetVector(val, M_DOUBLE); }
		void operator=(const Vector_<int>& val) override { SetVector(val, M_INT); }
		void operator=(const Vector_<bool>& val) override 
		{	// POSTPONED -- pack bool vectors
			SetVectorHeader(val, M_BOOL); 
			for (auto b : val)
				Append(dst_, b ? '1' : '0');
		}
		void operator=(const Vector_<String_>& val) override
		{
			SetVectorHeader(val, M_CSTR);
			for (const auto& s : val)
			{
				dst_.Push(&s[0], s.size(), 1);
				Append(dst_, C_NULL);
			}
		}
		void operator=(const Vector_<Date_>& val) override 
		{ 
			SetVectorHeader(val, M_DATE); 
			for (const auto& d : val)
				Append(dst_, Date::ToExcel(d));
		}
		void operator=(const Vector_<DateTime_>& val) override
		{
			SetVectorHeader(val, M_DATETIME);
			for (const auto& d : val)
				Append(dst_, NumericValueOf(d));
		}

		template<class E_> void SetMatrixHeader(int rows, int cols, int which)
		{
			int nExtra = StrideExtraBytes(Max(rows, cols));
			Append(dst_, MonomorphicHeader(which, 2, nExtra));
			for (int ii = 0, n = rows; ii <= nExtra; ++ii)
			{
				Append(dst_, static_cast<uint8>(n & 0xFF));
				n >>= 8;
			}
			for (int ii = 0, n = cols; ii <= nExtra; ++ii)
			{
				Append(dst_, static_cast<uint8>(n & 0xFF));
				n >>= 8;
			}
		}

		void operator=(const Matrix_<>& val) override
		{
			SetMatrixHeader(val.Rows(), val.Cols(), M_DOUBLE);
			for (int iRow = 0; iRow < val.Rows(); ++iRow)
				for (auto v : val.Row(iRow))
					Append(dst_, v);
		}

		void operator=(const Matrix_<Cell_>& val) override
		{
			SetMatrixHeader(val.Rows(), val.Cols(), M_CELL);
			SetCell_ write(dst_);
			for (int iRow = 0; iRow < val.Rows(); ++iRow)
				for (const Cell_& c : val.Row(iRow))
					c.Visit(write);
		}

		void operator=(const Dictionary_& val) override
		{
			SetMatrixHeader(val.Size(), 2, M_CELL);
			SetCell_ write(dst_);
			for (const auto& [k, v] : val)
			{
				Cell_(k).Visit(write);
				v.Visit(write);
			}
		}
	};

	//------------------------------------------------------------------------
	// helper functions for UnSplat

	// binary source must support Peek() to see next byte
		// and template Take(count, dst) to populate destination
			// Take with sizeof > 1 may trigger alignment/endian adjustments

	// dimension 0 to 3: 2 bits
	// stride 1 to 4 bytes big: 2 bits
	// kind of data: 4 bits
	constexpr int WhichM(uint8 header)
	{
		return (header & 0xF0) >> 4;
	}
	constexpr int StrideBytesM(uint8 header)
	{
		return 1 + ((header & 0x0C) >> 2);
	}
	constexpr int DimM(uint8 header)
	{
		return header & 3;
	}

	template<class B_> String_ ReadToNull(B_& data)
	{
		String_ retval;
		char c;
		for ( ; ; )
		{
			data.Take(1, &c);
			if (c)
				retval.push_back(c);
			else
				return retval;
		}
	}
	
	// reverse size write in, e.g., SetVectorHeader above
	template<class B_> int ReadSize(B_& data, int bytes)
	{
		int step = 1, retval = 0;
		uint8 inc;
		for (int ii = 0; ii < bytes; ++ii)
		{
			data.Take(1, &inc);
			retval += inc * step;
			step <<= 8;
		}
		return retval;
	}


	template<class B_, bool verbose = false>
	struct XBinaryRead_ : Archive::View_
	{
		B_& data_;
		XBinaryRead_(B_& data) : data_(data) {}

		template<class E_> E_ Take1()
		{
			E_ retval;
			data_.Take(1, &retval);
			return retval;
		}

		int Tag() const
		{
			auto meta = Take1<uint8>();
			assert(WhichM(meta) == M_TAG);
			return Take1<int>();
		}

		String_ Type() const override
		{
			auto meta = Take1<uint8>();
			assert(WhichM(meta) == M_TYPE);
			return ReadToNull(data_);
		}

		const View_& Child(const String_& name) const override
		{
			return *this;
		}
		bool HasChild(const String_& name) const override
		{
			// Binary can't tell if it has a child, so we should AddNullChild() as needed
			return true;
		}
		void Unexpected(const String_& child_name) const override { }

		// output atomic values
		int AsInt() const override
		{
			auto meta = Take1<uint8>();
			assert(meta == MonomorphicHeader(M_INT, 0, 0));
			return Take1<int>();
		}
		double AsDouble() const override
		{
			auto meta = Take1<uint8>();
			assert(meta == MonomorphicHeader(M_DOUBLE, 0, 0));
			return Take1<double>();
		}
		bool AsBool() const override
		{
			auto meta = Take1<bool>();
			assert(meta == MonomorphicHeader(M_BOOL, 0, 0));
			return Take1<bool>();
		}
		Date_ AsDate() const override
		{
			auto meta = Take1<bool>();
			assert(meta == MonomorphicHeader(M_DATE, 0, 0));
			return Date::FromExcel(Take1<int>());
		}
		String_ AsString() const override
		{
			auto meta = Take1<uint8>();
			assert(meta == MonomorphicHeader(M_CSTR, 0, 0));
			return ReadToNull(data_);
		}

		int ReadVectorSize(int kind)
		{
			auto meta = Take1<uint8>();
			assert(WhichM(meta) == kind && DimM(meta) == 1);
			return ReadSize(data_, StrideBytesM(meta));
		}
		template<class E_> Vector_<E_> ReadVector(int kind)
		{
			int count = ReadVectorSize(kind);
			Vector_<E_> retval(count);
			data_.Take(count, &retval[0]);
			return retval;
		}

		Vector_<> AsDoubleVector() const override { return ReadVector<double>(M_DOUBLE); }
		Vector_<int> AsIntVector() const override { return ReadVector<int>(M_INT); }
		Vector_<bool> AsBoolVector() const override 
		{ 
			int count = ReadVectorSize(M_BOOL);
			Vector_<bool> retval(count);
			for (int ii = 0; ii < count; ++ii)
				retval[ii] = Take1<char>() == '1';
			return retval;
		}
		Vector_<String_> AsStringVector() const override
		{
			int count = ReadVectorSize(M_BOOL);
			Vector_<String_> retval(count);
			for (int ii = 0; ii < count; ++ii)
				retval[ii] = ReadToNull(data_);
			return retval;
		}
		Vector_<Date_> AsDateVector() const override
		{
			int count = ReadVectorSize(M_DATE);
			Vector_<Date_> retval(count);
			for (int ii = 0; ii < count; ++ii)
				retval[ii] = Date::FromExcel(Take1<int>());
			return retval;
		}
		Vector_<DateTime_> AsDateTimeVector() const override
		{
			int count = ReadVectorSize(M_DATETIME);
			Vector_<DateTime_> retval(count);
			for (int ii = 0; ii < count; ++ii)
				retval[ii] = DateTime::FromExcel(Take1<double>());
			return retval;
		}

		std::pair<int, int> ReadMatrixSize() const
		{
			auto meta = Take1<uint8>();
			assert(DimM(meta) == 2);
			int bytes = StrideBytesM(meta);
			int rows = ReadSize(data_, bytes);
			int cols = ReadSize(data_, bytes);
			return make_pair(rows, cols);
		}


		Matrix_<> AsDoubleMatrix() const override
		{
			auto [rows, cols] = ReadMatrixSize();
			Matrix_<> retval(rows, cols);
			// commit to row-major storage
			for (int row = 0; row < rows; ++row)
				data_.Take(&retval(row, 0), cols);
			return retval;
		}

		Matrix_<Cell_> AsCellMatrix() const override
		{
			auto [rows, cols] = ReadMatrixSize();
			Matrix_<Cell_> retval(rows, cols);
			for (int row = 0; row < rows; ++row)
			{
				for (int col = 0; col < cols; ++col)
				{
					switch (WhichM(Take1<uint8>()))
					{
					case M_BOOL:
						retval(row, col) = Take1<bool>();
						break;

					case M_DOUBLE:
						retval(row, col) = Take1<double>();
						break;

					case M_DATE:
						retval(row, col) = Date::FromExcel(Take1<int>());
						break;

					case M_DATETIME:
						retval(row, col) = DateTime::FromExcel(Take1<double>());
						break;

					case M_CSTR:
						retval(row, col) = ReadToNull(data_);
						break;

					case M_NONE:
						break;

					default:
						THROW("Unrecognized cell type input");
					}
				}
			}
			return retval;
		}

		Dictionary_ AsDictionary() const override
		{	// POSTPONED -- avoid constructing temp matrix
			auto temp = AsCellMatrix();
			REQUIRE(temp.Cols() == 2, "Can't extract dictionary because entry does not have two columns");
			Dictionary_ retval;
			for (int ir = 0; ir < temp.Rows(); ++ir)
				retval.Insert(Cell::ToString(temp(ir, 0)), temp(ir, 1));
			return retval;
		}

		Handle_<Storable_>& Known(Archive::Built_& built) const override
		{
			return built.known_[String::FromInt(Tag())];
		}
	};
}	// leave local

// a low-level byte store, for use by the above

namespace
{
#ifdef _BIG_ENDIAN
	void EndianFlip(uint8* start, size_t count, size_t size)
	{
		for (auto ii = 0; ii < count; ++ii)
			std::reverse(start + ii * size, start + (ii + 1) * size);
	}
#else
	inline void EndianFlip(uint8* start, size_t count, size_t size) {}
#endif

	constexpr int SMALL = 128, ALIGN = 16;
	struct VVByteStore_
	{
		// after all pushes, the concatenation of all vals_ is the binary store
		Vector_<Vector_<uint8>> vals_;
		int loc_;

		VVByteStore_() : vals_(1, Vector_<uint8>(SMALL & ~(ALIGN - 1))), loc_(0) {}

		void Grow(int require)
		{
			int nextSize = Max(require, (vals_.back().size() * 5) / 4 + SMALL);
			nextSize &= ~(ALIGN - 1);
			if (nextSize < require)
				nextSize += ALIGN;
			vals_.push_back(Vector_<uint8>(nextSize));
			loc_ = 0;
		}

		void Push(const uint8* start, int count, int size)
		{
			int pad = loc_ % size ? size - (loc_ % size) : 0;
			assert(loc_ + pad <= vals_.back().size());	// ALIGN should be large enough to avoid this
			loc_ += pad;
			int bytes = count * size;
			if (loc_ + bytes > vals_.back().size())
			{	// write what we can
				int part = vals_.back().size() - loc_;
				assert(part % size == 0);	// again, ALIGN should be big enough
				memcpy(&vals_.back()[loc_], start, part);
				EndianFlip(&vals_.back()[loc_], part / size, size);
				start += part;
				bytes -= part;
				Grow(bytes);
			}
			memcpy(&vals_.back()[loc_], start, bytes);
			EndianFlip(&vals_.back()[loc_], count, size);
			loc_ += bytes;
		}
	};

	struct RawByteView_
	{
		const uint8* src_;

		RawByteView_(const uint8* src) : src_(src)
		{
			REQUIRE(!(reinterpret_cast<unsigned long long>(src) % ALIGN), "Insufficiently aligned data");
		}

		uint8 Peek() const { return *src_; }

		template<class E_> void Take(size_t count, E_* dst)
		{
			memcpy(dst, src_, count * sizeof(E_));
			EndianFlip(reinterpret_cast<uint8*>(dst), count, sizeof(E_));
		}
	};
}