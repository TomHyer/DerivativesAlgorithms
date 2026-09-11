#pragma once

#ifdef IS_SUDOKU
#define SUDOKU_EXPORT __declspec(dllexport)
#else
#define SUDOKU_EXPORT __declspec(dllimport)
#endif

struct noncopyable
{
	noncopyable() {}
	noncopyable(const noncopyable&) = delete;
	noncopyable& operator=(const noncopyable&) = delete;
};
