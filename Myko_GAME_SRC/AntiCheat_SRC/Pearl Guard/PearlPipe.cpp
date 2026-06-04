/*
 * PearlPipe.cpp — Static member tanimlari
 * C++14 uyumlu. PearlPipe.h ile birlikte derlenir.
 */
#include "stdafx.h"
#include "PearlPipe.h"

HANDLE            PearlPipe::s_hPipe         = INVALID_HANDLE_VALUE;
std::atomic<bool> PearlPipe::s_connected     { false };
std::atomic<bool> PearlPipe::s_running       { false };
std::mutex        PearlPipe::s_writeMutex;
std::thread       PearlPipe::s_reconnThread;
DWORD             PearlPipe::s_lastReconnect = 0;
