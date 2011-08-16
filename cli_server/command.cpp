//
// server_msg.cpp
// ~~~~~~~~~~~~~~
//
// Copyright (c) 2011 Victor C. Su
//

#include "command.hpp"

namespace cli
{
namespace server
{

void command::clear()
{
    op.clear();
    data.clear();
}

} // namespace server
} // namespace cli