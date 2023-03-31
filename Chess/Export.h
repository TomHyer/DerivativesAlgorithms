#pragma once

#ifdef IS_CHESS
#define CHESS_EXPORT __declspec(dllexport)
#else
#define CHESS_EXPORT __declspec(dllimport)
#endif

