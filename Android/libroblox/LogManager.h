#pragma once

namespace RBX
{
namespace JNI
{
namespace LogManager
{
void setupFastLog();
void tearDownFastLog();
void setupBreakpad(bool cleanup);
} // namespace LogManager
} // namespace JNI
} // namespace RBX
